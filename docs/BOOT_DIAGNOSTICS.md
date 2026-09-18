# Boot failure root cause - 2026-09-18

## Symptom
ST-LINK successfully erased, programmed and verified the STM32F407 at 0x08000000,
but there was no PB1 LED activity, no delayed LCD backlight sequence, and no USART1
boot text even though the CP2104 COM port enumerated on the PC.

## Root cause
The custom minimal startup file routed SysTick_Handler to Default_Handler.
HAL_Init() enables the HAL SysTick interrupt before main continues. On the first SysTick
interrupt the CPU therefore entered an infinite Default_Handler loop, preventing the
firmware from reaching GPIO/UART/LCD diagnostics.

## Fix
SysTick_Handler now calls HAL_IncTick(). This allows HAL_GetTick() and HAL_Delay() to
operate and prevents the immediate post-HAL_Init trap.

## Remaining startup work
The current custom vector table is still only a minimal core-exception table. Before
enabling production DMA/Ethernet/CAN/UART interrupt operation it must be replaced by the
official complete STM32F407 startup/vector table (or an equivalent complete table) and
the required IRQ handlers must be connected. The SysTick fix is sufficient to validate
the immediate boot path but is not the final interrupt architecture.
