/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body for LabDAQ-Control STM32F407 System
 *                   EWB-STM32F407 Rev2.0 + 4.0-inch SPI LCD + external DAQ I/O
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
#include "labdaq_display.h"
#include "labdaq_heartbeat.h"

/* Global Master System Instance */
labdaq_system_t g_labdaq;

/* UART Handles for SCPI (USART1) and RS485 Modbus (USART2) */
UART_HandleTypeDef huart1 = {0};
UART_HandleTypeDef huart2 = {0};

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

/* Debugger-visible boot breadcrumbs.
 * Watch these in STM32CubeIDE Expressions/Live Expressions while single-stepping.
 * They require no UART/semihosting and therefore work with an ordinary ST-LINK SWD probe.
 */
volatile uint32_t g_debug_stage = 0U;
volatile uint32_t g_debug_error = 0U;
volatile uint32_t g_debug_hal_tick = 0U;

#define DEBUG_STAGE(n) do { g_debug_stage = (n); g_debug_hal_tick = HAL_GetTick(); __DSB(); } while (0)

int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    g_debug_stage = 1U; /* entered main */
    HAL_Init();
    DEBUG_STAGE(2U);     /* HAL/SysTick initialized */

    /* Configure the board-reference bring-up clock: direct 16 MHz HSI, no PLL/HSE. */
    SystemClock_Config();
    DEBUG_STAGE(3U);     /* 168 MHz clock configured */

    /* Initialize Peripherals and GPIOs */
    MX_GPIO_Init();
    DEBUG_STAGE(4U);     /* GPIO initialized */

    /* Rev2 power-on self-test:
     * - LED1 is PB1, active LOW, and blinks five times immediately.
     * - LCD backlight is PB0 and remains OFF for the first second.
     * This makes a newly flashed image visually distinguishable from power-only behavior.
     */
    HAL_GPIO_WritePin(LABDAQ_LCD_BL_PORT, LABDAQ_LCD_BL_PIN, GPIO_PIN_RESET);
    LABDAQ_Heartbeat_Init();
    DEBUG_STAGE(5U);     /* heartbeat/LED initialized */
    MX_USART1_UART_Init();
    DEBUG_STAGE(6U);     /* USART1 initialized */

    const char *early_boot = "\r\n[BOOT] EWB-STM32F407 Rev2 main() reached @ 115200 8N1\r\n";
    HAL_StatusTypeDef uart_boot_status = HAL_UART_Transmit(&huart1, (uint8_t *)early_boot, strlen(early_boot), 100);
    g_debug_error = (uint32_t)uart_boot_status;
    DEBUG_STAGE(7U);     /* first UART transmit attempted */

    for (uint8_t i = 0; i < 5U; ++i) {
        HAL_GPIO_WritePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN, GPIO_PIN_SET);
        HAL_Delay(100);
    }
    DEBUG_STAGE(8U);     /* PB1 five-blink test completed */
    HAL_GPIO_WritePin(LABDAQ_LCD_BL_PORT, LABDAQ_LCD_BL_PIN, GPIO_PIN_SET);
    DEBUG_STAGE(9U);     /* LCD backlight asserted */

    LABDAQ_Heartbeat_SetState(LABDAQ_HB_INIT);
    MX_USART2_UART_Init();
    DEBUG_STAGE(10U);    /* USART2 initialized */

    /* Bring up the local 4-inch LCD before DAQ init so boot/fault status is visible. */
    DEBUG_STAGE(11U);    /* entering LCD init */
    LABDAQ_Display_Init();
    DEBUG_STAGE(12U);    /* LCD init returned */

    /* Initialize LabDAQ Master System (MUX, ADC, Filters, Ping-Pong Buffers, Timer) */
    DEBUG_STAGE(13U);    /* entering DAQ init */
    if (!LABDAQ_System_Init(&g_labdaq)) {
        g_debug_error = 0xDA01U;
        Error_Handler();
    }
    DEBUG_STAGE(14U);    /* DAQ init returned */

    /* Initialize Communication Interfaces (RS485 DIR, etc.) */
    LABDAQ_Comm_Init(&g_labdaq);
    LABDAQ_Control_Init();

    /* Initialize Ethernet Network Stack (UDP Multicast/Unicast & Embedded HTTP Web Server) */
    DEBUG_STAGE(15U);
    LABDAQ_Net_Init(&g_labdaq);
    DEBUG_STAGE(16U);    /* network init returned */

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
    DEBUG_STAGE(17U);    /* acquisition started / entering superloop */
    LABDAQ_Heartbeat_SetState(LABDAQ_HB_ACQUIRING);

    uint32_t last_sync_step = 0;
    uint32_t last_udp_telemetry_tick = 0;

    /* Main Superloop */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* 1. Periodic DAQ Task & Cyclic Test Controller update */
        LABDAQ_System_Task(&g_labdaq);
        LABDAQ_Control_Task(&g_labdaq);
        LABDAQ_Display_Task(&g_labdaq);

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

        /* Local HMI PAGE button: PA0, debounced in display module. */
        static GPIO_PinState last_page_button = GPIO_PIN_SET;
        GPIO_PinState page_button = HAL_GPIO_ReadPin(LABDAQ_BTN_PORT, LABDAQ_BTN_PIN);
        if (page_button == GPIO_PIN_RESET && last_page_button == GPIO_PIN_SET) {
            LABDAQ_Display_ButtonEvent();
        }
        last_page_button = page_button;

        /* 6. Non-blocking staged heartbeat: LED speed identifies operating state. */
        if (g_labdaq.test_state == TEST_STATE_RUNNING) {
            LABDAQ_Heartbeat_SetState(LABDAQ_HB_TEST_RUNNING);
        } else if (g_labdaq.streaming_active) {
            LABDAQ_Heartbeat_SetState(LABDAQ_HB_ACQUIRING);
        } else {
            LABDAQ_Heartbeat_SetState(LABDAQ_HB_READY);
        }
        LABDAQ_Heartbeat_Task(now);
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
    /* Board reference CubeMX project (blink1.ioc) runs SYSCLK directly
     * from the internal 16 MHz HSI. For hardware bring-up, intentionally
     * do not enable PLL/HSE here. This matches the known-good board sample
     * and removes oscillator/PLL switching from the boot path.
     */
    g_debug_stage = 21U;

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        g_debug_error = 0xC101U;
        Error_Handler();
    }
    g_debug_stage = 22U;

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        g_debug_error = 0xC102U;
        Error_Handler();
    }

    SystemCoreClockUpdate();
    g_debug_stage = 23U;
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
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    g_debug_stage = 51U;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        g_debug_error = 0xAA01U;
        Error_Handler();
    }
    g_debug_stage = 52U;
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
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    g_debug_stage = 61U;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        g_debug_error = 0xAA02U;
        Error_Handler();
    }
    g_debug_stage = 62U;
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* Board example confirms LED1 is PB1 and active LOW. */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LABDAQ_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LABDAQ_LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LABDAQ_LED_PORT, LABDAQ_LED_PIN, GPIO_PIN_SET);

    /* Rev2 schematic: PB0 -> BL_EN -> Q1 S8050 -> LEDK1/2/3, active HIGH. */
    GPIO_InitStruct.Pin = LABDAQ_LCD_BL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LABDAQ_LCD_BL_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LABDAQ_LCD_BL_PORT, LABDAQ_LCD_BL_PIN, GPIO_PIN_RESET);

    /* RS485 direction pin must be a defined output before communication code uses it. */
    GPIO_InitStruct.Pin = LABDAQ_RS485_DIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LABDAQ_RS485_DIR_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LABDAQ_RS485_DIR_PORT, LABDAQ_RS485_DIR_PIN, GPIO_PIN_RESET);

    /* User Button on PA0 */
    GPIO_InitStruct.Pin = LABDAQ_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(LABDAQ_BTN_PORT, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    if (g_debug_error == 0U) {
        g_debug_error = 0xE001U;
    }
    g_debug_stage = 0xEEEEU;
    __DSB();

    /* Keep SysTick alive so the distinctive fault blink remains visible. */
    LABDAQ_Heartbeat_FaultBlocking();
}
