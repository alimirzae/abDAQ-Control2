# AGENTS.md - LabDAQ-Control AI Engineering Directives

This document provides system-level instructions, architectural boundaries, hardware constraints, and code conventions for AI coding assistants working with the **LabDAQ-Control** repository.

---

## 1. System Overview & Core Mission
**LabDAQ-Control** is a dual-purpose embedded firmware and acquisition platform running on the **STM32F407VGT6 (ARM Cortex-M4F @ 168 MHz)**:
1. **High-Speed Multi-Channel DAQ**: 16 multiplexed analog channels via `CD74HC4067` and level shifter `TXS0108E` with ping-pong DMA acquisition.
2. **Cyclic Mechanical / Fatigue Test Controller**: Closed-loop or synchronous load-cycle generator with pulse output (`SYNC_OUT`), solenoid/actuator drive (`ACTUATOR_OUT`), and automatic cycle counter with safety limits.
3. **Multi-Protocol Industrial Interfacing**: SCPI (over USB-CDC/UART), Modbus RTU (over RS-485 with SP3485), and high-speed binary TCP packet streaming (via LAN8720A RMII).

---

## 2. Hardware Mapping & Hardware Rules

### Level Shifting Constraint (CRITICAL)
- The STM32F407 operates on **3.3V VDD logic**.
- The `TXS0108E` 8-channel bidirectional level shifter is connected to **Port E (PE8 .. PE15)**:
  - Port A of TXS0108E is powered by **3.3V** (STM32 side).
  - Port B of TXS0108E is powered by **5.0V** (Multiplexer / external actuator side).
  - **Do NOT reassign MUX address lines (S0..S3, /EN)** to pins outside Port E without checking the schematic and level shifter connections.
  - Port E pin assignments:
    - `PE8`: S0 (MUX Channel Address 0)
    - `PE9`: S1 (MUX Channel Address 1)
    - `PE10`: S2 (MUX Channel Address 2)
    - `PE11`: S3 (MUX Channel Address 3)
    - `PE12`: /EN (MUX Enable - Active LOW)
    - `PE13`: SYNC_OUT (Sync pulse for external trigger/oscilloscope)
    - `PE14`: ACTUATOR_OUT (Solid-state relay / solenoid valve drive)
    - `PE15`: TEST_STATUS (LED / digital status flag)

### Analog Channel Multiplexing
- The multiplexed signal (`SIG` pin 1 on CD74HC4067) routes directly into **PA4 (`ADC1_IN4`)**.
- When switching channels, a minimum settling delay of **5 microseconds (`LABDAQ_MUX_SETTLE_US`)** MUST be honored before triggering ADC conversion to prevent capacitive crosstalk between adjacent channels.

### RS-485 Direction Control
- The SP3485 transceiver uses **PD7** as `DE` (Driver Enable) / `~RE` (Receiver Enable).
- **Rule**: Set `PD7 = HIGH` before transmitting bytes; set `PD7 = LOW` immediately upon transmission completion to listen for responses.

---

## 3. Firmware Coding Standards (STM32 HAL / CMSIS)

- **Language Standard**: C99 / C++11 compatible. All headers must wrap definitions in `extern "C" { ... }`.
- **Naming Conventions**:
  - Functions: `LABDAQ_<Module>_<Action>` (e.g. `LABDAQ_ADC_Init`, `LABDAQ_MUX16_SelectChannel`).
  - Types/Structs: `labdaq_<module>_t` or `labdaq_<name>_t`.
  - Macros/Constants: `LABDAQ_<MODULE>_<NAME>`.
- **Interrupt Safety**:
  - Shared variables between ISRs and foreground tasks (such as buffer flags or cycle counts) MUST be declared `volatile`.
  - Atomic reads/writes or critical section macros (`__disable_irq()` / `__enable_irq()`) must guard 32-bit cycle count updates if accessed across multiple contexts.
- **Zero Heap Allocation**:
  - No `malloc()` or `free()` allowed in the real-time acquisition hot-path. All buffers, filters, and state objects are statically allocated in `.bss` / `.data` or placed in 64 KB CCMRAM.

---

## 4. Signal Processing & Filtering Pipeline

Any modifications or extensions to the digital signal conditioning pipeline in `firmware/Drivers/labdaq_filter.c` must adhere to:
1. **Low Computational Cost**: Max budget of 200 cycles per sample per channel so that 16 channels can be processed at up to 10 kSPS without exceeding 15% CPU load at 168 MHz.
2. **Fixed or Direct Form II**: Use Direct Form II Transposed for IIR filters to maximize numerical stability on Cortex-M4 single-precision FPU.
3. **Spike Rejection**: Median filter handles single-cycle impulse noise from electrical switching of high-power relays or hydraulic valves.

---

## 5. Communication Protocol Integrity

- **SCPI**: Standard IEEE 488.2 query responses end in `\r\n`. Any query command must always return a deterministic response even upon error (`ERR: ...`).
- **Modbus RTU**: Strict adherence to Modbus over serial line specification. Always compute and verify standard CRC16 (polynomial `0xA001`).
- **Binary Streaming Frame**: Always preserve the 2-byte magic header `0xAA55` and 2-byte CRC16-CCITT trailer to allow stream re-synchronization over TCP sockets.

---
## 6. Triaxial controller extension rules
- `labdaq_control.c/.h` owns runtime time/log/calibration/PID/experiment configuration; do not duplicate these states in HTTP handlers.
- The release sampling targets are 100/500/1000 complete 16-channel frames/s. Any higher rate must be treated as experimental until measured.
- Two EP outputs are 0–10 V command domains. Never enable physical PID output before hardware pin/timer mapping, polarity, feedback channel, E-stop and pressure relief are verified.
- Every sensor channel has a human name, engineering unit, gain and offset. Test records must reference calibration metadata.
- Network/UI work must never block the acquisition/PID hot path; no heap allocation or flash/SD writes inside real-time callbacks.
- A successful link without `Reset_Handler` is NOT a valid firmware build. Release builds require a valid vector table and startup path.
