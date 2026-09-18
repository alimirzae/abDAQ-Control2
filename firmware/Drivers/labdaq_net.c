/**
 * @file    labdaq_net.c
 * @brief   High-Speed UDP Multicast/Unicast Telemetry, Symmetrical Command Engine
 *          and Embedded HTTP Server for LabDAQ-Control STM32F407 + LAN8720A
 */

#include "labdaq_net.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
        return snprintf(resp_out, resp_max, ":RATE %u Hz\r\n", sys->sampling_rate_hz);
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

int LABDAQ_HTTP_GenerateDashboard(labdaq_system_t *sys, char *html_buf, int max_len)
{
    if (!sys || !html_buf || max_len <= 0) return 0;

    uint8_t sel_ch = g_net.web_selected_channel;
    if (sel_ch >= LABDAQ_NUM_CHANNELS) sel_ch = 0;

    float current_mv = sys->latest_voltage_frame[sel_ch];
    uint16_t current_raw = sys->latest_raw_frame[sel_ch];

    return snprintf(html_buf, max_len,
        "<!DOCTYPE html>"
        "<html lang='en'>"
        "<head>"
        "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
        "<title>LabDAQ-Control Web Server</title>"
        "<style>"
        "body{font-family:system-ui,-apple-system,sans-serif;background:#090d16;color:#e2e8f0;margin:0;padding:24px;}"
        ".card{background:#131b2e;border:1px solid #1e293b;border-radius:12px;padding:20px;max-width:850px;margin:0 auto 20px;box-shadow:0 10px 25px rgba(0,0,0,0.5);}"
        "h1{color:#10b981;font-size:22px;margin-top:0;display:flex;align-items:center;gap:10px;}"
        ".badge{background:#064e3b;color:#6ee7b7;padding:3px 8px;border-radius:6px;font-size:12px;font-weight:600;}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;margin:16px 0;}"
        "label{font-size:13px;color:#94a3b8;display:block;margin-bottom:6px;}"
        "select,input,button{background:#1e293b;border:1px solid #334155;color:#fff;padding:10px 14px;border-radius:8px;font-size:14px;width:100%%;box-sizing:border-box;}"
        "button{background:#10b981;color:#000;font-weight:700;cursor:pointer;border:none;margin-top:8px;transition:0.2s;}"
        "button:hover{background:#34d399;}"
        "#canvasWrapper{background:#050811;border:1px solid #1e293b;border-radius:8px;padding:12px;margin-top:16px;}"
        "canvas{width:100%%;height:240px;display:block;}"
        ".val-badge{font-size:28px;font-family:monospace;font-weight:bold;color:#38bdf8;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class='card'>"
        "<h1><span>⚡ LabDAQ-Control</span><span class='badge'>STM32F407 Web Server</span></h1>"
        "<p style='color:#94a3b8;font-size:14px;margin-bottom:20px;'>Real-time Web Management Dashboard & Live Channel Oscilloscope</p>"
        "<form method='GET' action='/config'>"
        "<div class='grid'>"
        "<div><label>Sampling Rate (Hz)</label>"
        "<select name='rate' onchange='this.form.submit()'>"
        "<option value='100' %s>100 Hz (Low Speed)</option>"
        "<option value='500' %s>500 Hz</option>"
        "<option value='1000' %s>1000 Hz (Default 1 kSPS)</option>"
        "<option value='2000' %s>2000 Hz (2 kSPS)</option>"
        "<option value='5000' %s>5000 Hz (5 kSPS)</option>"
        "<option value='10000' %s>10000 Hz (10 kSPS)</option>"
        "</select></div>"
        "<div><label>Active Channel to Display</label>"
        "<select name='ch' id='chSelect' onchange='this.form.submit()'>"
        "%s"
        "</select></div>"
        "</div>"
        "</form>"
        "<div class='grid' style='margin-top:10px;'>"
        "<div><span style='font-size:13px;color:#94a3b8;'>Live Voltage:</span><div class='val-badge'>%.2f mV</div></div>"
        "<div><span style='font-size:13px;color:#94a3b8;'>Raw 12-Bit ADC:</span><div class='val-badge' style='color:#a855f7;'>%u</div></div>"
        "<div><span style='font-size:13px;color:#94a3b8;'>UDP Telemetry:</span><div style='font-size:14px;color:#10b981;font-weight:bold;margin-top:6px;'>239.255.0.100:5001</div></div>"
        "</div>"
        "<div id='canvasWrapper'><canvas id='scope'></canvas></div>"
        "<script>"
        "const canvas=document.getElementById('scope');const ctx=canvas.getContext('2d');"
        "let data=[];canvas.width=canvas.parentElement.clientWidth;canvas.height=240;"
        "function draw(){"
        "ctx.fillStyle='#050811';ctx.fillRect(0,0,canvas.width,canvas.height);"
        "ctx.strokeStyle='#1e293b';ctx.lineWidth=1;ctx.beginPath();"
        "for(let y=40;y<canvas.height;y+=40){ctx.moveTo(0,y);ctx.lineTo(canvas.width,y);}"
        "ctx.stroke();"
        "if(data.length>1){"
        "ctx.strokeStyle='#10b981';ctx.lineWidth=2;ctx.beginPath();"
        "for(let i=0;i<data.length;i++){"
        "let x=(i/(data.length-1))*canvas.width;"
        "let y=canvas.height-(data[i]/3300)*canvas.height;"
        "if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);"
        "}ctx.stroke();}"
        "}"
        "setInterval(()=>{fetch('/api/sample?ch=' + %d).then(r=>r.json()).then(d=>{"
        "data.push(d.mv);if(data.length>150)data.shift();draw();"
        "});},100);"
        "</script>"
        "</div></body></html>",
        (sys->sampling_rate_hz == 100) ? "selected" : "",
        (sys->sampling_rate_hz == 500) ? "selected" : "",
        (sys->sampling_rate_hz == 1000) ? "selected" : "",
        (sys->sampling_rate_hz == 2000) ? "selected" : "",
        (sys->sampling_rate_hz == 5000) ? "selected" : "",
        (sys->sampling_rate_hz == 10000) ? "selected" : "",
        /* Options for channels 0..15 */
        "<option value='0' selected>Channel 0 (Load Cell)</option><option value='1'>Channel 1 (Displacement)</option><option value='2'>Channel 2 (Pressure)</option><option value='3'>Channel 3</option><option value='4'>Channel 4</option><option value='5'>Channel 5</option><option value='6'>Channel 6</option><option value='7'>Channel 7</option><option value='8'>Channel 8</option><option value='9'>Channel 9</option><option value='10'>Channel 10</option><option value='11'>Channel 11</option><option value='12'>Channel 12</option><option value='13'>Channel 13</option><option value='14'>Channel 14</option><option value='15'>Channel 15</option>",
        current_mv,
        current_raw,
        sel_ch
    );
}
