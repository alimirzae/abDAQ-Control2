/**
 ******************************************************************************
 * @file    stm32f4xx_it.c
 * @brief   Interrupt Service Routines for LabDAQ-Control
 ******************************************************************************
 */

#include "main.h"
#include "stm32f4xx_it.h"

extern DMA_HandleTypeDef hdma_adc;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

void NMI_Handler(void)
{
    while (1) {}
}

void HardFault_Handler(void)
{
    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
  * @brief This function handles DMA2 stream0 global interrupt (ADC1 DMA)
  */
void DMA2_Stream0_IRQHandler(void)
{
    /* Check Half-Transfer Complete */
    if (__HAL_DMA_GET_FLAG(&g_labdaq.adc.hdma_adc, DMA_FLAG_HTIF0_4)) {
        __HAL_DMA_CLEAR_FLAG(&g_labdaq.adc.hdma_adc, DMA_FLAG_HTIF0_4);
        LABDAQ_Buffer_OnHalfTransfer(&g_labdaq.buffer_mgr);
    }

    /* Check Full-Transfer Complete */
    if (__HAL_DMA_GET_FLAG(&g_labdaq.adc.hdma_adc, DMA_FLAG_TCIF0_4)) {
        __HAL_DMA_CLEAR_FLAG(&g_labdaq.adc.hdma_adc, DMA_FLAG_TCIF0_4);
        LABDAQ_Buffer_OnFullTransfer(&g_labdaq.buffer_mgr);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
