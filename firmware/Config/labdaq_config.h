/**
 ******************************************************************************
 * @file    labdaq_config.h
 * @brief   Global configuration parameters for LabDAQ-Control system
 *          Hardware: EWB-STM32F407V-LAN-V3.0 + TXS0108E Level Shifter + MUX16
 ******************************************************************************
 */

#ifndef INC_LABDAQ_CONFIG_H_
#define INC_LABDAQ_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* System & Firmware Metadata                                                */
/* ========================================================================== */
#define LABDAQ_FW_VERSION_MAJOR      1
#define LABDAQ_FW_VERSION_MINOR      0
#define LABDAQ_FW_VERSION_PATCH      0
#define LABDAQ_DEVICE_ID_STRING      "LabDAQ-STM32F407-16CH"
#define LABDAQ_MANUFACTURER_STRING   "LabDAQ Instrumentation"

/* ========================================================================== */
/* Hardware Pin Mapping (EWB-STM32F407V-LAN-V3.0 + TXS0108E Level Shifter)   */
/* ========================================================================== */

/* TXS0108E Level Shifter is connected to Header J1 (Port E high-byte)       */
/* Port E Pin 8..15 drive 3.3V logic -> shifted to 5.0V for external MUX/IO  */

/* 16-Channel Analog Multiplexer (e.g. CD74HC4067) Address & Enable Lines   */
#define LABDAQ_MUX_PORT              GPIOE
#define LABDAQ_MUX_S0_PIN            GPIO_PIN_8   /* TXS0108E Channel A1 -> B1 -> MUX S0 */
#define LABDAQ_MUX_S1_PIN            GPIO_PIN_9   /* TXS0108E Channel A2 -> B2 -> MUX S1 */
#define LABDAQ_MUX_S2_PIN            GPIO_PIN_10  /* TXS0108E Channel A3 -> B3 -> MUX S2 */
#define LABDAQ_MUX_S3_PIN            GPIO_PIN_11  /* TXS0108E Channel A4 -> B4 -> MUX S3 */
#define LABDAQ_MUX_EN_PIN            GPIO_PIN_12  /* TXS0108E Channel A5 -> B5 -> MUX /EN (Active LOW) */

/* Cyclic Test Actuator & Trigger Synchronization Digital Outputs            */
#define LABDAQ_SYNC_OUT_PIN          GPIO_PIN_13  /* TXS0108E Channel A6 -> B6 -> SYNC Out */
#define LABDAQ_ACTUATOR_PIN          GPIO_PIN_14  /* TXS0108E Channel A7 -> B7 -> Actuator / Valve */
#define LABDAQ_STATUS_PIN            GPIO_PIN_15  /* TXS0108E Channel A8 -> B8 -> Test Active Status */

/* ADC Analog Input Pin: MUX Common Signal (SIG) Output -> STM32 PA4 (ADC1_IN4) */
#define LABDAQ_ADC_INSTANCE          ADC1
#define LABDAQ_ADC_CHANNEL           ADC_CHANNEL_4
#define LABDAQ_ADC_PORT              GPIOA
#define LABDAQ_ADC_PIN               GPIO_PIN_4

/* Direct Secondary Analog Inputs (Direct High-Speed without MUX delay)       */
#define LABDAQ_DIRECT_ADC_CH1        ADC_CHANNEL_3  /* PA3: Fast Load Cell / Force */
#define LABDAQ_DIRECT_ADC_CH2        ADC_CHANNEL_5  /* PA5: Linear Displacement LVDT */

/* On-board User LED and Pushbutton                                          */
#define LABDAQ_LED_PORT              GPIOC
#define LABDAQ_LED_PIN               GPIO_PIN_13
#define LABDAQ_BTN_PORT              GPIOA
#define LABDAQ_BTN_PIN               GPIO_PIN_0    /* PA0 WAKEUP */

/* RS485 Transceiver (SP3485) Pinout                                         */
#define LABDAQ_RS485_USART           USART2
#define LABDAQ_RS485_PORT            GPIOD
#define LABDAQ_RS485_TX_PIN          GPIO_PIN_5
#define LABDAQ_RS485_RX_PIN          GPIO_PIN_6
#define LABDAQ_RS485_DIR_PORT        GPIOD
#define LABDAQ_RS485_DIR_PIN         GPIO_PIN_7   /* High = TX, Low = RX */

/* CAN Transceiver (SN65HVD230) Pinout                                       */
#define LABDAQ_CAN_INSTANCE          CAN1
#define LABDAQ_CAN_PORT              GPIOB
#define LABDAQ_CAN_RX_PIN            GPIO_PIN_8
#define LABDAQ_CAN_TX_PIN            GPIO_PIN_9

/* Ethernet LAN8720A RMII Interface                                          */
/* MDIO: PA2, MDC: PC1, RXD0: PC4, RXD1: PC5, CRS_DV: PA7,                  */
/* TX_EN: PB11, TXD0: PB12, TXD1: PB13, REF_CLK: PA1 (50MHz)                 */
#define LABDAQ_ETH_TCP_PORT          5000
#define LABDAQ_ETH_UDP_PORT          5001
#define LABDAQ_DEFAULT_IP_ADDR       "192.168.1.150"
#define LABDAQ_DEFAULT_NETMASK       "255.255.255.0"
#define LABDAQ_DEFAULT_GATEWAY       "192.168.1.1"

/* ========================================================================== */
/* DAQ Acquisition Architecture & Buffer Sizing                               */
/* ========================================================================== */
#define LABDAQ_NUM_CHANNELS          16
#define LABDAQ_ADC_RESOLUTION_BITS   12
#define LABDAQ_ADC_VREF_MV           3300.0f

/* Default Sampling Rates                                                     */
#define LABDAQ_DEFAULT_SAMPLE_RATE   1000   /* 1 kHz per channel default (16k total) */
#define LABDAQ_MAX_SAMPLE_RATE       10000  /* Up to 10 kHz per channel */
#define LABDAQ_MIN_SAMPLE_RATE       10     /* 10 Hz minimum */

/* MUX Settling Time in microseconds (CD74HC4067 + TXS0108E delay)           */
#define LABDAQ_MUX_SETTLE_US         5      /* 5us allows high impedance RC to settle */

/* DMA Ping-Pong Circular Buffer Sizes                                        */
/* Each buffer block contains N frames. 1 frame = 16 channel readings.       */
#define LABDAQ_BUFFER_BLOCK_FRAMES   128
#define LABDAQ_RAW_BUFFER_SIZE       (LABDAQ_NUM_CHANNELS * LABDAQ_BUFFER_BLOCK_FRAMES * 2)

/* ========================================================================== */
/* Digital Filter Defaults                                                    */
/* ========================================================================== */
#define LABDAQ_FILTER_MAX_WINDOW     64
#define LABDAQ_DEFAULT_MAV_WINDOW    8
#define LABDAQ_DEFAULT_EMA_ALPHA     0.25f  /* Exponential Moving Average weight */

/* ========================================================================== */
/* Cyclic Test Profile Configuration                                          */
/* ========================================================================== */
#define LABDAQ_CYCLIC_DEFAULT_FREQ   1.0f   /* 1.0 Hz default cyclic excitation */
#define LABDAQ_CYCLIC_MAX_FREQ       50.0f  /* 50 Hz max mechanical cycle */
#define LABDAQ_CYCLIC_DEFAULT_CYCLES 10000  /* 10,000 cycles target */

/* ========================================================================== */
/* Modbus RTU Configuration                                                   */
/* ========================================================================== */
#define LABDAQ_MODBUS_DEFAULT_ADDR   1
#define LABDAQ_MODBUS_BAUDRATE       115200

#ifdef __cplusplus
}
#endif

#endif /* INC_LABDAQ_CONFIG_H_ */
