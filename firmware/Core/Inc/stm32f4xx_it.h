/**
 ******************************************************************************
 * @file    stm32f4xx_it.h
 * @brief   This file contains the headers of the interrupt handlers.
 ******************************************************************************
 */

#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* Peripheral interrupt handlers */
void DMA2_Stream0_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void ETH_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
