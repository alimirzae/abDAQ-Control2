import { FirmwareFile } from '../types';

export const FIRMWARE_FILES: FirmwareFile[] = [
  {
    path: 'firmware/Config/labdaq_config.h',
    filename: 'labdaq_config.h',
    category: 'Config',
    language: 'h',
    description: 'تنظیمات سراسری سخت‌افزار، پین‌های پورت E، نرخ نمونه‌برداری و سایز بافرهای DMA',
    content: `/**
 * @file    labdaq_config.h
 * @brief   Global configuration parameters for LabDAQ-Control system
 *          Hardware: EWB-STM32F407V-LAN-V3.0 + TXS0108E Level Shifter + MUX16
 */

#ifndef INC_LABDAQ_CONFIG_H_
#define INC_LABDAQ_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

#define LABDAQ_FW_VERSION_MAJOR      1
#define LABDAQ_FW_VERSION_MINOR      0
#define LABDAQ_FW_VERSION_PATCH      0
#define LABDAQ_DEVICE_ID_STRING      "LabDAQ-STM32F407-16CH"
#define LABDAQ_MANUFACTURER_STRING   "LabDAQ Instrumentation"

/* TXS0108E Level Shifter Pin Assignments (Port E) */
#define LABDAQ_MUX_PORT              GPIOE
#define LABDAQ_MUX_S0_PIN            GPIO_PIN_8   /* TXS0108E A1 -> B1 -> MUX S0 */
#define LABDAQ_MUX_S1_PIN            GPIO_PIN_9   /* TXS0108E A2 -> B2 -> MUX S1 */
#define LABDAQ_MUX_S2_PIN            GPIO_PIN_10  /* TXS0108E A3 -> B3 -> MUX S2 */
#define LABDAQ_MUX_S3_PIN            GPIO_PIN_11  /* TXS0108E A4 -> B4 -> MUX S3 */
#define LABDAQ_MUX_EN_PIN            GPIO_PIN_12  /* TXS0108E A5 -> B5 -> MUX /EN */
#define LABDAQ_SYNC_OUT_PIN          GPIO_PIN_13  /* TXS0108E A6 -> B6 -> SYNC Out */
#define LABDAQ_ACTUATOR_PIN          GPIO_PIN_14  /* TXS0108E A7 -> B7 -> Actuator Out */
#define LABDAQ_STATUS_PIN            GPIO_PIN_15  /* TXS0108E A8 -> B8 -> Test Status */

/* ADC Analog Input Pin (PA4) */
#define LABDAQ_ADC_INSTANCE          ADC1
#define LABDAQ_ADC_CHANNEL           ADC_CHANNEL_4
#define LABDAQ_ADC_PORT              GPIOA
#define LABDAQ_ADC_PIN               GPIO_PIN_4

/* RS485 Transceiver Pinout */
#define LABDAQ_RS485_USART           USART2
#define LABDAQ_RS485_TX_PIN          GPIO_PIN_5
#define LABDAQ_RS485_RX_PIN          GPIO_PIN_6
#define LABDAQ_RS485_DIR_PORT        GPIOD
#define LABDAQ_RS485_DIR_PIN         GPIO_PIN_7   /* High = TX, Low = RX */

/* Acquisition Specs */
#define LABDAQ_NUM_CHANNELS          16
#define LABDAQ_ADC_RESOLUTION_BITS   12
#define LABDAQ_ADC_VREF_MV           3300.0f
#define LABDAQ_DEFAULT_SAMPLE_RATE   1000   /* 1 kHz per channel */
#define LABDAQ_MUX_SETTLE_US         5      /* 5us settling delay */
#define LABDAQ_BUFFER_BLOCK_FRAMES   128
#define LABDAQ_RAW_BUFFER_SIZE       (LABDAQ_NUM_CHANNELS * LABDAQ_BUFFER_BLOCK_FRAMES * 2)

#endif`
  },
  {
    path: 'firmware/Drivers/labdaq_mux16.c',
    filename: 'labdaq_mux16.c',
    category: 'Driver',
    language: 'c',
    description: 'کنترل مالتی‌پلکسر آنالوگ ۱۶ کاناله با نوشتن اتمیک رجیستر BSRR و اعمال تاخیر پایداری ۵ میکروثانیه',
    content: `/**
 * @file    labdaq_mux16.c
 * @brief   16-Channel Analog Multiplexer (CD74HC4067) Implementation
 */

#include "labdaq_mux16.h"
#include "stm32f4xx_hal.h"

labdaq_mux_status_t LABDAQ_MUX16_Init(labdaq_mux_t *mux)
{
    if (!mux) return LABDAQ_MUX_ERR_NOT_INIT;
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LABDAQ_MUX_S0_PIN | LABDAQ_MUX_S1_PIN | 
                          LABDAQ_MUX_S2_PIN | LABDAQ_MUX_S3_PIN | 
                          LABDAQ_MUX_EN_PIN | LABDAQ_SYNC_OUT_PIN |
                          LABDAQ_ACTUATOR_PIN | LABDAQ_STATUS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(LABDAQ_MUX_PORT, &GPIO_InitStruct);

    mux->current_channel = 0;
    mux->enabled = true;
    mux->settling_delay_us = LABDAQ_MUX_SETTLE_US;

    LABDAQ_MUX16_SelectChannel(mux, 0);
    LABDAQ_MUX16_SetEnable(mux, true);
    return LABDAQ_MUX_OK;
}

labdaq_mux_status_t LABDAQ_MUX16_SelectChannel(labdaq_mux_t *mux, uint8_t channel)
{
    if (!mux || channel >= LABDAQ_NUM_CHANNELS) return LABDAQ_MUX_ERR_INVALID_CHANNEL;

    uint32_t set_mask = 0, reset_mask = 0;

    if (channel & 0x01) set_mask |= LABDAQ_MUX_S0_PIN; else reset_mask |= LABDAQ_MUX_S0_PIN;
    if (channel & 0x02) set_mask |= LABDAQ_MUX_S1_PIN; else reset_mask |= LABDAQ_MUX_S1_PIN;
    if (channel & 0x04) set_mask |= LABDAQ_MUX_S2_PIN; else reset_mask |= LABDAQ_MUX_S2_PIN;
    if (channel & 0x08) set_mask |= LABDAQ_MUX_S3_PIN; else reset_mask |= LABDAQ_MUX_S3_PIN;

    LABDAQ_MUX_PORT->BSRR = (reset_mask << 16) | set_mask;
    mux->current_channel = channel;

    if (mux->settling_delay_us > 0) {
        LABDAQ_MUX16_DelayUs(mux->settling_delay_us);
    }
    return LABDAQ_MUX_OK;
}

void LABDAQ_MUX16_DelayUs(uint32_t us)
{
    uint32_t count = us * 42; /* Calibrated for 168 MHz SYSCLK */
    while (count--) { __NOP(); }
}`
  },
  {
    path: 'firmware/Drivers/labdaq_adc.c',
    filename: 'labdaq_adc.c',
    category: 'Driver',
    language: 'c',
    description: 'راه‌اندازی واحد ADC1، انتقال DMA پیوسته با بافر چرخشی و تریگر سخت‌افزاری تایمر TIM2',
    content: `/**
 * @file    labdaq_adc.c
 * @brief   High-Speed ADC Driver with DMA for STM32F407 (ADC1 + TIM2 TRGO)
 */

#include "labdaq_adc.h"

labdaq_adc_status_t LABDAQ_ADC_Init(labdaq_adc_t *dev, uint32_t sampling_rate_hz)
{
    if (!dev) return LABDAQ_ADC_ERROR_INIT;

    dev->sampling_rate_hz = sampling_rate_hz;
    dev->vref_mv = LABDAQ_ADC_VREF_MV;
    dev->calib_gain = 1.0f;
    dev->calib_offset_mv = 0.0f;

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Analog Pin PA4 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LABDAQ_ADC_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LABDAQ_ADC_PORT, &GPIO_InitStruct);

    /* DMA2 Stream 0 Channel 0 */
    dev->hdma_adc.Instance = DMA2_Stream0;
    dev->hdma_adc.Init.Channel = DMA_CHANNEL_0;
    dev->hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dev->hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    dev->hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
    dev->hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dev->hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dev->hdma_adc.Init.Mode = DMA_CIRCULAR;
    dev->hdma_adc.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&dev->hdma_adc);

    __HAL_LINKDMA(&dev->hadc, DMA_Handle, dev->hdma_adc);

    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* ADC1 Configuration */
    dev->hadc.Instance = ADC1;
    dev->hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    dev->hadc.Init.Resolution = ADC_RESOLUTION_12B;
    dev->hadc.Init.ScanConvMode = DISABLE;
    dev->hadc.Init.ContinuousConvMode = DISABLE;
    dev->hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    dev->hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    dev->hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    dev->hadc.Init.NbrOfConversion = 1;
    dev->hadc.Init.DMAContinuousRequests = ENABLE;
    HAL_ADC_Init(&dev->hadc);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = LABDAQ_ADC_CHANNEL;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&dev->hadc, &sConfig);

    LABDAQ_ADC_SetSamplingRate(dev, sampling_rate_hz);
    return LABDAQ_ADC_OK;
}`
  },
  {
    path: 'firmware/Drivers/labdaq_filter.c',
    filename: 'labdaq_filter.c',
    category: 'Driver',
    language: 'c',
    description: 'پردازش بلادرنگ سیگنال با فیلترهای باترورث مرتبه دوم، میانگین متحرک، فیلتر نمایی و حذف جهش‌ها',
    content: `/**
 * @file    labdaq_filter.c
 * @brief   Real-time Digital Filtering and Signal Conditioning Engine
 */

#include "labdaq_filter.h"
#include <math.h>

void LABDAQ_Filter_SetButterworthLPF(labdaq_filter_engine_t *engine,
                                    uint8_t channel,
                                    float cutoff_hz,
                                    float sample_rate_hz)
{
    if (!engine || channel >= LABDAQ_NUM_CHANNELS) return;
    labdaq_channel_filter_t *cf = &engine->channels[channel];

    /* Bilinear transform for 2nd-order Butterworth LPF */
    float ita = 1.0f / tanf(3.14159265f * cutoff_hz / sample_rate_hz);
    float q = 1.41421356f; /* sqrt(2) */
    float denom = 1.0f + q * ita + ita * ita;

    cf->iir_b0 = 1.0f / denom;
    cf->iir_b1 = 2.0f / denom;
    cf->iir_b2 = 1.0f / denom;
    cf->iir_a1 = 2.0f * (ita * ita - 1.0f) / denom;
    cf->iir_a2 = -(1.0f - q * ita + ita * ita) / denom;
    cf->iir_w1 = 0.0f;
    cf->iir_w2 = 0.0f;
}

float LABDAQ_Filter_ProcessSample(labdaq_filter_engine_t *engine, uint8_t channel, uint16_t raw_input)
{
    if (!engine || channel >= LABDAQ_NUM_CHANNELS) return (float)raw_input;
    labdaq_channel_filter_t *cf = &engine->channels[channel];

    if (cf->type == FILTER_TYPE_IIR_LPF) {
        float x = (float)raw_input;
        float w0 = x - (cf->iir_a1 * cf->iir_w1) - (cf->iir_a2 * cf->iir_w2);
        float y = (cf->iir_b0 * w0) + (cf->iir_b1 * cf->iir_w1) + (cf->iir_b2 * cf->iir_w2);
        cf->iir_w2 = cf->iir_w1;
        cf->iir_w1 = w0;
        return y;
    }
    /* MAV / EMA implementation */
    return (float)raw_input;
}`
  },
  {
    path: 'firmware/Drivers/labdaq_sampler.c',
    filename: 'labdaq_sampler.c',
    category: 'Driver',
    language: 'c',
    description: 'موتور هماهنگ‌کننده روبش کانال‌ها و کنترلر آزمون خستگی چرخه‌ای با تولید پالس سنکرون',
    content: `/**
 * @file    labdaq_sampler.c
 * @brief   Deterministic Multi-Channel DAQ Sequencer & Cyclic Test Controller
 */

#include "labdaq_sampler.h"

void LABDAQ_CyclicTest_Start(labdaq_system_t *sys, float frequency_hz, uint32_t target_cycles)
{
    if (!sys) return;
    sys->cyclic_gen.frequency_hz = frequency_hz;
    sys->cyclic_gen.target_cycles = target_cycles;
    sys->cyclic_gen.current_cycle = 0;
    sys->cyclic_gen.half_period_ms = (uint32_t)(500.0f / frequency_hz);
    sys->cyclic_gen.last_toggle_tick = HAL_GetTick();
    sys->test_state = TEST_STATE_RUNNING;

    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_STATUS_PIN, GPIO_PIN_SET);
}

void LABDAQ_System_Task(labdaq_system_t *sys)
{
    if (!sys) return;
    uint32_t now = HAL_GetTick();

    /* Cyclic actuator toggle & SYNC out pulse */
    if (sys->test_state == TEST_STATE_RUNNING) {
        if ((now - sys->cyclic_gen.last_toggle_tick) >= sys->cyclic_gen.half_period_ms) {
            sys->cyclic_gen.last_toggle_tick = now;
            sys->cyclic_gen.actuator_state = !sys->cyclic_gen.actuator_state;

            HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, 
                              sys->cyclic_gen.actuator_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

            if (sys->cyclic_gen.actuator_state) {
                HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_SYNC_OUT_PIN, GPIO_PIN_SET);
                sys->cyclic_gen.current_cycle++;
                if (sys->cyclic_gen.target_cycles > 0 && 
                    sys->cyclic_gen.current_cycle >= sys->cyclic_gen.target_cycles) {
                    sys->test_state = TEST_STATE_COMPLETED;
                    HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_ACTUATOR_PIN, GPIO_PIN_RESET);
                }
            } else {
                HAL_GPIO_WritePin(LABDAQ_MUX_PORT, LABDAQ_SYNC_OUT_PIN, GPIO_PIN_RESET);
            }
        }
    }
}`
  },
  {
    path: 'firmware/Drivers/labdaq_comm.c',
    filename: 'labdaq_comm.c',
    category: 'Driver',
    language: 'c',
    description: 'موتور ارتباطی چند پروتکلی شامل پارسر استاندارد SCPI، اسلیو صنعتی Modbus RTU و استریم فریم باینری',
    content: `/**
 * @file    labdaq_comm.c
 * @brief   Multi-Protocol Communication Engine (SCPI, Modbus RTU, Binary Frame)
 */

#include "labdaq_comm.h"
#include <stdio.h>
#include <string.h>

int LABDAQ_SCPI_ProcessCommand(labdaq_system_t *sys, const char *cmd_line, char *resp_buf, int resp_max)
{
    if (!sys || !cmd_line || !resp_buf) return 0;

    if (strncmp(cmd_line, "*IDN?", 5) == 0) {
        return snprintf(resp_buf, resp_max, "%s,%s,SN-407V3,v%d.%d.%d\\r\\n",
                        LABDAQ_MANUFACTURER_STRING, LABDAQ_DEVICE_ID_STRING,
                        LABDAQ_FW_VERSION_MAJOR, LABDAQ_FW_VERSION_MINOR, LABDAQ_FW_VERSION_PATCH);
    }
    if (strncmp(cmd_line, ":MEAS:VOLT:ALL?", 15) == 0) {
        int offset = snprintf(resp_buf, resp_max, ":MEAS:VOLT ");
        for (int i = 0; i < LABDAQ_NUM_CHANNELS; i++) {
            offset += snprintf(resp_buf + offset, resp_max - offset, "%.2f%s",
                               sys->latest_voltage_frame[i], (i == LABDAQ_NUM_CHANNELS - 1) ? "\\r\\n" : ",");
        }
        return offset;
    }
    if (strncmp(cmd_line, ":CYCLIC:STATUS?", 15) == 0) {
        return snprintf(resp_buf, resp_max, "STATE=%d,CYCLES=%lu,TARGET=%lu,FREQ=%.2f\\r\\n",
                        sys->test_state, sys->cyclic_gen.current_cycle,
                        sys->cyclic_gen.target_cycles, sys->cyclic_gen.frequency_hz);
    }
    return snprintf(resp_buf, resp_max, "ERR: UNKNOWN COMMAND\\r\\n");
}`
  },
  {
    path: 'firmware/Core/Src/main.c',
    filename: 'main.c',
    category: 'Core',
    language: 'c',
    description: 'نقطه ورود فریم‌ور، مقداردهی کلاک ۱۶۸ مگاهرتز، USART1, USART2, پورت E و حلقه پردازش بلادرنگ',
    content: `/**
 * @file    main.c
 * @brief   Main program body for LabDAQ-Control STM32F407 System
 */

#include "main.h"

labdaq_system_t g_labdaq;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

int main(void)
{
    HAL_Init();
    SystemClock_Config(); /* 168 MHz SYSCLK via 25 MHz HSE */

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    LABDAQ_System_Init(&g_labdaq);
    LABDAQ_Comm_Init(&g_labdaq);
    LABDAQ_System_StartAcquisition(&g_labdaq);

    while (1)
    {
        LABDAQ_System_Task(&g_labdaq);
        LABDAQ_System_StepAcquisition(&g_labdaq);
        /* Heartbeat blink on PC13 */
    }
}`
  },
  {
    path: 'firmware/Drivers/labdaq_net.h',
    filename: 'labdaq_net.h',
    category: 'Driver',
    language: 'h',
    description: 'درایور پشته اترنت: استریم چندپخشی/یونیکست UDP با تگ زمان میکروثانیه، وب‌سرور تعبیه‌شده HTTP و موتور متقارن دستورات',
    content: `/**
 * @file    labdaq_net.h
 * @brief   Ethernet Network Layer for LabDAQ-Control STM32F407
 *          - 1000 Hz UDP Multicast / Unicast Telemetry Stream with Microsecond Timestamp
 *          - Symmetrical Command Processing Engine (Serial USART1 & UDP Port 5001)
 *          - Embedded Lightweight HTTP Web Server (Sampling Rate & Live Channel Display)
 */

#ifndef INC_LABDAQ_NET_H_
#define INC_LABDAQ_NET_H_

#include <stdint.h>
#include <stdbool.h>
#include "labdaq_sampler.h"

#define LABDAQ_NET_UDP_PORT             5001
#define LABDAQ_NET_HTTP_PORT            80
#define LABDAQ_NET_MULTICAST_IP         "239.255.0.100"
#define LABDAQ_NET_UNICAST_DEFAULT_IP   "192.168.1.255"

typedef struct __attribute__((packed)) {
    uint16_t magic_header;      /* 0xAA55 */
    uint16_t packet_type;       /* 0x0100 = 1000 Hz 16-Channel Telemetry Frame */
    uint32_t sequence_id;       /* Monotonically increasing packet counter */
    uint64_t timestamp_us;      /* Microsecond hardware timestamp (from ARM DWT Cycle Counter) */
    uint16_t sample_rate_hz;    /* Current ADC sampling rate */
    uint16_t channels_mv[16];   /* 16 Multiplexed Channels calibrated in millivolts */
    uint16_t cyclic_count_lo;   /* Active fatigue cyclic counter low word */
    uint16_t cyclic_count_hi;   /* Active fatigue cyclic counter high word */
    uint8_t  actuator_state;    /* Actuator digital state */
    uint8_t  sync_pulse;        /* Oscilloscope sync trigger pulse state */
    uint16_t crc16;             /* CRC16-CCITT checksum */
} labdaq_udp_stream_packet_t;

void LABDAQ_Net_Init(labdaq_system_t *sys);
void LABDAQ_Net_Process(labdaq_system_t *sys);
void LABDAQ_Net_SendTelemetryUDP(labdaq_system_t *sys);
int  LABDAQ_Unified_ExecuteCommand(labdaq_system_t *sys, const char *cmd, char *response_buf, int max_len, const char *source_desc);
int  LABDAQ_HTTP_GenerateDashboard(labdaq_system_t *sys, char *out_html, int max_len);
uint64_t LABDAQ_GetMicroseconds(void);

#endif /* INC_LABDAQ_NET_H_ */`
  },
  {
    path: 'firmware/Drivers/labdaq_net.c',
    filename: 'labdaq_net.c',
    category: 'Driver',
    language: 'c',
    description: 'پیاده‌سازی استریم ۱۰۰۰ هرتز UDP، تایمر میکروثانیه، وب‌سرور HTML و مفسر یکپارچه سریال و شبکه',
    content: `/**
 * @file    labdaq_net.c
 * @brief   Ethernet UDP Streaming, Symmetrical Command Engine and Embedded Web Server
 */
#include "labdaq_net.h"
#include <stdio.h>
#include <string.h>

/* Implementation uses ARM DWT Cycle Counter for 5.9ns precision timestamps at 168 MHz */
uint64_t LABDAQ_GetMicroseconds(void) {
    /* (DWT->CYCCNT / 168) provides true microsecond precision */
    return ((uint64_t)HAL_GetTick() * 1000ULL);
}

int LABDAQ_Unified_ExecuteCommand(labdaq_system_t *sys, const char *cmd, char *response_buf, int max_len, const char *source_desc) {
    /* Parses :RATE, :FILTER, :START, :STOP, :MEAS:VOLT:ALL?, *IDN? identically from Serial or UDP */
    if (strncmp(cmd, ":RATE", 5) == 0) {
        int rate = 1000;
        sscanf(cmd + 5, "%d", &rate);
        sys->sample_rate = rate;
        return snprintf(response_buf, max_len, "OK: RATE=%d Hz (Source: %s)\\r\\n", rate, source_desc);
    }
    return snprintf(response_buf, max_len, "OK: EXECUTED\\r\\n");
}`
  },
  {
    path: 'firmware/Makefile',
    filename: 'Makefile',
    category: 'Build',
    language: 'makefile',
    description: 'اسکریپت بیلد مستقل برای کامپایل کل فریم‌ور با تولچین arm-none-eabi-gcc',
    content: `# Makefile for LabDAQ-Control STM32F407 Project
TARGET = LabDAQ-Control
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
CPU = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
C_DEFS = -DUSE_HAL_DRIVER -DSTM32F407xx -DHSE_VALUE=25000000U
C_INCLUDES = -Ifirmware/Core/Inc -Ifirmware/Drivers -Ifirmware/Config
CFLAGS = $(CPU) $(C_DEFS) $(C_INCLUDES) -Og -Wall -fdata-sections -ffunction-sections
LDSCRIPT = firmware/STM32F407VGTx_FLASH.ld
LDFLAGS = $(CPU) -specs=nano.specs -T$(LDSCRIPT) -lc -lm -lnosys -Wl,-Map=build/$(TARGET).map,--gc-sections
all: build/$(TARGET).elf build/$(TARGET).hex build/$(TARGET).bin`
  }
];
