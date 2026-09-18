# LabDAQ-Control — Triaxial Control, PID, Calibration, Time & Logs

## Scope
The firmware is organized as a dual-purpose 16-channel DAQ and cyclic triaxial controller. The control layer is `labdaq_control.c/.h` and is deliberately separated from the acquisition hot path.

## Web tabs
1. **Data Logger** — live acquisition, 100/500/1000 samples/s/channel commissioning presets, streaming status and scope.
2. **Triaxial Tests** — cyclic triaxial, monotonic, stress-controlled, strain-controlled and custom profiles; confining pressure, axial target, frequency and cycle count; arm/start/pause/emergency-stop workflow.
3. **PID / EP Motors** — two independent PID loops for two EP motor/valve channels. Output is normalized to the 0–10 V command domain.
4. **Channels & Calibration** — all 16 inputs have name, unit, gain, offset, min/max and enable state.
5. **Time** — set/query device Unix time from web/command interface.
6. **Logs** — ring-buffer event log for boot/config/calibration/test/fault events.
7. **Settings** — sampling, filters, network and safety/commissioning settings.

## EP motor / hydraulic jack commissioning
Never energize a hydraulic actuator merely because PID is configured. Commission in this order: verify E-stop and pressure relief; verify PWM→0–10 V direction and hard output limits; two-point calibrate each EP channel; identify the feedback channel; test open-loop at low command; then enable PID with conservative gains. Firmware currently keeps the physical closed-loop output gated in `LABDAQ_Control_Task` until the final PWM/DAC pin mapping and safe polarity are confirmed.

## Calibration model
Engineering value = raw/voltage input × gain + offset. Calibration metadata is per channel and must be recorded with sensor serial number, reference standard and date in the experiment record.

## Commands
- `:TIME:SET <unix>`, `:TIME?`
- `:CHAN:CAL ch,name,unit,gain,offset`
- `:PID:SET motor,kp,ki,kd,setpoint`
- `:MOTOR:CAL motor,cmdMin,cmdMax,feedbackMin,feedbackMax`
- `:LOG?`
- Existing `:RATE`, `:FILTER`, `:CYCLIC:START`, `:CYCLIC:STOP`, measurement and UDP commands remain supported.

## Persistence
Runtime configuration is represented explicitly in `labdaq_control_t`. Flash-backed configuration records, CRC/versioning and backup-domain RTC persistence are the next persistence layer; do not write flash in the acquisition ISR/hot path.
