/**
 ******************************************************************************
 * @file    labdaq_comm.c
 * @brief   Multi-Protocol Communication Engine (SCPI, Modbus RTU, Binary)
 ******************************************************************************
 */

#include "labdaq_comm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void LABDAQ_Comm_Init(labdaq_system_t *sys)
{
    (void)sys;

    /* Configure RS485 Direction control pin (PD7) */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LABDAQ_RS485_DIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LABDAQ_RS485_DIR_PORT, &GPIO_InitStruct);

    /* Default to RX mode (Low) */
    HAL_GPIO_WritePin(LABDAQ_RS485_DIR_PORT, LABDAQ_RS485_DIR_PIN, GPIO_PIN_RESET);
}

int LABDAQ_SCPI_ProcessCommand(labdaq_system_t *sys, const char *cmd_line, char *resp_buf, int resp_max)
{
    if (!sys || !cmd_line || !resp_buf || resp_max <= 0) return 0;

    /* Trim leading spaces and newline characters */
    while (*cmd_line == ' ' || *cmd_line == '\t') cmd_line++;

    /* *IDN? Query */
    if (strncmp(cmd_line, "*IDN?", 5) == 0) {
        return snprintf(resp_buf, resp_max, "%s,%s,SN-407V3,v%d.%d.%d\r\n",
                        LABDAQ_MANUFACTURER_STRING, LABDAQ_DEVICE_ID_STRING,
                        LABDAQ_FW_VERSION_MAJOR, LABDAQ_FW_VERSION_MINOR, LABDAQ_FW_VERSION_PATCH);
    }

    /* *RST Command */
    if (strncmp(cmd_line, "*RST", 4) == 0) {
        LABDAQ_CyclicTest_Abort(sys);
        LABDAQ_System_StopAcquisition(sys);
        LABDAQ_Filter_Init(&sys->filter_engine);
        return snprintf(resp_buf, resp_max, "OK: RESET\r\n");
    }

    /* :MEAS:VOLT:ALL? */
    if (strncmp(cmd_line, ":MEAS:VOLT:ALL?", 15) == 0) {
        int offset = 0;
        offset += snprintf(resp_buf + offset, resp_max - offset, ":MEAS:VOLT ");
        for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
            offset += snprintf(resp_buf + offset, resp_max - offset, "%.2f%s",
                               sys->latest_voltage_frame[i], (i == LABDAQ_NUM_CHANNELS - 1) ? "\r\n" : ",");
            if (offset >= resp_max - 16) break;
        }
        return offset;
    }

    /* :MEAS:VOLT:CHAN? <ch> */
    if (strncmp(cmd_line, ":MEAS:VOLT:CHAN?", 16) == 0) {
        int ch = atoi(cmd_line + 16);
        if (ch >= 0 && ch < LABDAQ_NUM_CHANNELS) {
            return snprintf(resp_buf, resp_max, "%.2f mV\r\n", sys->latest_voltage_frame[ch]);
        } else {
            return snprintf(resp_buf, resp_max, "ERR: INVALID CHANNEL\r\n");
        }
    }

    /* :MEAS:RAW:ALL? */
    if (strncmp(cmd_line, ":MEAS:RAW:ALL?", 14) == 0) {
        int offset = 0;
        offset += snprintf(resp_buf + offset, resp_max - offset, ":RAW ");
        for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
            offset += snprintf(resp_buf + offset, resp_max - offset, "%u%s",
                               sys->latest_raw_frame[i], (i == LABDAQ_NUM_CHANNELS - 1) ? "\r\n" : ",");
            if (offset >= resp_max - 16) break;
        }
        return offset;
    }

    /* :RATE <Hz> */
    if (strncmp(cmd_line, ":RATE ", 6) == 0) {
        int rate = atoi(cmd_line + 6);
        if (rate >= LABDAQ_MIN_SAMPLE_RATE && rate <= LABDAQ_MAX_SAMPLE_RATE) {
            LABDAQ_ADC_SetSamplingRate(&sys->adc, rate * LABDAQ_NUM_CHANNELS);
            sys->sampling_rate_hz = rate;
            return snprintf(resp_buf, resp_max, "OK: RATE=%d Hz\r\n", rate);
        } else {
            return snprintf(resp_buf, resp_max, "ERR: OUT OF RANGE (%d..%d)\r\n", 
                            LABDAQ_MIN_SAMPLE_RATE, LABDAQ_MAX_SAMPLE_RATE);
        }
    }

    /* :START */
    if (strncmp(cmd_line, ":START", 6) == 0) {
        LABDAQ_System_StartAcquisition(sys);
        return snprintf(resp_buf, resp_max, "OK: ACQUISITION STARTED\r\n");
    }

    /* :STOP */
    if (strncmp(cmd_line, ":STOP", 5) == 0) {
        LABDAQ_System_StopAcquisition(sys);
        return snprintf(resp_buf, resp_max, "OK: ACQUISITION STOPPED\r\n");
    }

    /* :CYCLIC:START <freq>,<target_cycles> */
    if (strncmp(cmd_line, ":CYCLIC:START", 13) == 0) {
        float freq = 1.0f;
        uint32_t cycles = 1000;
        sscanf(cmd_line + 13, "%f,%lu", &freq, &cycles);
        LABDAQ_CyclicTest_Start(sys, freq, cycles);
        return snprintf(resp_buf, resp_max, "OK: CYCLIC TEST STARTED F=%.2fHz, TARGET=%lu\r\n", freq, cycles);
    }

    /* :CYCLIC:STOP */
    if (strncmp(cmd_line, ":CYCLIC:STOP", 12) == 0) {
        LABDAQ_CyclicTest_Abort(sys);
        return snprintf(resp_buf, resp_max, "OK: CYCLIC TEST STOPPED\r\n");
    }

    /* :CYCLIC:STATUS? */
    if (strncmp(cmd_line, ":CYCLIC:STATUS?", 15) == 0) {
        const char *state_str = "IDLE";
        if (sys->test_state == TEST_STATE_RUNNING) state_str = "RUNNING";
        else if (sys->test_state == TEST_STATE_PAUSED) state_str = "PAUSED";
        else if (sys->test_state == TEST_STATE_COMPLETED) state_str = "COMPLETED";

        return snprintf(resp_buf, resp_max, "STATE=%s,CYCLES=%lu,TARGET=%lu,FREQ=%.2f\r\n",
                        state_str, sys->cyclic_gen.current_cycle,
                        sys->cyclic_gen.target_cycles, sys->cyclic_gen.frequency_hz);
    }

    /* Unknown Command */
    return snprintf(resp_buf, resp_max, "ERR: UNKNOWN COMMAND\r\n");
}

/* Modbus RTU CRC16 */
uint16_t LABDAQ_CRC16_Modbus(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* CRC16-CCITT for binary stream packets */
uint16_t LABDAQ_CRC16_CCITT(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

int LABDAQ_Modbus_ProcessFrame(labdaq_system_t *sys, const uint8_t *rx_buf, int rx_len, uint8_t *tx_buf)
{
    if (!sys || !rx_buf || rx_len < 4 || !tx_buf) return 0;

    /* Verify CRC16 */
    uint16_t rx_crc = (rx_buf[rx_len - 1] << 8) | rx_buf[rx_len - 2];
    uint16_t calc_crc = LABDAQ_CRC16_Modbus(rx_buf, rx_len - 2);
    if (rx_crc != calc_crc) return 0; /* CRC error */

    uint8_t slave_addr = rx_buf[0];
    if (slave_addr != LABDAQ_MODBUS_DEFAULT_ADDR && slave_addr != 0) {
        return 0; /* Not our address */
    }

    uint8_t func_code = rx_buf[1];
    uint16_t start_reg = (rx_buf[2] << 8) | rx_buf[3];
    uint16_t reg_count = (rx_buf[4] << 8) | rx_buf[5];

    tx_buf[0] = slave_addr;
    tx_buf[1] = func_code;

    /* FC 03 (Read Holding Registers) or FC 04 (Read Input Registers) */
    if (func_code == 0x03 || func_code == 0x04) {
        if (reg_count > 32) reg_count = 32;
        uint8_t byte_count = reg_count * 2;
        tx_buf[2] = byte_count;

        int out_idx = 3;
        for (uint16_t i = 0; i < reg_count; i++) {
            uint16_t current_reg = start_reg + i;
            uint16_t val = 0;

            if (current_reg == MODBUS_REG_DEVICE_ID) {
                val = 0x4C44; /* "LD" */
            } else if (current_reg == MODBUS_REG_FW_VERSION) {
                val = (LABDAQ_FW_VERSION_MAJOR << 8) | LABDAQ_FW_VERSION_MINOR;
            } else if (current_reg == MODBUS_REG_TEST_STATE) {
                val = (uint16_t)sys->test_state;
            } else if (current_reg == MODBUS_REG_CYCLE_LOW) {
                val = (uint16_t)(sys->cyclic_gen.current_cycle & 0xFFFF);
            } else if (current_reg == MODBUS_REG_CYCLE_HIGH) {
                val = (uint16_t)((sys->cyclic_gen.current_cycle >> 16) & 0xFFFF);
            } else if (current_reg >= MODBUS_INPUT_REG_CH0 && current_reg < (MODBUS_INPUT_REG_CH0 + LABDAQ_NUM_CHANNELS)) {
                uint8_t ch = current_reg - MODBUS_INPUT_REG_CH0;
                val = (uint16_t)sys->latest_voltage_frame[ch];
            } else if (current_reg == MODBUS_REG_SAMPLE_RATE) {
                val = (uint16_t)sys->sampling_rate_hz;
            }

            tx_buf[out_idx++] = (val >> 8) & 0xFF;
            tx_buf[out_idx++] = val & 0xFF;
        }

        uint16_t crc = LABDAQ_CRC16_Modbus(tx_buf, out_idx);
        tx_buf[out_idx++] = crc & 0xFF;
        tx_buf[out_idx++] = (crc >> 8) & 0xFF;
        return out_idx;
    }
    /* FC 06: Write Single Holding Register */
    else if (func_code == 0x06) {
        uint16_t reg_addr = start_reg;
        uint16_t write_val = reg_count;

        if (reg_addr == MODBUS_REG_CMD_START_STOP) {
            if (write_val == 1) LABDAQ_CyclicTest_Start(sys, sys->cyclic_gen.frequency_hz, sys->cyclic_gen.target_cycles);
            else if (write_val == 0) LABDAQ_CyclicTest_Abort(sys);
            else if (write_val == 2) LABDAQ_CyclicTest_Pause(sys);
        } else if (reg_addr == MODBUS_REG_SAMPLE_RATE) {
            LABDAQ_ADC_SetSamplingRate(&sys->adc, write_val * LABDAQ_NUM_CHANNELS);
            sys->sampling_rate_hz = write_val;
        }

        /* Echo back written frame */
        memcpy(tx_buf, rx_buf, 6);
        uint16_t crc = LABDAQ_CRC16_Modbus(tx_buf, 6);
        tx_buf[6] = crc & 0xFF;
        tx_buf[7] = (crc >> 8) & 0xFF;
        return 8;
    }

    return 0;
}

void LABDAQ_Comm_BuildBinaryPacket(labdaq_system_t *sys, labdaq_binary_packet_t *packet)
{
    if (!sys || !packet) return;

    packet->magic[0] = LABDAQ_PACKET_MAGIC_0;
    packet->magic[1] = LABDAQ_PACKET_MAGIC_1;
    packet->packet_type = 0x01;
    packet->channel_count = LABDAQ_NUM_CHANNELS;
    packet->sequence_num = sys->frame_sequence;
    packet->timestamp_us = HAL_GetTick() * 1000;
    
    for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
        packet->raw_channels[i] = sys->latest_raw_frame[i];
    }

    packet->test_state = (uint8_t)sys->test_state;
    packet->current_cycle = sys->cyclic_gen.current_cycle;

    /* Compute CRC over entire packet except last 2 bytes */
    uint32_t payload_len = sizeof(labdaq_binary_packet_t) - sizeof(uint16_t);
    packet->crc16 = LABDAQ_CRC16_CCITT((const uint8_t *)packet, payload_len);
}
