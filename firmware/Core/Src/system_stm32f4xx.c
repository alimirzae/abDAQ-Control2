/**
 ******************************************************************************
 * @file    system_stm32f4xx.c
 * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File
 ******************************************************************************
 */

#include "stm32f4xx.h"

#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    25000000U /* 25 MHz External oscillator on EWB board */
#endif

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    16000000U
#endif

uint32_t SystemCoreClock = 168000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void)
{
    /* FPU settings ------------------------------------------------------------*/
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
      SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
    #endif

    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x24003010;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;

    /* Disable all interrupts */
    RCC->CIR = 0x00000000;

    /* Configure the Vector Table location add offset address ------------------*/
    #ifdef VECT_TAB_SRAM
      SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET;
    #else
      SCB->VTOR = FLASH_BASE | 0x00;
    #endif
}

void SystemCoreClockUpdate(void)
{
    uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;
    
    tmp = RCC->CFGR & RCC_CFGR_SWS;

    switch (tmp)
    {
      case 0x00:
        SystemCoreClock = HSI_VALUE;
        break;
      case 0x04:
        SystemCoreClock = HSE_VALUE;
        break;
      case 0x08:
        pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
        pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
        
        if (pllsource != 0) {
          pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
        } else {
          pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
        }

        pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
        SystemCoreClock = pllvco/pllp;
        break;
      default:
        SystemCoreClock = HSI_VALUE;
        break;
    }
    tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
    SystemCoreClock >>= tmp;
}
