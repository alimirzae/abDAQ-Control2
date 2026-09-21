/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for LabDAQ-Control STM32F407 System
  *                   Upgraded with non-blocking peripheral init & robust boot
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LABDAQ_CHANNEL_COUNT        16U
#define LABDAQ_ADC_AVG_SAMPLES      10U
#define LABDAQ_ADC_TIMEOUT_MS       10U
#define LABDAQ_MUX_SETTLE_US        20U

/* Active high / low enable for MUX */
#define LABDAQ_MUX_ENABLE_LEVEL     GPIO_PIN_SET
#define LABDAQ_MUX_DISABLE_LEVEL    GPIO_PIN_RESET

#define LABDAQ_PWM_PERIOD           999U   /* TIM1: 168 MHz / (84 * 1000) = 2 kHz PWM */
#define LABDAQ_UART_TIMEOUT_MS      100U
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ETH_TxPacketConfig TxConfig;
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

ADC_HandleTypeDef hadc1;
ETH_HandleTypeDef heth;
RTC_HandleTypeDef hrtc;
SD_HandleTypeDef hsd;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static uint16_t g_adc_raw[LABDAQ_CHANNEL_COUNT];
static uint32_t g_scan_counter = 0;
static char g_uart_buf[256];
static uint8_t g_dwt_ready = 0;
static uint8_t g_sd_ready = 0;
static uint8_t g_eth_ready = 0;
static uint8_t g_rtc_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ETH_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */
static void DWT_Delay_Init(void);
static void Delay_us(uint32_t us);
static void MUX_Enable(uint8_t enable);
static void MUX_Select(uint8_t channel);
static uint16_t ADC_ReadRaw(void);
static uint16_t ADC_ReadAverage(uint8_t samples);
static void LabDAQ_Scan16(uint16_t values[LABDAQ_CHANNEL_COUNT]);
static void UART_Print(const char *text);
static void UART_PrintScan(const uint16_t values[LABDAQ_CHANNEL_COUNT]);

static void LCD_Select(void);
static void LCD_Unselect(void);
static void LCD_WriteCommand(uint8_t cmd);
static void LCD_WriteData(const uint8_t *data, uint16_t len);
static void LCD_Reset(void);
static void LCD_HW_Init(void);

static void PWM_SetPercent(uint8_t channel, float percent);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Initialize DWT cycle counter for microsecond delays with fail-safe fallback
  */
static void DWT_Delay_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  
  /* Unlock DWT access if LAR is present on Cortex-M4 */
  #if defined(DWT_LAR_PTR)
  volatile uint32_t *dwt_lar = (volatile uint32_t *)0xE0001FB0;
  *dwt_lar = 0xC5ACCE55;
  #endif

  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* Check if DWT cycle counter is actually incrementing */
  uint32_t t1 = DWT->CYCCNT;
  for (volatile int i = 0; i < 100; i++) { __NOP(); }
  uint32_t t2 = DWT->CYCCNT;
  g_dwt_ready = (t2 != t1) ? 1 : 0;
}

/**
  * @brief Microsecond delay using DWT (with software fallback if DWT unavailable)
  */
static void Delay_us(uint32_t us)
{
  if (us == 0) return;

  if (g_dwt_ready)
  {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    /* Safe wait with loop limit to prevent freezing */
    while ((DWT->CYCCNT - start) < ticks)
    {
      if ((DWT->CYCCNT - start) > (ticks + 1000000U)) break; /* overflow protection */
    }
  }
  else
  {
    /* Fallback cycle loop for 168 MHz (approx. 42 cycles per microsecond loop) */
    volatile uint32_t count = us * 42U;
    while (count--) { __NOP(); }
  }
}

static void MUX_Enable(uint8_t enable)
{
  HAL_GPIO_WritePin(MUX_EN_GPIO_Port, MUX_EN_Pin,
                    enable ? LABDAQ_MUX_ENABLE_LEVEL : LABDAQ_MUX_DISABLE_LEVEL);
}

static void MUX_Select(uint8_t channel)
{
  channel &= 0x0FU;
  HAL_GPIO_WritePin(MUX_A0_GPIO_Port, MUX_A0_Pin, (channel & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MUX_A1_GPIO_Port, MUX_A1_Pin, (channel & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MUX_A2_GPIO_Port, MUX_A2_Pin, (channel & 0x04U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MUX_A3_GPIO_Port, MUX_A3_Pin, (channel & 0x08U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint16_t ADC_ReadRaw(void)
{
  uint16_t value = 0;

  if (HAL_ADC_Start(&hadc1) != HAL_OK) return 0;
  if (HAL_ADC_PollForConversion(&hadc1, LABDAQ_ADC_TIMEOUT_MS) == HAL_OK)
  {
    value = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);

  return value;
}

static uint16_t ADC_ReadAverage(uint8_t samples)
{
  uint32_t sum = 0;
  if (samples == 0U) samples = 1U;

  /* Dummy conversion after changing MUX channel to clear sample capacitor */
  (void)ADC_ReadRaw();

  for (uint8_t i = 0; i < samples; ++i)
  {
    sum += ADC_ReadRaw();
  }

  return (uint16_t)(sum / samples);
}

static void LabDAQ_Scan16(uint16_t values[LABDAQ_CHANNEL_COUNT])
{
  MUX_Enable(1U);

  for (uint8_t ch = 0; ch < LABDAQ_CHANNEL_COUNT; ++ch)
  {
    MUX_Select(ch);
    Delay_us(LABDAQ_MUX_SETTLE_US);
    values[ch] = ADC_ReadAverage(LABDAQ_ADC_AVG_SAMPLES);
  }

  ++g_scan_counter;
}

static void UART_Print(const char *text)
{
  if (text == NULL) return;
  HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)strlen(text), LABDAQ_UART_TIMEOUT_MS);
}

static void UART_PrintScan(const uint16_t values[LABDAQ_CHANNEL_COUNT])
{
  int n = snprintf(g_uart_buf, sizeof(g_uart_buf), "#%lu", (unsigned long)g_scan_counter);
  for (uint8_t ch = 0; ch < LABDAQ_CHANNEL_COUNT && n > 0 && n < (int)sizeof(g_uart_buf) - 12; ++ch)
  {
    n += snprintf(&g_uart_buf[n], sizeof(g_uart_buf) - (size_t)n, ",%u", values[ch]);
  }

  if (n > 0 && n < (int)sizeof(g_uart_buf) - 3)
  {
    g_uart_buf[n++] = '\r';
    g_uart_buf[n++] = '\n';
    g_uart_buf[n] = '\0';
    HAL_UART_Transmit(&huart1, (uint8_t *)g_uart_buf, (uint16_t)n, LABDAQ_UART_TIMEOUT_MS);
  }
}

/* Generic SPI LCD hardware layer */
static void LCD_Select(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

static void LCD_Unselect(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static void LCD_WriteCommand(uint8_t cmd)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
  LCD_Select();
  HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
  LCD_Unselect();
}

static void LCD_WriteData(const uint8_t *data, uint16_t len)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  LCD_Select();
  HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, 500);
  LCD_Unselect();
}

static void LCD_Reset(void)
{
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120);
}

static void LCD_HW_Init(void)
{
  LCD_Unselect();
  HAL_GPIO_WritePin(LCD_BL_EN_GPIO_Port, LCD_BL_EN_Pin, GPIO_PIN_RESET);
  LCD_Reset();

  /* Backlight ON */
  HAL_GPIO_WritePin(LCD_BL_EN_GPIO_Port, LCD_BL_EN_Pin, GPIO_PIN_SET);
}

static void PWM_SetPercent(uint8_t channel, float percent)
{
  if (percent < 0.0f) percent = 0.0f;
  if (percent > 100.0f) percent = 100.0f;

  uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim1);
  uint32_t pulse = (uint32_t)((percent * (float)(period + 1U)) / 100.0f);
  if (pulse > period) pulse = period;

  if (channel == 1U)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
  else if (channel == 2U)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pulse);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock (168 MHz) */
  SystemClock_Config();

  /* Initialize all configured GPIO pins */
  MX_GPIO_Init();

  /* Initialize Primary Diagnostic UART FIRST so we can report status */
  MX_USART1_UART_Init();
  UART_Print("\r\n========================================\r\n");
  UART_Print("   LabDAQ-Control STM32F407 Booting...  \r\n");
  UART_Print("========================================\r\n");
  UART_Print("[OK] System Clocks & GPIO Initialized\r\n");
  UART_Print("[OK] USART1 115200 Baud Console Ready\r\n");

  /* Initialize Timers & PWM */
  MX_TIM1_Init();
  MX_TIM2_Init();
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  PWM_SetPercent(1U, 0.0f);
  PWM_SetPercent(2U, 0.0f);
  UART_Print("[OK] TIM1 (PWM 2kHz) & TIM2 Initialized\r\n");

  /* Initialize ADC1 */
  MX_ADC1_Init();
  UART_Print("[OK] ADC1 Initialized (PA4 / ADC1_IN4)\r\n");

  /* Initialize SPI1 for LCD */
  MX_SPI1_Init();
  LCD_HW_Init();
  UART_Print("[OK] SPI1 & LCD Hardware Initialized\r\n");

  /* Initialize Microsecond DWT delay */
  DWT_Delay_Init();
  if (g_dwt_ready) {
    UART_Print("[OK] DWT Microsecond Timer Enabled\r\n");
  } else {
    UART_Print("[WARN] DWT Cycle Counter inactive, using software delay\r\n");
  }

  /* Initialize RTC (Non-blocking, failsafe) */
  MX_RTC_Init();
  if (g_rtc_ready) {
    UART_Print("[OK] RTC Initialized\r\n");
  } else {
    UART_Print("[WARN] RTC not synchronized (bypassed)\r\n");
  }

  /* Initialize SDIO (Non-blocking, failsafe if no SD card inserted) */
  MX_SDIO_SD_Init();
  if (g_sd_ready) {
    UART_Print("[OK] SD Card Detected and Initialized\r\n");
  } else {
    UART_Print("[INFO] SD Card slot empty or uninitialized (bypassed)\r\n");
  }

  /* Initialize LAN8720A Ethernet (Non-blocking, failsafe if no link/cable) */
  MX_ETH_Init();
  if (g_eth_ready) {
    UART_Print("[OK] Ethernet LAN8720A RMII Initialized\r\n");
  } else {
    UART_Print("[INFO] Ethernet link not active / bypassed\r\n");
  }

  /* Setup MUX */
  MUX_Enable(0U);
  MUX_Select(0U);
  MUX_Enable(1U);

  HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_SET);
  UART_Print("LabDAQ-Control: 16-Channel Analog Scanning Active\r\n");
  UART_Print("----------------------------------------\r\n");

  /* Infinite loop */
  while (1)
  {
    LabDAQ_Scan16(g_adc_raw);
    UART_PrintScan(g_adc_raw);

    HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);

    /* 250 ms scan interval in bring-up mode */
    HAL_Delay(250);
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  *   Supports HSI + LSI by default, and attempts HSE if present.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /* RTC Clock Selection (LSI as robust fallback) */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  (void)HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel (PA4 -> ADC1_IN4) */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function with proper PHY reset & non-blocking guard
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{
  static uint8_t MACAddr[6] = {0x00, 0x80, 0xE1, 0x00, 0x00, 0x00};

  /* 1. Hardware Reset sequence for LAN8720A PHY via PC0 */
  HAL_GPIO_WritePin(ETH_PHY_RST_GPIO_Port, ETH_PHY_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(25); /* Hold PHY in reset */
  HAL_GPIO_WritePin(ETH_PHY_RST_GPIO_Port, ETH_PHY_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(50); /* Allow PHY crystal / RMII 50MHz clock to stabilize */

  /* 2. Configure Ethernet MAC & DMA */
  heth.Instance = ETH;
  heth.Init.MACAddr = MACAddr;
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* Non-blocking check: do not halt the entire MCU if Ethernet is unplugged */
  if (HAL_ETH_Init(&heth) == HAL_OK)
  {
    memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
    TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
    g_eth_ready = 1;
  }
  else
  {
    g_eth_ready = 0;
  }
}

/**
  * @brief RTC Initialization Function with non-blocking guard
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 249; /* For 32 kHz LSI clock: (127+1)*(249+1) = 32000 */
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

  if (HAL_RTC_Init(&hrtc) == HAL_OK)
  {
    g_rtc_ready = 1;
  }
  else
  {
    g_rtc_ready = 0;
  }
}

/**
  * @brief SDIO Initialization Function with non-blocking guard
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 2;

  /* Non-blocking initialization: if no SD card is in the socket, do NOT hang! */
  if (HAL_SD_Init(&hsd) == HAL_OK)
  {
    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) == HAL_OK)
    {
      g_sd_ready = 1;
    }
  }
  else
  {
    g_sd_ready = 0;
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; /* 84 MHz / 4 = 21 MHz (safe for display) */
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 PWM Initialization Function (PE9, PE11)
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  __HAL_RCC_TIM1_CLK_ENABLE();

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 83;        /* 168 MHz / 84 = 2 MHz */
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;          /* 2 MHz / 1000 = 2 kHz */
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    Error_Handler();

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    Error_Handler();
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 5249;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function (Console / Diagnostic)
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* Configure Initial GPIO Output Levels */
  HAL_GPIO_WritePin(ETH_PHY_RST_GPIO_Port, ETH_PHY_RST_Pin, GPIO_PIN_SET); /* Default PHY Active (nRST=High) */
  HAL_GPIO_WritePin(GPIOB, LCD_BL_EN_Pin | STATUS_LED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, MUX_A0_Pin | MUX_A1_Pin | MUX_A2_Pin | MUX_A3_Pin | MUX_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, LCD_CS_Pin | LCD_RST_Pin | LCD_DC_Pin, GPIO_PIN_RESET);

  /* Configure GPIO pin : ETH_PHY_RST_Pin (PC0) */
  GPIO_InitStruct.Pin = ETH_PHY_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ETH_PHY_RST_GPIO_Port, &GPIO_InitStruct);

  /* Configure GPIO pin : USER_KEY_Pin (PA0) */
  GPIO_InitStruct.Pin = USER_KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_KEY_GPIO_Port, &GPIO_InitStruct);

  /* Configure GPIO pins : LCD_BL_EN_Pin (PB0), STATUS_LED_Pin (PB1) */
  GPIO_InitStruct.Pin = LCD_BL_EN_Pin | STATUS_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure GPIO pins : MUX_A0 (PE7), MUX_A1 (PE8), MUX_A2 (PE10), MUX_A3 (PE12), MUX_EN (PE13) */
  GPIO_InitStruct.Pin = MUX_A0_Pin | MUX_A1_Pin | MUX_A2_Pin | MUX_A3_Pin | MUX_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* Configure GPIO pins : LCD_CS (PD12), LCD_RST (PD14), LCD_DC (PD15) */
  GPIO_InitStruct.Pin = LCD_CS_Pin | LCD_RST_Pin | LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    /* Trap in case of critical unrecoverable clock/bus error */
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report file name and line number */
}
#endif /* USE_FULL_ASSERT */
