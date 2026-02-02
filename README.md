# Remote Monitoring via IoT Dashboard (Wokwi ESP32)

An ESP32-based IoT robot simulation built in Wokwi. It publishes telemetry over MQTT (distance + IR switches + Wi-Fi RSSI) and listens for simple motion commands. LEDs emulate left/right motors so you can validate control logic without real hardware.

## Features
- ESP32 + HC-SR04 ultrasonic distance sensing
- 5 IR switch inputs (simulated with slide switches)
- MQTT telemetry publishing + command subscription
- Safety stop on obstacle proximity and command timeout
- Fully reproducible in Wokwi

## Project Layout
- `sketch.ino` — main firmware (Wi-Fi + MQTT + sensors + motor LEDs)
- `diagram.json` — Wokwi wiring diagram
- `libraries.txt` — Wokwi library list (PubSubClient)
- `wokwi-project.txt` — original Wokwi project link

## MQTT Topics and Commands
The robot uses a unique `ROBOT_ID` in `sketch.ino`:

- Telemetry topic: `robot/<id>/telemetry`
- Command topic:   `robot/<id>/cmd`

Command payloads (single character):
- `F` — forward (both motor LEDs on)
- `B` — backward (blink both LEDs)
- `L` — turn left (right LED on)
- `R` — turn right (left LED on)
- `S` — stop (both LEDs off)

Example telemetry JSON:
```json
{
  "dist_cm": 19.42,
  "ir": [1, 1, 0, 1, 1],
  "rssi": -45
}
```

## Quick Start (Wokwi)
1. Open the Wokwi project listed in `wokwi-project.txt`.
2. Ensure `sketch.ino`, `diagram.json`, and `libraries.txt` are loaded.
3. Start the simulation. The ESP32 connects to Wi-Fi and the MQTT broker.
4. Publish commands to `robot/<id>/cmd` and watch LED behavior.
5. Subscribe to `robot/<id>/telemetry` to view sensor updates.

## Configuration
Edit these in `sketch.ino`:
- `ROBOT_ID` — must be unique to avoid topic clashes
- `MQTT_HOST`, `MQTT_PORT` — broker settings
- `TELE_PERIOD_MS` — telemetry interval
- `CMD_TIMEOUT_MS` — auto-stop timeout (ms)
- Safety threshold: stop if `dist_cm < 20`

## Notes
- Wokwi Wi-Fi uses `Wokwi-GUEST` with an empty password.
- LEDs are used as motor indicators for safe simulation.
- IR switches are wired with `INPUT_PULLUP` (active-low).

## License
No license specified yet. Add a LICENSE file if you plan to open-source this.
# Remote_monitoring_via_IoT_dashboard
