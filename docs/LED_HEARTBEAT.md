# On-board LED heartbeat diagnostics

The PC13 on-board LED is a visual health indicator independent of Ethernet, web UI and LCD.

| State | Toggle interval | Meaning |
|---|---:|---|
| BOOT | 100 ms | MCU has started executing firmware |
| INIT | 250 ms | peripherals/subsystems are being initialized |
| READY | 1000 ms | firmware healthy, DAQ not streaming |
| ACQUIRING | 500 ms | normal continuous data acquisition |
| TEST_RUNNING | 125 ms | cyclic/triaxial experiment is running |
| FAULT | 75 ms | initialization/runtime fatal error |

The heartbeat implementation is non-blocking during normal operation. Fault mode intentionally becomes a blocking rapid blink after a fatal error. LED polarity may be inverted on some board revisions; the diagnostic is based on blinking frequency, not ON/OFF polarity.

The heartbeat must never be used as the only safety indicator for hydraulic actuation. E-stop and hardware safety chains remain independent.
