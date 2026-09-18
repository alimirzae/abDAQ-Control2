/**
 * Minimal CMSIS startup for STM32F407VG.
 * Provides vector table and Reset_Handler so the ELF is bootable.
 */
#include <stdint.h>
#include "stm32f4xx_hal.h"

extern int main(void);
extern void SystemInit(void);
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;

void Default_Handler(void)
{
    while (1) {}
}

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata) {
        *dst++ = *src++;
    }

    for (dst = &_sbss; dst < &_ebss;) {
        *dst++ = 0U;
    }

    SystemInit();
    (void)main();

    while (1) {}
}

#define WEAK_DEFAULT __attribute__((weak, alias("Default_Handler")))

void NMI_Handler(void) WEAK_DEFAULT;
void HardFault_Handler(void) WEAK_DEFAULT;
void MemManage_Handler(void) WEAK_DEFAULT;
void BusFault_Handler(void) WEAK_DEFAULT;
void UsageFault_Handler(void) WEAK_DEFAULT;
void SVC_Handler(void) WEAK_DEFAULT;
void DebugMon_Handler(void) WEAK_DEFAULT;
void PendSV_Handler(void) WEAK_DEFAULT;

/* HAL_Init() enables SysTick. Keep the HAL millisecond time base alive. */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler
};
