# ROADMAP — LabDAQ-Control

## Baseline completed
- STM32F407 project builds with Cortex-M4 target flags and matching HAL/CMSIS dependencies.
- 16-channel acquisition architecture, filtering, buffering, SCPI/Modbus/UDP command foundation.
- Bootable vector table / Reset_Handler added.
- Web management information architecture expanded.
- Control core added for time, logs, channel calibration, two EP motor calibrations and two PID loops.
- Triaxial experiment model includes cyclic, monotonic, stress-controlled, strain-controlled and custom profiles.

## Phase 1 — hardware commissioning
- Confirm exact two PWM→0–10 V output pins/timers and EP motor polarity.
- Confirm two feedback channels (pressure/load/displacement) used by each PID.
- Verify E-stop, pressure relief, watchdog and fail-safe zero-output behavior.
- Validate 100/500/1000 complete 16-channel frames/s on oscilloscope and captured data.

## Phase 2 — persistent configuration & RTC
- Add CRC/versioned flash settings with wear-safe save strategy.
- Bind time abstraction to STM32 RTC backup domain and optional battery; web time remains the configuration surface.
- Persist channel names/calibration, PID, motor calibration, network and last safe sampling preset.

## Phase 3 — closed-loop triaxial engine
- Implement deterministic timer-driven PID update independent of HTTP/network tasks.
- Add anti-windup, bumpless transfer, slew-rate/output clamps and sensor-fault interlocks.
- Add waveform/profile generator: sine, triangle, trapezoid, custom points.
- Add pressure-controlled and displacement/strain-controlled modes.

## Phase 4 — experiment records
- Test metadata, specimen ID, operator, calibration IDs, start/end timestamps.
- SD-card binary/CSV logging with buffered writes outside real-time loop.
- Download/export endpoints and run summary/statistics.

## Release gate
A production release requires: 0 compiler errors; no missing Reset_Handler; validated vector table; 100/500/1000 SPS/ch timing evidence; actuator safe-state test; E-stop test; calibration traceability; PID commissioning report; 8-hour soak test.
