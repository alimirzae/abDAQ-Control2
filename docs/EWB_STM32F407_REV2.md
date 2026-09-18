# EWB-STM32F407 Rev2.0 hardware mapping

This project uses the EWB-STM32F407 Rev2.0 schematic/user manual as the board authority.

## Confirmed board pins

| Function | STM32 pin | Notes |
|---|---|---|
| LED1 | PB1 | PB1 -> LED1 net -> R3 1.5K -> red LED -> GND; active HIGH |
| LCD backlight | PB0 | BL_EN -> R28/Q1 S8050 -> LEDK1/2/3; active HIGH |
| LCD CS | PD13 | LCD_CS |
| LCD reset | PD14 | LCD_RST |
| LCD D/C | PD15 | LCD_DC |
| LCD SPI clock | PA5 | SPI1_CLK |
| LCD SPI MISO | PA6 | SPI1_MISO |
| LCD SPI MOSI | PB5 | SPI1_MOSI |
| USB/Serial | USART1 PA9/PA10 | CP2104, firmware 115200 8N1 |
| WAKEUP button | PA0 | No external/user button is required for boot |

## 4-inch LCD

The installed panel is YT400S006, 4 inch. The board schematic marks 3.5-4.0 inch panels as
IM2 IM1 IM0 = 1 1 1, 4-wire 8-bit serial. Firmware uses the ST7796S-compatible SPI
command set, RGB565, 480x320 landscape.

The LCD is presentation-only. PA5 is reserved for LCD SPI1 clock while this display is
enabled and must not simultaneously be configured as a direct ADC input.

## Power-on diagnostics

After reset:
1. PB0 backlight is forced OFF.
2. USART1 sends a boot line at 115200 8N1.
3. PB1 LED1 blinks five times (100 ms ON / 100 ms OFF), totaling about one second.
4. PB0 backlight is enabled.
5. LCD initializes and shows the iMonitor/LabDAQ boot screen.
6. Normal heartbeat then indicates firmware state.

## Important correction history

Older project notes incorrectly used the V3 board mapping and temporarily assigned LED1
to PB2 and BL_EN to PB1. Those mappings are invalid for this Rev2 board and have been
replaced by the confirmed schematic mapping above.
