/**
 * @file    labdaq_config.h
 * @brief   Global configuration parameters for LabDAQ-Control system
 *          Hardware: EWB-STM32F407V-LAN-V3.0 + TXS0108E Level Shifter + MUX16
 */

#ifndef DRIVERS_LABDAQ_CONFIG_H_
#define DRIVERS_LABDAQ_CONFIG_H_

#if __has_include("../Config/labdaq_config.h")
#include "../Config/labdaq_config.h"
#elif __has_include("../../Config/labdaq_config.h")
#include "../../Config/labdaq_config.h"
#elif __has_include("Config/labdaq_config.h")
#include "Config/labdaq_config.h"
#else

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* System & Firmware Metadata */
#define LABDAQ_FW_VERSION_MAJOR      1
#define LABDAQ_FW_VERSION_MINOR      0
#define LABDAQ_FW_VERSION_PATCH      0
#define LABDAQ_DEVICE_ID_STRING      "LabDAQ-STM32F407-16CH"
#define LABDAQ_MANUFACTURER_STRING   "LabDAQ Instrumentation"

/* Hardware Pin Mapping (EWB-STM32F407V-LAN-V3.0 + TXS0108E Level Shifter) */
#define LABDAQ_MUX_PORT              GPIOE
#define LABDAQ_MUX_S0_PIN            GPIO_PIN_8
#define LABDAQ_MUX_S1_PIN            GPIO_PIN_9
#define LABDAQ_MUX_S2_PIN            GPIO_PIN_10
#define LABDAQ_MUX_S3_PIN            GPIO_PIN_11
#define LABDAQ_MUX_EN_PIN            GPIO_PIN_12

#define LABDAQ_SYNC_OUT_PIN          GPIO_PIN_13
#define LABDAQ_ACTUATOR_PIN          GPIO_PIN_14
#define LABDAQ_STATUS_PIN            GPIO_PIN_15

#define LABDAQ_ADC_INSTANCE          ADC1
#define LABDAQ_ADC_CHANNEL           ADC_CHANNEL_4
#define LABDAQ_ADC_PORT              GPIOA
#define LABDAQ_ADC_PIN               GPIO_PIN_4

#define LABDAQ_DIRECT_ADC_CH1        ADC_CHANNEL_3
#define LABDAQ_DIRECT_ADC_CH2        ADC_CHANNEL_5

#define LABDAQ_LED_PORT              GPIOC
#define LABDAQ_LED_PIN               GPIO_PIN_13
#define LABDAQ_BTN_PORT              GPIOA
#define LABDAQ_BTN_PIN               GPIO_PIN_0

#define LABDAQ_RS485_USART           USART2
#define LABDAQ_RS485_PORT            GPIOD
#define LABDAQ_RS485_TX_PIN          GPIO_PIN_5
#define LABDAQ_RS485_RX_PIN          GPIO_PIN_6
#define LABDAQ_RS485_DIR_PORT        GPIOD
#define LABDAQ_RS485_DIR_PIN         GPIO_PIN_7

#define LABDAQ_CAN_INSTANCE          CAN1
#define LABDAQ_CAN_PORT              GPIOB
#define LABDAQ_CAN_RX_PIN            GPIO_PIN_8
#define LABDAQ_CAN_TX_PIN            GPIO_PIN_9

#define LABDAQ_ETH_TCP_PORT          5000
#define LABDAQ_ETH_UDP_PORT          5001
#define LABDAQ_DEFAULT_IP_ADDR       "192.168.1.150"
#define LABDAQ_DEFAULT_NETMASK       "255.255.255.0"
#define LABDAQ_DEFAULT_GATEWAY       "192.168.1.1"

#define LABDAQ_NUM_CHANNELS          16
#define LABDAQ_ADC_RESOLUTION_BITS   12
#define LABDAQ_ADC_VREF_MV           3300.0f

#define LABDAQ_DEFAULT_SAMPLE_RATE   1000
#define LABDAQ_MAX_SAMPLE_RATE       10000
#define LABDAQ_MIN_SAMPLE_RATE       10

#define LABDAQ_MUX_SETTLE_US         5

#define LABDAQ_BUFFER_BLOCK_FRAMES   128
#define LABDAQ_RAW_BUFFER_SIZE       (LABDAQ_NUM_CHANNELS * LABDAQ_BUFFER_BLOCK_FRAMES * 2)

#define LABDAQ_FILTER_MAX_WINDOW     64
#define LABDAQ_DEFAULT_MAV_WINDOW    8
#define LABDAQ_DEFAULT_EMA_ALPHA     0.25f

#define LABDAQ_CYCLIC_DEFAULT_FREQ   1.0f
#define LABDAQ_CYCLIC_MAX_FREQ       50.0f
#define LABDAQ_CYCLIC_DEFAULT_CYCLES 10000

#define LABDAQ_MODBUS_DEFAULT_ADDR   1
#define LABDAQ_MODBUS_BAUDRATE       115200

#ifdef __cplusplus
}
#endif

#endif

#endif /* DRIVERS_LABDAQ_CONFIG_H_ */
