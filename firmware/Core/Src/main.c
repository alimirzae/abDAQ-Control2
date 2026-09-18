/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body for LabDAQ-Control STM32F407 System
 *                   EWB-STM32F407V-LAN-V3.0 + TXS0108E Level Shifter + MUX16
 ******************************************************************************
 */

#if __has_include("main.h")
#include "main.h"
#elif __has_include("../Inc/main.h")
#include "../Inc/main.h"
#elif __has_include("../../Core/Inc/main.h")
#include "../../Core/Inc/main.h"
#endif
#include <stdio.h>
#include <string.h>
#include "labdaq_control.h"

/* Global Master System Instance */
labdaq_system_t g_labdaq;

/* UART Handles for SCPI (USART1) and RS485 Modbus (USART2) */
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Reception ring buffers */
static char scpi_rx_line[256];
static uint16_t scpi_rx_idx = 0;
static char scpi_tx_buf[512];

static uint8_t modbus_rx_buf[256];
static uint16_t modbus_rx_idx = 0;
static uint8_t modbus_tx_buf[256];

static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock (168 MHz from 25 MHz HSE Crystal) */
    SystemClock_Config();

    /* Initialize Peripherals and GPIOs */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* Initialize LabDAQ Master System (MUX, ADC, Filters, Ping-Pong Buffers, Timer) */
    if (!LABDAQ_System_Init(&g_labdaq)) {
        Error_Handler();
    }

    /* Initialize Communication Interfaces (RS485 DIR, etc.) */
    LABDAQ_Comm_Init(&g_labdaq);
    LABDAQ_Control_Init();

    /* Initialize Ethernet Network Stack (UDP Multicast/Unicast & Embedded HTTP Web Server) */
    LABDAQ_Net_Init(&g_labdaq);

    /* Send Boot Banner via USART1 (CP2104 USB-to-UART) */
    const char *boot_msg = "\r\n=========================================\r\n"
                           "  LabDAQ-Control STM32F407 System v1.0.0 \r\n"
                           "  16-Ch High-Speed DAQ & Cyclic Controller\r\n"
                           "  UDP Multicast: 239.255.0.100:5001 (1 kSPS)\r\n"
                           "  HTTP Web Server: http://192.168.1.150:80 \r\n"
                           "  Dual SCPI Engine: Serial + UDP:5001     \r\n"
                           "=========================================\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)boot_msg, strlen(boot_msg), 100);

    /* Start Continuous Acquisition by default */
    LABDAQ_System_StartAcquisition(&g_labdaq);

    uint32_t last_status_blink = 0;
    uint32_t last_sync_step = 0;
    uint32_t last_udp_telemetry_tick = 0;

    /* Main Superloop */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* 1. Periodic DAQ Task & Cyclic Test Controller update */
        LABDAQ_System_Task(&g_labdaq);
        LABDAQ_Control_Task(&g_labdaq);

        /* 2. Step round-robin analog scanning if running in software-stepped mode */
        if (now - last_sync_step >= 1) {
            last_sync_step = now;
            LABDAQ_System_StepAcquisition(&g_labdaq);
        }

        /* 3. Automatic 1000 Hz UDP Telemetry Streaming with microsecond timestamps */
        if (g_labdaq.streaming_active && (now - last_udp_telemetry_tick >= 1)) {
            last_udp_telemetry_tick = now;
            LABDAQ_Net_SendTelemetryUDP(&g_labdaq);
        }

        /* 4. Process Network Stack (LwIP, incoming UDP packets & HTTP requests) */
        LABDAQ_Net_Process(&g_labdaq);

        /* 5. Handle incoming commands over USART1 (Unified Engine) */
        uint8_t ch = 0;
        if (HAL_UART_Receive(&huart1, &ch, 1, 0) == HAL_OK) {
            if (ch == '\r' || ch == '\n') {
                if (scpi_rx_idx > 0) {
                    scpi_rx_line[scpi_rx_idx] = '\0';
                    int resp_len = LABDAQ_Unified_ExecuteCommand(&g_labdaq, scpi_rx_line, scpi_tx_buf, sizeof(scpi_tx_buf), "SERIAL_UART1");
                    if (resp_len > 0) {
                        HAL_UART_Transmit(&huart1, (uint8_t *)scpi_tx_buf, resp_len, 100);
                    }
                    scpi_rx_idx = 0;
                }
            } else if (scpi_rx_idx < sizeof(scpi_rx_line) - 1) {
                scpi_rx_line[scpi_rx_idx++] = (char)ch;
            }
        }

        /* 4. Handle Modbus RTU over RS485 (USART2) */
        if (HAL_UART_Receive(&huart2, &ch, 1, 0) == HAL_OK) {
            if (modbus_rx_idx < sizeof(modbus_rx_buf)) {
                modbus_rx_buf[modbus_rx_idx++] = ch;
            }
        } else if (modbus_rx_idx >= 4) {
            /* Frame idle timeout detected -> process Modbus frame */
            int tx_len = LABDAQ_Modbus_ProcessFrame(&g_labdaq, modbus_rx_buf, modbus_rx_idx, modbus_tx_buf);
            if (tx_len > 0) {
                /* Enable RS485 Transmitter (DIR High) */
                HAL_GPIO_WritePin(LABDAQ_RS485_DIR_PORT, LABDAQ_RS485_DIR_PIN, GPIO_PIN_SET);
                HAL_UART_Transmit(&huart2, modbus_tx_buf, tx_len, 50);
                /* Back to Receiver (DIR Low) */
                HAL_GPIO_WritePin(LABDAQ_RS485_DIR_PORT, LABDAQ_RS485_DIR_PIN, GPIO_PIN_RESET);
            }
            modbus_rx_idx = 0;
        }

        /* 5. Heartbeat LED Blink (every 500ms) */
        if (now - last_status_blink >= 500) {
            last_status_blink = now;
            HAL_GPIO_TogglePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN);
        }
    }
}

/**
  * @brief System Clock Configuration
  *        System Clock source = PLL (HSE 25 MHz)
  *        SYSCLK(Hz)          = 168000000
  *        HCLK(Hz)            = 168000000
  *        AHB Prescaler       = 1
  *        APB1 Prescaler      = 4 (42 MHz / 84 MHz Timer)
  *        APB2 Prescaler      = 2 (84 MHz / 168 MHz Timer)
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9 -> USART1_TX, PA10 -> USART1_RX */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PD5 -> USART2_TX, PD6 -> USART2_RX */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = LABDAQ_MODBUS_BAUDRATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED1 on PC13 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LABDAQ_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LABDAQ_LED_PORT, &GPIO_InitStruct);

    /* User Button on PA0 */
    GPIO_InitStruct.Pin = LABDAQ_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(LABDAQ_BTN_PORT, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* Rapid blink error LED */
        HAL_GPIO_TogglePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN);
        for (volatile int i = 0; i < 500000; i++);
    }
}
