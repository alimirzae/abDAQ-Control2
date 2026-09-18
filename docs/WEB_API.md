# Web/API contract

The embedded UI exposes tabs for Data Logger, Triaxial Tests, PID/EP Motors, Channels & Calibration, Time, Logs and Settings.

The command engine is the canonical configuration backend so UART and Ethernet can share identical behavior. HTTP handlers should translate validated form/JSON fields into the same control functions/commands rather than duplicate business logic.

Recommended endpoints for the LwIP HTTP integration:
- GET `/api/status`
- GET `/api/sample?ch=0..15`
- GET/POST `/api/settings`
- GET/POST `/api/time`
- GET/POST `/api/channels`
- GET/POST `/api/pid/0`, `/api/pid/1`
- POST `/api/motor/0/calibrate`, `/api/motor/1/calibrate`
- GET/POST `/api/experiment`
- POST `/api/experiment/start|pause|resume|stop`
- GET `/api/logs`

All actuator-changing endpoints must reject commands when the system is faulted/not armed and must apply hard output limits.
