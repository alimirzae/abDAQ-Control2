# MEMORY.md - LabDAQ-Control System Context & Hardware Memory

This document stores persistent hardware specifications, architectural decisions, pin assignments, and memory allocation maps for the **LabDAQ-Control** project.

---

## 1. Hardware Architecture & Pin Matrix

### Microcontroller Target
- **MCU**: STM32F407VGT6 (100-pin LQFP)
- **Clock**: 168 MHz Core Clock (SYSCLK), generated from 25.0 MHz External HSE Crystal
  - PLL_M = 25, PLL_N = 336, PLL_P = 2, PLL_Q = 7
  - APB1 Peripheral Clock = 42 MHz (Timers = 84 MHz)
  - APB2 Peripheral Clock = 84 MHz (Timers = 168 MHz, ADC1 clock = 21 MHz)
- **Internal Flash**: 1024 KB
- **SRAM**: 128 KB SRAM1/SRAM2 + 64 KB Core Coupled Memory (CCMRAM at `0x10000000`)

---

### Header J1 / Level Shifter Pin Assignments (Port E)

The board routes Port E high-byte pins through an 8-channel bidirectional voltage level shifter (`TXS0108E`):
- `VCCA` (Port A): Connected to **+3.3V** rail from STM32 board
- `VCCB` (Port B): Connected to **+5.0V** rail for external MUX & Actuator

| STM32 Pin | Shifter Side A | Shifter Side B | Target Peripheral | Function Description |
| :--- | :--- | :--- | :--- | :--- |
| **PE8** | A1 (3.3V) | B1 (5.0V) | CD74HC4067 Pin 10 | Address line **S0** (LSB) |
| **PE9** | A2 (3.3V) | B2 (5.0V) | CD74HC4067 Pin 11 | Address line **S1** |
| **PE10** | A3 (3.3V) | B3 (5.0V) | CD74HC4067 Pin 14 | Address line **S2** |
| **PE11** | A4 (3.3V) | B4 (5.0V) | CD74HC4067 Pin 13 | Address line **S3** (MSB) |
| **PE12** | A5 (3.3V) | B5 (5.0V) | CD74HC4067 Pin 15 | Enable input **/EN** (Active LOW) |
| **PE13** | A6 (3.3V) | B6 (5.0V) | Ext. BNC / Terminal | **SYNC_OUT** (10µs pulse on cycle start) |
| **PE14** | A7 (3.3V) | B7 (5.0V) | Relay / SSR Driver | **ACTUATOR_OUT** (Cyclic load valve/motor) |
| **PE15** | A8 (3.3V) | B8 (5.0V) | Front Panel LED | **TEST_STATUS** (Active High while running) |

---

### Analog & Sensor Pins

| STM32 Pin | Peripheral Function | Connected Node | Signal Voltage Range |
| :--- | :--- | :--- | :--- |
| **PA4** | ADC1_IN4 | CD74HC4067 Pin 1 (SIG) | 0.0V to 3.3V (Scaled sensor input) |
| **PA3** | ADC1_IN3 | Direct Analog Input 1 | 0.0V to 3.3V (Fast Load Cell bypass) |
| **PA5** | ADC1_IN5 | Direct Analog Input 2 | 0.0V to 3.3V (Fast LVDT bypass) |
| **VREF+** | Analog Reference | Filtered 3.3V Analog Rail | 3.300 V ± 0.5% |
| **AGND** | Analog Ground | Star Ground / Sensor Ground | 0.000 V |

---

### Communication Peripheral Pins

| Interface | Peripheral | STM32 Pin | Connected Hardware | Default Parameters |
| :--- | :--- | :--- | :--- | :--- |
| **Serial / SCPI** | USART1 | PA9 (TX), PA10 (RX) | CP2104 USB-UART | 115200 bps, 8-N-1 |
| **RS-485 Modbus** | USART2 | PD5 (TX), PD6 (RX) | SP3485 Transceiver | 115200 bps, 8-N-1 |
| **RS-485 Direction**| GPIO Output | PD7 | SP3485 DE / ~RE | High=TX, Low=RX |
| **CAN Bus** | CAN1 | PB8 (RX), PB9 (TX) | SN65HVD230 Trcv. | 500 kbps / 1 Mbps |
| **Ethernet** | RMII / ETH | PA1, PA2, PA7, PB11..PB13, PC1, PC4, PC5 | LAN8720A PHY | 100 Mbps Full Duplex, IP 192.168.1.150 |
| **Heartbeat LED** | GPIO Output | PC13 | Onboard Blue LED | 1 Hz toggle |
| **User Button** | GPIO Input | PA0 (WAKEUP) | Onboard Pushbutton | Active High with Pulldown |

---

## 2. Memory & Buffer Allocation Map

```
========================================================================
FLASH MEMORY (1024 KB) at 0x08000000
------------------------------------------------------------------------
0x08000000 - 0x080001FC : Cortex-M4 Interrupt Vector Table (.isr_vector)
0x08000200 - 0x0801FFFF : Executable Code (.text) & Constants (.rodata)
0x08020000 - 0x080FFFFF : Calibration Parameters & Test Profiles Sector

INTERNAL SRAM1+2 (128 KB) at 0x20000000
------------------------------------------------------------------------
0x20000000 - 0x20000FFF : System & Peripheral Descriptors (.data / .bss)
0x20001000 - 0x20002FFF : DMA Ping-Pong Buffer (4096 words = 8192 bytes)
                           Block 0 (Half): 2048 words (128 frames x 16 ch)
                           Block 1 (Full): 2048 words (128 frames x 16 ch)
0x20003000 - 0x200037FF : Filter History Arrays & Accumulators
0x20003800 - 0x20004FFF : TCP/IP Ethernet Buffers (LwIP / Socket queues)
0x20005000 - 0x2001EFFF : Available Application Heap
0x2001F000 - 0x2001FFFF : Main Stack Pointer (MSP) (4 KB stack reserved)

CCMRAM (64 KB) at 0x10000000 (Zero-wait state Cortex-M4 direct bus)
------------------------------------------------------------------------
0x10000000 - 0x10003FFF : Real-Time Cyclic Engine Math & Statistics Cache
========================================================================
```

---

## 3. Uploaded Hardware Images & Documentation Reference

Images provided for circuit documentation:
- `IMG_20260918_121022.jpg`: EWB-STM32F407V-LAN board top view with J1 header wiring to TXS0108E level shifter and CD74HC4067 multiplexer carrier board.
- `IMG_20260918_121030.jpg`: Detailed close-up of the analog conditioning circuitry, power decoupling capacitors, and sensor terminal blocks.
- Images are preserved in documentation directories:
  - `docs/images/IMG_20260918_121022.jpg`
  - `docs/images/IMG_20260918_121030.jpg`
  - Referenced in [`docs/WIRING.md`](docs/WIRING.md) and [`docs/HARDWARE.md`](docs/HARDWARE.md).

---

## 4. Ethernet Architecture: 1000 Hz UDP Multicast & Symmetrical Command Engine

### High-Speed Telemetry Streaming
- **Protocol**: UDP Multicast / Unicast.
- **Multicast Group**: `239.255.0.100` (Port 5001). Unicast fallback: `192.168.1.255:5001`.
- **Packet Rate**: Exactly 1,000 packets/sec (1 ms interval) driven by hardware timer interrupts.
- **Hardware Microsecond Timestamping**:
  - Derived from the ARM Cortex-M4 **DWT (Data Watchpoint and Trace)** cycle counter (`DWT->CYCCNT`).
  - At 168 MHz, 1 cycle = ~5.95 ns.
  - Calculated as: `Timestamp_us = (uint64_t)DWT->CYCCNT / 168U` (with overflow rollover tracker).
  - Guarantees zero jitter and absolute time tagging across network nodes.

### Symmetrical Command Processing Environment
- Unified command interpreter: `LABDAQ_Unified_ExecuteCommand(sys, cmd, resp, max_len, source_desc)`.
- Input sources:
  1. `USART1`: USB-VCP serial stream at 115200 baud.
  2. `UDP Socket 5001`: Inbound datagrams on port 5001 (broadcast/multicast/unicast).
- Commands supported identically:
  - `:RATE <Hz>`: Sets ADC sample rate and timer reload register.
  - `:FILTER <BYPASS|MAV|EMA|IIR|MEDIAN> [param]`: Configures digital DSP conditioning.
  - `:START` / `:STOP`: Starts/stops acquisition and cyclic testing.
  - `:MEAS:VOLT:ALL?`: Queries 16-channel calibrated voltages.
  - `*IDN?`: Returns IEEE-488.2 device identification string.

### Embedded Lightweight HTTP Web Server
- **Service Port**: HTTP Port 80 (LwIP RAW API).
- **Default IP**: `192.168.1.150` (or dynamically assigned via DHCP).
- **Capabilities**:
  - Sample rate selection dropdown (100 Hz, 500 Hz, 1000 Hz, 2000 Hz, 5000 Hz, 10000 Hz).
  - Analog channel selector (0 to 15) for active scope viewing.
  - HTML5 Canvas real-time oscilloscope updating at 20-30 FPS directly inside any web browser.

