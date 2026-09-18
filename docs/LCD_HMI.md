# Local LCD HMI

## Branding
The local instrument HMI uses:
- **iMonitor**
- **Azerbaijan Industrial Processing Co.**
- **iMonitor.ir**

This text is the firmware branding baseline for the LCD header. A bitmap/vector logo asset can be bound later when the physical LCD controller, resolution and pixel format are selected.

## Interaction
The existing PA0 user button is assigned as the **PAGE** button. Each rising edge advances one page with 200 ms software debounce. Emergency stop remains a separate hard-wired safety function and must never depend on the LCD or PAGE button.

## Pages
1. Status — DAQ/test state, sample rate, cycles, time, logs.
2. Sensors — all 16 named/calibrated engineering values.
3. Live Graph — rolling history of the selected sensor.
4. Test Graph — experiment X/Y graph surface (e.g. load-displacement or stress-strain).
5. Experiment — type, frequency, cycles, confining pressure, axial target.
6. PID / EP — two PID setpoints/gains and calibration state.
7. Alarms — recent event log.
8. System — firmware/platform/interfaces.

## Architecture
`labdaq_display.c/.h` is non-blocking and refreshes at 10 Hz. It only reads shared DAQ/control state. Rendering is behind weak `LABDAQ_DisplayHW_*` adapter hooks, so selecting ILI9341, ST7796, SSD1963 or another LCD later does not couple its driver to acquisition/PID code.

## Hardware still intentionally undecided
No SPI/FSMC pins, LCD controller, resolution, backlight PWM or touch controller are hard-coded yet. This prevents conflicts with Ethernet RMII, CAN, UART, ADC, MUX and the two future PWM→0–10 V outputs. Once the exact LCD module is selected, implement one hardware adapter file and keep the page/UI layer unchanged.
