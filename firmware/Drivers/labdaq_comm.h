/**
 ******************************************************************************
 * @file    labdaq_comm.h
 * @brief   Multi-Protocol Communication Engine for LabDAQ-Control
 *          Supports SCPI (ASCII), Modbus RTU (RS485), and Binary High-Speed TCP/UDP.
 ******************************************************************************
 */

#ifndef INC_LABDAQ_COMM_H_
#define INC_LABDAQ_COMM_H_

#include "labdaq_config.h"
#include "labdaq_sampler.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Binary High-Speed Streaming Packet Structure                               */
/* ========================================================================== */
#define LABDAQ_PACKET_MAGIC_0        0xAA
#define LABDAQ_PACKET_MAGIC_1        0x55

#pragma pack(push, 1)
typedef struct {
    uint8_t   magic[2];            /* 0xAA, 0x55 */
    uint8_t   packet_type;         /* 0x01 = Live Data Frame, 0x02 = Cyclic Event */
    uint8_t   channel_count;       /* 16 */
    uint32_t  sequence_num;        /* Monotonically increasing */
    uint32_t  timestamp_us;        /* Microsecond hardware timestamp */
    uint16_t  raw_channels[16];    /* 16 x 12-bit ADC counts */
    uint8_t   test_state;          /* 0=Idle, 2=Running, 3=Paused, 4=Completed */
    uint32_t  current_cycle;       /* Cyclic test counter */
    uint16_t  crc16;               /* CRC-16-CCITT checksum */
} labdaq_binary_packet_t;
#pragma pack(pop)

/* ========================================================================== */
/* Modbus RTU Register Map (Holding & Input Registers)                        */
/* ========================================================================== */
#define MODBUS_REG_DEVICE_ID         0x0000 /* RO: 0x4C44 ("LD") */
#define MODBUS_REG_FW_VERSION        0x0001 /* RO: Major << 8 | Minor */
#define MODBUS_REG_TEST_STATE        0x0002 /* RO: 0=Idle, 2=Run, 3=Pause, 4=Done */
#define MODBUS_REG_CYCLE_LOW         0x0003 /* RO: Current cycle lower 16 bits */
#define MODBUS_REG_CYCLE_HIGH        0x0004 /* RO: Current cycle upper 16 bits */

/* Input Registers: 16 Channels in Millivolts (0..3300 mV)                     */
#define MODBUS_INPUT_REG_CH0         0x0010 /* Ch 0 mV */
/* ... channels 1..15 are 0x0011 .. 0x001F */

/* Control Holding Registers                                                 */
#define MODBUS_REG_CMD_START_STOP    0x0020 /* RW: 1=Start, 0=Stop, 2=Pause */
#define MODBUS_REG_SAMPLE_RATE       0x0021 /* RW: Sample Rate in Hz */
#define MODBUS_REG_TARGET_CYCLE_L    0x0022 /* RW: Target cycles lower 16 bits */
#define MODBUS_REG_TARGET_CYCLE_H    0x0023 /* RW: Target cycles upper 16 bits */
#define MODBUS_REG_CYCLIC_FREQ_X100  0x0024 /* RW: Test frequency * 100 (e.g. 150 = 1.50 Hz) */

/**
 * @brief  Initialize communication drivers (USARTs, RS485 DIR pin)
 * @param  sys Pointer to master system handle
 */
void LABDAQ_Comm_Init(labdaq_system_t *sys);

/**
 * @brief  Parse incoming SCPI ASCII line and generate response string
 * @param  sys Pointer to system handle
 * @param  cmd_line Null-terminated ASCII input string
 * @param  resp_buf Buffer to store ASCII response (min 256 bytes)
 * @param  resp_max Maximum response buffer length
 * @return Length of response string
 */
int LABDAQ_SCPI_ProcessCommand(labdaq_system_t *sys, const char *cmd_line, char *resp_buf, int resp_max);

/**
 * @brief  Process incoming Modbus RTU frame and generate response frame
 * @param  sys Pointer to system handle
 * @param  rx_buf Received raw bytes
 * @param  rx_len Length of received frame
 * @param  tx_buf Output response frame buffer
 * @return Length of response frame in bytes (0 if no response/error)
 */
int LABDAQ_Modbus_ProcessFrame(labdaq_system_t *sys, const uint8_t *rx_buf, int rx_len, uint8_t *tx_buf);

/**
 * @brief  Construct a binary high-speed streaming packet from latest system frame
 * @param  sys Pointer to system handle
 * @param  packet Pointer to binary packet structure to populate
 */
void LABDAQ_Comm_BuildBinaryPacket(labdaq_system_t *sys, labdaq_binary_packet_t *packet);

/**
 * @brief  Calculate standard CRC16-CCITT for binary packets or Modbus
 * @param  data Pointer to byte buffer
 * @param  length Number of bytes
 * @return 16-bit CRC checksum
 */
uint16_t LABDAQ_CRC16_CCITT(const uint8_t *data, uint32_t length);
uint16_t LABDAQ_CRC16_Modbus(const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_COMM_H_ */
