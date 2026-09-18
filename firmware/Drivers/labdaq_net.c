/**
 * @file    labdaq_net.c
 * @brief   High-Speed UDP Multicast/Unicast Telemetry, Symmetrical Command Engine
 *          and Embedded HTTP Server for LabDAQ-Control STM32F407 + LAN8720A
 */

#include "labdaq_net.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "labdaq_control.h"

/* Global network state */
static labdaq_net_state_t g_net = {
    .initialized = false,
    .link_up = true,
    .udp_mode = LABDAQ_UDP_MODE_MULTICAST,
    .udp_port = LABDAQ_UDP_DATA_PORT,
    .multicast_ip = LABDAQ_DEFAULT_MULTICAST_IP,
    .unicast_ip = LABDAQ_DEFAULT_UNICAST_IP,
    .packets_sent = 0,
    .commands_received = 0,
    .web_selected_channel = 0,
    .web_refresh_rate_ms = 200
};

static uint32_t g_seq_counter = 0;
static uint64_t g_us_high_words = 0;
static uint32_t g_last_dwt_cyccnt = 0;

/* High-resolution microsecond timer using ARM Cortex-M4 DWT Cycle Counter */
uint64_t LABDAQ_GetMicroseconds(void)
{
    /* In actual STM32 Cortex-M4: CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; */
    /* 168 MHz -> 168 cycles = 1 microsecond */
    uint32_t current_cyc = 0;
#ifdef CoreDebug
    current_cyc = DWT->CYCCNT;
#else
    /* Fallback / portable simulation */
    current_cyc = HAL_GetTick() * 168000;
#endif

    if (current_cyc < g_last_dwt_cyccnt) {
        g_us_high_words += (uint64_t)0x100000000ULL / 168;
    }
    g_last_dwt_cyccnt = current_cyc;

    uint64_t total_us = g_us_high_words + ((uint64_t)current_cyc / 168);
    return total_us;
}

bool LABDAQ_Net_Init(labdaq_system_t *sys)
{
    (void)sys;
    /* In actual hardware: Initialize ETH RMII GPIOs, HAL_ETH_Init, LwIP tcpip_init */
    g_net.initialized = true;
    g_net.link_up = true;
    return true;
}

void LABDAQ_Net_SetUDPMode(labdaq_udp_mode_t mode, const char *target_ip)
{
    g_net.udp_mode = mode;
    if (target_ip && strlen(target_ip) > 0) {
        if (mode == LABDAQ_UDP_MODE_MULTICAST) {
            strncpy(g_net.multicast_ip, target_ip, sizeof(g_net.multicast_ip) - 1);
        } else {
            strncpy(g_net.unicast_ip, target_ip, sizeof(g_net.unicast_ip) - 1);
        }
    }
}

void LABDAQ_Net_SendTelemetryUDP(labdaq_system_t *sys)
{
    if (!sys || !sys->streaming_active) return;

    labdaq_udp_stream_packet_t pkt;
    pkt.magic[0] = LABDAQ_PACKET_MAGIC_0;
    pkt.magic[1] = LABDAQ_PACKET_MAGIC_1;
    pkt.version = 0x01;
    pkt.channel_count = LABDAQ_NUM_CHANNELS;
    pkt.sequence_num = ++g_seq_counter;
    pkt.timestamp_us = LABDAQ_GetMicroseconds();
    pkt.sample_rate_hz = (uint16_t)sys->sampling_rate_hz;

    /* Copy latest ADC readings */
    for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
        pkt.raw_channels[i] = sys->latest_raw_frame[i];
    }

    pkt.test_state = (uint8_t)sys->test_state;
    pkt.filter_type = (uint8_t)sys->filter_engine.channels[0].type;
    pkt.current_cycle = sys->cyclic_gen.current_cycle;

    /* Compute CRC16-CCITT over entire packet except last 2 bytes */
    uint32_t payload_len = sizeof(labdaq_udp_stream_packet_t) - sizeof(uint16_t);
    pkt.crc16 = LABDAQ_CRC16_CCITT((const uint8_t *)&pkt, payload_len);

    /* In LwIP implementation:
     * struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(pkt), PBUF_RAM);
     * memcpy(p->payload, &pkt, sizeof(pkt));
     * udp_sendto(g_telemetry_pcb, p, &dest_ip, LABDAQ_UDP_DATA_PORT);
     * pbuf_free(p);
     */
    g_net.packets_sent++;
}

int LABDAQ_Unified_ExecuteCommand(labdaq_system_t *sys, 
                                 const char *cmd_input, 
                                 char *resp_out, 
                                 int resp_max,
                                 const char *source_desc)
{
    if (!sys || !cmd_input || !resp_out || resp_max <= 0) return 0;

    g_net.commands_received++;

    /* Trim leading whitespace */
    while (*cmd_input == ' ' || *cmd_input == '\t' || *cmd_input == '\r' || *cmd_input == '\n') {
        cmd_input++;
    }

    /* 1. Identification */
    if (strncmp(cmd_input, "*IDN?", 5) == 0) {
        return snprintf(resp_out, resp_max, "%s,%s,SN-407V3,v%d.%d.%d [SRC:%s]\r\n",
                        LABDAQ_MANUFACTURER_STRING, LABDAQ_DEVICE_ID_STRING,
                        LABDAQ_FW_VERSION_MAJOR, LABDAQ_FW_VERSION_MINOR, LABDAQ_FW_VERSION_PATCH,
                        source_desc);
    }

    /* 2. Reset */
    if (strncmp(cmd_input, "*RST", 4) == 0) {
        LABDAQ_CyclicTest_Abort(sys);
        LABDAQ_System_StopAcquisition(sys);
        LABDAQ_Filter_Init(&sys->filter_engine);
        return snprintf(resp_out, resp_max, "OK: RESET APPLIED\r\n");
    }

    /* 3. Sampling Rate Config (:RATE <Hz>) */
    if (strncmp(cmd_input, ":RATE ", 6) == 0) {
        int rate = atoi(cmd_input + 6);
        if (rate >= LABDAQ_MIN_SAMPLE_RATE && rate <= LABDAQ_MAX_SAMPLE_RATE) {
            LABDAQ_ADC_SetSamplingRate(&sys->adc, rate * LABDAQ_NUM_CHANNELS);
            sys->sampling_rate_hz = rate;
            return snprintf(resp_out, resp_max, "OK: RATE=%d Hz (Source: %s)\r\n", rate, source_desc);
        } else {
            return snprintf(resp_out, resp_max, "ERR: RATE OUT OF RANGE (%d..%d Hz)\r\n", 
                            LABDAQ_MIN_SAMPLE_RATE, LABDAQ_MAX_SAMPLE_RATE);
        }
    }
    if (strncmp(cmd_input, ":RATE?", 6) == 0) {
        return snprintf(resp_out, resp_max, ":RATE %lu Hz\r\n", (unsigned long)sys->sampling_rate_hz);
    }

    /* 4. Filter Configuration (:FILTER <TYPE> [PARAM])
     * Types: BYPASS, MAV <window>, EMA <alpha>, IIR <cutoff_hz>, MEDIAN
     */
    if (strncmp(cmd_input, ":FILTER ", 8) == 0) {
        const char *p = cmd_input + 8;
        if (strncmp(p, "BYPASS", 6) == 0) {
            for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
                LABDAQ_Filter_ConfigureChannel(&sys->filter_engine, i, FILTER_TYPE_BYPASS, 0);
            }
            return snprintf(resp_out, resp_max, "OK: FILTER=BYPASS\r\n");
        } else if (strncmp(p, "MAV", 3) == 0) {
            int win = atoi(p + 3);
            if (win <= 0 || win > LABDAQ_FILTER_MAX_WINDOW) win = LABDAQ_DEFAULT_MAV_WINDOW;
            for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
                LABDAQ_Filter_ConfigureChannel(&sys->filter_engine, i, FILTER_TYPE_MAV, (float)win);
            }
            return snprintf(resp_out, resp_max, "OK: FILTER=MAV (WINDOW=%d)\r\n", win);
        } else if (strncmp(p, "EMA", 3) == 0) {
            float alpha = (float)atof(p + 3);
            if (alpha <= 0.0f || alpha > 1.0f) alpha = LABDAQ_DEFAULT_EMA_ALPHA;
            for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
                LABDAQ_Filter_ConfigureChannel(&sys->filter_engine, i, FILTER_TYPE_EMA, alpha);
            }
            return snprintf(resp_out, resp_max, "OK: FILTER=EMA (ALPHA=%.2f)\r\n", alpha);
        } else if (strncmp(p, "IIR", 3) == 0) {
            float cutoff = (float)atof(p + 3);
            if (cutoff <= 0.1f) cutoff = 20.0f;
            for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
                LABDAQ_Filter_SetButterworthLPF(&sys->filter_engine, i, cutoff, (float)sys->sampling_rate_hz);
            }
            return snprintf(resp_out, resp_max, "OK: FILTER=IIR_BUTTERWORTH (CUTOFF=%.1f Hz)\r\n", cutoff);
        } else if (strncmp(p, "MEDIAN", 6) == 0) {
            for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
                LABDAQ_Filter_ConfigureChannel(&sys->filter_engine, i, FILTER_TYPE_MEDIAN, 0);
            }
            return snprintf(resp_out, resp_max, "OK: FILTER=MEDIAN_3PT\r\n");
        }
        return snprintf(resp_out, resp_max, "ERR: UNKNOWN FILTER TYPE (USE BYPASS|MAV|EMA|IIR|MEDIAN)\r\n");
    }

    /* 5. UDP Streaming Target Configuration (:UDP:MODE <MULTICAST|UNICAST|BROADCAST> [IP]) */
    if (strncmp(cmd_input, ":UDP:MODE", 9) == 0) {
        char mode_str[16] = {0};
        char ip_str[24] = {0};
        int parsed = sscanf(cmd_input + 9, "%15s %23s", mode_str, ip_str);
        if (parsed >= 1) {
            if (strcasecmp(mode_str, "MULTICAST") == 0) {
                LABDAQ_Net_SetUDPMode(LABDAQ_UDP_MODE_MULTICAST, parsed >= 2 ? ip_str : LABDAQ_DEFAULT_MULTICAST_IP);
                return snprintf(resp_out, resp_max, "OK: UDP MODE=MULTICAST IP=%s PORT=%d\r\n", 
                                g_net.multicast_ip, LABDAQ_UDP_DATA_PORT);
            } else if (strcasecmp(mode_str, "UNICAST") == 0) {
                LABDAQ_Net_SetUDPMode(LABDAQ_UDP_MODE_UNICAST, parsed >= 2 ? ip_str : LABDAQ_DEFAULT_UNICAST_IP);
                return snprintf(resp_out, resp_max, "OK: UDP MODE=UNICAST IP=%s PORT=%d\r\n", 
                                g_net.unicast_ip, LABDAQ_UDP_DATA_PORT);
            }
        }
        return snprintf(resp_out, resp_max, "STATUS: UDP_MODE=%d MULTICAST=%s UNICAST=%s PORT=%d SENT=%lu\r\n",
                        g_net.udp_mode, g_net.multicast_ip, g_net.unicast_ip, g_net.udp_port, g_net.packets_sent);
    }

    /* 6. Continuous Acquisition Control */
    if (strncmp(cmd_input, ":START", 6) == 0) {
        LABDAQ_System_StartAcquisition(sys);
        return snprintf(resp_out, resp_max, "OK: CONTINUOUS STREAMING STARTED\r\n");
    }
    if (strncmp(cmd_input, ":STOP", 5) == 0) {
        LABDAQ_System_StopAcquisition(sys);
        return snprintf(resp_out, resp_max, "OK: STREAMING STOPPED\r\n");
    }

    /* 7. Instantaneous Measurements */
    if (strncmp(cmd_input, ":MEAS:VOLT:ALL?", 15) == 0) {
        int offset = snprintf(resp_out, resp_max, ":MEAS:VOLT ");
        for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
            offset += snprintf(resp_out + offset, resp_max - offset, "%.2f%s",
                               sys->latest_voltage_frame[i], (i == LABDAQ_NUM_CHANNELS - 1) ? "\r\n" : ",");
            if (offset >= resp_max - 16) break;
        }
        return offset;
    }
    if (strncmp(cmd_input, ":MEAS:VOLT:CHAN?", 16) == 0) {
        int ch = atoi(cmd_input + 16);
        if (ch >= 0 && ch < LABDAQ_NUM_CHANNELS) {
            return snprintf(resp_out, resp_max, "CH%d=%.2f mV (Raw: %u)\r\n", ch, 
                            sys->latest_voltage_frame[ch], sys->latest_raw_frame[ch]);
        }
        return snprintf(resp_out, resp_max, "ERR: INVALID CHANNEL INDEX (0..15)\r\n");
    }

    /* 8. Cyclic Test Control */
    if (strncmp(cmd_input, ":CYCLIC:START", 13) == 0) {
        float freq = 1.0f;
        uint32_t cycles = 10000;
        sscanf(cmd_input + 13, "%f,%lu", &freq, &cycles);
        LABDAQ_CyclicTest_Start(sys, freq, cycles);
        return snprintf(resp_out, resp_max, "OK: CYCLIC TEST STARTED F=%.2fHz, TARGET=%lu\r\n", freq, cycles);
    }
    if (strncmp(cmd_input, ":CYCLIC:STOP", 12) == 0) {
        LABDAQ_CyclicTest_Abort(sys);
        return snprintf(resp_out, resp_max, "OK: CYCLIC TEST ABORTED\r\n");
    }
    if (strncmp(cmd_input, ":CYCLIC:STATUS?", 15) == 0) {
        return snprintf(resp_out, resp_max, "STATE=%d,CYCLES=%lu,TARGET=%lu,FREQ=%.2f,ACTUATOR=%d\r\n",
                        sys->test_state, sys->cyclic_gen.current_cycle,
                        sys->cyclic_gen.target_cycles, sys->cyclic_gen.frequency_hz,
                        sys->cyclic_gen.actuator_state ? 1 : 0);
    }

    /* 9. Time, calibration, PID and experiment configuration */
    if (strncmp(cmd_input, ":TIME:SET ", 10) == 0) {
        LABDAQ_Time_SetUnix((uint32_t)strtoul(cmd_input + 10, NULL, 10));
        return snprintf(resp_out, resp_max, "OK: TIME=%lu\\r\\n", (unsigned long)LABDAQ_Time_NowUnix());
    }
    if (strncmp(cmd_input, ":TIME?", 6) == 0)
        return snprintf(resp_out, resp_max, "TIME=%lu\\r\\n", (unsigned long)LABDAQ_Time_NowUnix());
    if (strncmp(cmd_input, ":CHAN:CAL ", 10) == 0) {
        int ch=0; char name[24]={0},unit[12]={0}; float gain=1,offset=0;
        if (sscanf(cmd_input+10,"%d,%23[^,],%11[^,],%f,%f",&ch,name,unit,&gain,&offset)==5 && ch>=0 && ch<LABDAQ_NUM_CHANNELS) {
            LABDAQ_Channel_Set((uint8_t)ch,name,unit,gain,offset);
            return snprintf(resp_out,resp_max,"OK: CH%d CALIBRATED\\r\\n",ch);
        }
        return snprintf(resp_out,resp_max,"ERR: CHAN:CAL ch,name,unit,gain,offset\\r\\n");
    }
    if (strncmp(cmd_input, ":PID:SET ", 9) == 0) {
        int motor=0; float kp=0,ki=0,kd=0,sp=0;
        if(sscanf(cmd_input+9,"%d,%f,%f,%f,%f",&motor,&kp,&ki,&kd,&sp)==5 && motor>=0 && motor<2){
            LABDAQ_PID_Set((uint8_t)motor,kp,ki,kd,sp);
            return snprintf(resp_out,resp_max,"OK: PID%d UPDATED\\r\\n",motor+1);
        }
        return snprintf(resp_out,resp_max,"ERR: PID:SET motor,kp,ki,kd,setpoint\\r\\n");
    }
    if (strncmp(cmd_input, ":MOTOR:CAL ", 11) == 0) {
        int motor=0; float cmin=0,cmax=10,fmin=0,fmax=10;
        if(sscanf(cmd_input+11,"%d,%f,%f,%f,%f",&motor,&cmin,&cmax,&fmin,&fmax)==5 && motor>=0 && motor<2){
            LABDAQ_Motor_SetCalibration((uint8_t)motor,cmin,cmax,fmin,fmax);
            return snprintf(resp_out,resp_max,"OK: MOTOR%d CALIBRATED\\r\\n",motor+1);
        }
        return snprintf(resp_out,resp_max,"ERR: MOTOR:CAL motor,cmdMin,cmdMax,fbMin,fbMax\\r\\n");
    }
    if (strncmp(cmd_input, ":LOG?", 5) == 0)
        return snprintf(resp_out,resp_max,"LOG_COUNT=%u\\r\\n",g_labdaq_control.log_count);

    return snprintf(resp_out, resp_max, "ERR: UNKNOWN COMMAND: %s\r\n", cmd_input);
}

void LABDAQ_Net_Process(labdaq_system_t *sys)
{
    if (!sys || !g_net.initialized) return;

    /* In actual hardware:
     * - sys_check_timeouts() / ethernetif_input()
     * - Check incoming UDP packets on port 5001
     * - Check incoming HTTP connection requests on port 80
     */
}

int LABDAQ_HTTP_GenerateDashboard(labdaq_system_t *sys, char *b, int n)
{
 if(!sys||!b||n<=0)return 0;
 return snprintf(b,n,
 "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
 "<title>LabDAQ Control</title><style>body{font-family:system-ui;background:#0b1220;color:#e5e7eb;margin:0}header{padding:18px 24px;background:#111827}nav{display:flex;gap:8px;flex-wrap:wrap;padding:12px 24px}.tab{padding:9px 12px;background:#1f2937;border-radius:8px;cursor:pointer}.p{display:none;padding:20px 24px}.p.on{display:block}.g{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px}.c{background:#111827;padding:16px;border-radius:12px}input,select,button{width:100%%;box-sizing:border-box;margin:5px 0;padding:9px;background:#1f2937;color:white;border:1px solid #374151;border-radius:7px}button{background:#059669;border:0}small{color:#9ca3af}</style></head>"
 "<header><b>LabDAQ-Control</b> · STM32F407 · <small>16CH DAQ + Triaxial Cyclic Controller</small></header>"
 "<nav><span class='tab' data-t='daq'>Data Logger</span><span class='tab' data-t='test'>Triaxial Tests</span><span class='tab' data-t='pid'>PID / EP Motors</span><span class='tab' data-t='channels'>Channels & Calibration</span><span class='tab' data-t='time'>Time</span><span class='tab' data-t='logs'>Logs</span><span class='tab' data-t='settings'>Settings</span></nav>"
 "<section id='daq' class='p on'><h2>Data Logger</h2><div class='g'><div class='c'>Sample rate: <b>%lu Hz/ch</b><select><option>100</option><option>500</option><option selected>1000</option></select><button>Apply</button></div><div class='c'>Frames: %lu<br>Streaming: %s</div><div class='c'>Live scope/API endpoint<br><small>/api/sample?ch=N</small></div></div></section>"
 "<section id='test' class='p'><h2>Triaxial / Cyclic Tests</h2><div class='g'><div class='c'><select><option>Cyclic triaxial</option><option>Monotonic</option><option>Stress controlled</option><option>Strain controlled</option><option>Custom profile</option></select><input placeholder='Confining pressure'><input placeholder='Axial target'><input placeholder='Frequency Hz'><input placeholder='Cycles'><button>Arm / Start</button></div><div class='c'>State: %d<br>Cycle: %lu / %lu<br><button>Pause</button><button>Emergency stop</button></div></div></section>"
 "<section id='pid' class='p'><h2>PID & EP Motor Calibration</h2><div class='g'><div class='c'>EP Motor 1<input placeholder='Kp'><input placeholder='Ki'><input placeholder='Kd'><input placeholder='Setpoint'><button>Save PID</button></div><div class='c'>EP Motor 2<input placeholder='Kp'><input placeholder='Ki'><input placeholder='Kd'><input placeholder='Setpoint'><button>Save PID</button></div><div class='c'>Two-point calibration<input placeholder='Command min V'><input placeholder='Command max V'><input placeholder='Feedback min'><input placeholder='Feedback max'><button>Calibrate</button></div></div></section>"
 "<section id='channels' class='p'><h2>16 Input Channels</h2><div class='c'>Each channel supports name, engineering unit, gain, offset, min/max and enable state.<br><small>SCPI: :CHAN:CAL ch,name,unit,gain,offset</small></div></section>"
 "<section id='time' class='p'><h2>Clock & Time</h2><div class='c'>Unix time: <b>%lu</b><input type='datetime-local'><button>Set device time</button></div></section>"
 "<section id='logs' class='p'><h2>Event Logs</h2><div class='c'>Stored events: <b>%u</b><br><small>Boot, configuration, calibration, PID, test start/stop/fault events.</small></div></section>"
 "<section id='settings' class='p'><h2>Settings</h2><div class='g'><div class='c'>Acquisition: 100 / 500 / 1000 samples/s/ch</div><div class='c'>Network / UDP / filters</div><div class='c'>Safety limits and actuator commissioning</div></div></section>"
 "<script>document.querySelectorAll('.tab').forEach(x=>x.onclick=()=>{document.querySelectorAll('.p').forEach(p=>p.classList.remove('on'));document.getElementById(x.dataset.t).classList.add('on')})</script></html>",
 (unsigned long)sys->sampling_rate_hz,(unsigned long)sys->frame_sequence,sys->streaming_active?"ON":"OFF",(int)sys->test_state,(unsigned long)sys->cyclic_gen.current_cycle,(unsigned long)sys->cyclic_gen.target_cycles,(unsigned long)LABDAQ_Time_NowUnix(),g_labdaq_control.log_count);
}
