/**
 ******************************************************************************
 * @file    stm32f4xx_hal_conf.h
 * @brief   HAL configuration template file for LabDAQ-Control STM32F407
 ******************************************************************************
 */

#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Module Selection */
#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CAN_MODULE_ENABLED
#define HAL_CRC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_ETH_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

/* Oscillator Values adapted to EWB-STM32F407 board (25 MHz HSE Crystal) */
#if !defined  (HSE_VALUE)
  #define HSE_VALUE    25000000U /* Value of the External oscillator in Hz */
#endif

#if !defined  (HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    100U
#endif

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    16000000U
#endif

#if !defined  (LSI_VALUE)
  #define LSI_VALUE    32000U
#endif

#if !defined  (LSE_VALUE)
  #define LSE_VALUE    32768U
#endif

#if !defined  (LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    5000U
#endif

#define VDD_VALUE                3300U
#define TICK_INT_PRIORITY        0x00U
#define PREFETCH_ENABLE          1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE        1U

/* Ethernet Driver configuration (LAN8720A RMII) */
#define ETH_RX_BUF_SIZE                1524U
#define ETH_TX_BUF_SIZE                1524U
#define ETH_RXBUFNB                    4U
#define ETH_TXBUFNB                    4U
#define DP83848_PHY_ADDRESS            0x01U
#define PHY_RESET_DELAY                0x000000FFU
#define PHY_CONFIG_DELAY               0x00000FFFU
#define PHY_READ_TO                    0x0000FFFFU
#define PHY_WRITE_TO                   0x0000FFFFU

/* Includes */
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f4xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif
#ifdef HAL_ADC_MODULE_ENABLED
  #include "stm32f4xx_hal_adc.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f4xx_hal_tim.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f4xx_hal_uart.h"
#endif
#ifdef HAL_ETH_MODULE_ENABLED
  #include "stm32f4xx_hal_eth.h"
#endif
#ifdef HAL_CAN_MODULE_ENABLED
  #include "stm32f4xx_hal_can.h"
#endif
#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32f4xx_hal_spi.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32f4xx_hal_i2c.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
