/**
 * @file    labdaq_net.h
 * @brief   LwIP-based Ethernet Network Stack for LabDAQ-Control:
 *          - UDP High-Speed Telemetry Streaming (Multicast & Unicast, 1000 Hz, µs timestamps)
 *          - Unified Dual-Port Command Listener (UDP + Serial SCPI parser on same socket)
 *          - Embedded Lightweight HTTP Web Server for sample rate & live channel visualizer
 *          Hardware: LAN8720A RMII PHY on STM32F407
 */

#ifndef INC_LABDAQ_NET_H_
#define INC_LABDAQ_NET_H_

#if __has_include("labdaq_config.h")
#include "labdaq_config.h"
#elif __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#endif

#if __has_include("labdaq_sampler.h")
#include "labdaq_sampler.h"
#elif __has_include("../Drivers/labdaq_sampler.h")
#include "../Drivers/labdaq_sampler.h"
#endif

#if __has_include("labdaq_comm.h")
#include "labdaq_comm.h"
#elif __has_include("../Drivers/labdaq_comm.h")
#include "../Drivers/labdaq_comm.h"
#endif
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Network & Port Configurations                                              */
/* ========================================================================== */
#define LABDAQ_DEFAULT_MULTICAST_IP   "239.255.0.100"  /* Standard Local Multicast Group */
#define LABDAQ_DEFAULT_UNICAST_IP     "192.168.1.255"  /* Subnet broadcast or unicast host */
#define LABDAQ_UDP_DATA_PORT          5001             /* Streaming Telemetry Port */
#define LABDAQ_UDP_CMD_PORT           5001             /* Symmetrical Command Listener Port */
#define LABDAQ_HTTP_PORT              80               /* Web Server Port */

/* Telemetry Mode */
typedef enum {
    LABDAQ_UDP_MODE_MULTICAST = 0,
    LABDAQ_UDP_MODE_UNICAST,
    LABDAQ_UDP_MODE_BROADCAST
} labdaq_udp_mode_t;

/* Streaming Packet with Microsecond Timestamp (58 bytes packed) */
#pragma pack(push, 1)
typedef struct {
    uint8_t   magic[2];            /* 0xAA, 0x55 */
    uint8_t   version;             /* 0x01 */
    uint8_t   channel_count;       /* 16 */
    uint32_t  sequence_num;        /* Incremental counter */
    uint64_t  timestamp_us;        /* 64-bit microsecond hardware timestamp */
    uint16_t  sample_rate_hz;      /* e.g. 1000 */
    uint16_t  raw_channels[16];    /* 16 x 12-bit ADC values (0..4095) */
    uint8_t   test_state;          /* 0=Idle, 2=Running, 3=Paused, etc. */
    uint8_t   filter_type;         /* 0=Bypass, 1=MAV, 2=EMA, 3=IIR, 4=Median */
    uint32_t  current_cycle;       /* Cyclic test counter */
    uint16_t  crc16;               /* CRC16-CCITT integrity checksum */
} labdaq_udp_stream_packet_t;
#pragma pack(pop)

/* Network Manager State */
typedef struct {
    bool                initialized;
    bool                link_up;
    labdaq_udp_mode_t   udp_mode;
    uint16_t            udp_port;
    char                multicast_ip[16];
    char                unicast_ip[16];
    uint32_t            packets_sent;
    uint32_t            commands_received;
    uint8_t             web_selected_channel; /* 0..15 for HTML chart */
    uint16_t            web_refresh_rate_ms;
} labdaq_net_state_t;

/**
 * @brief  Initialize Ethernet peripheral (RMII LAN8720A), LwIP stack, UDP sockets & HTTP server
 * @param  sys Pointer to master system handle
 * @return true if initialized successfully
 */
bool LABDAQ_Net_Init(labdaq_system_t *sys);

/**
 * @brief  Periodic network polling task (call in main superloop)
 * @param  sys Pointer to master system handle
 */
void LABDAQ_Net_Process(labdaq_system_t *sys);

/**
 * @brief  Stream single 16-channel telemetry frame over UDP (Multicast or Unicast)
 *         Triggered at 1000 Hz with hardware microsecond timestamp
 * @param  sys Pointer to master system handle
 */
void LABDAQ_Net_SendTelemetryUDP(labdaq_system_t *sys);

/**
 * @brief  Unified command processor: execute SCPI/Configuration commands
 *         from either UART Serial or UDP Network Socket identically
 * @param  sys Pointer to system handle
 * @param  cmd_input Null-terminated ASCII command string
 * @param  resp_out Buffer to store ASCII response
 * @param  resp_max Max response buffer size
 * @param  source_desc Description of caller ("SERIAL" or "UDP:IP:PORT")
 * @return Response length in bytes
 */
int LABDAQ_Unified_ExecuteCommand(labdaq_system_t *sys, 
                                 const char *cmd_input, 
                                 char *resp_out, 
                                 int resp_max,
                                 const char *source_desc);

/**
 * @brief  Configure UDP streaming mode (Multicast vs Unicast IP)
 */
void LABDAQ_Net_SetUDPMode(labdaq_udp_mode_t mode, const char *target_ip);

/**
 * @brief  Get high-resolution 64-bit microsecond timestamp using DWT->CYCCNT
 * @return Current microsecond timestamp since boot
 */
uint64_t LABDAQ_GetMicroseconds(void);

/**
 * @brief  Generate lightweight dynamic HTML dashboard for web browser
 * @param  sys Pointer to master system handle
 * @param  html_buf Output HTTP buffer
 * @param  max_len Maximum length of buffer
 * @return Length of HTML content
 */
int LABDAQ_HTTP_GenerateDashboard(labdaq_system_t *sys, char *html_buf, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_NET_H_ */
