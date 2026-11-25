# Level 3 — Network-connected sensor (ESP32 + MQTT)

## Overview
An ESP32 reads a potentiometer on GPIO34 and publishes JSON readings over MQTT. If WiFi or MQTT is unavailable, readings are stored in a RAM ring buffer and published oldest-first after reconnection.

## Requirements covered
- MQTT publishing to `broker.hivemq.com:1883`
- WiFi and MQTT reconnect attempts every 5 seconds
- Offline RAM buffer holding up to 30 readings
- Sampling continues every 2 seconds while offline
- Buffered readings are sent oldest-first after reconnect

When the buffer is full, the oldest reading is discarded so the newest data can be retained. `droppedReadings` counts these drops. The RAM buffer is cleared by reset or power loss.

## Hardware
- ESP32 DevKit v1
- Potentiometer VCC → 3V3, GND → GND, SIG → GPIO34

The potentiometer is a simple analog sensor stand-in. Turn it to change the reading.

## Files
- `sketch.ino` — ESP32 firmware
- `diagram.json` — Wokwi circuit
- `platformio.ini` — PlatformIO configuration and PubSubClient dependency
- `README.md` — setup and test instructions

## Run in Wokwi
1. Open https://wokwi.com/projects/new/esp32
2. Replace the sketch with `sketch.ino`.
3. Replace the circuit diagram with `diagram.json`.
4. Start the simulation and open Serial Monitor at **115200 baud**.
5. Turn the potentiometer. The Serial Monitor should show sampled readings and, when the broker is reachable, `Published:` messages.

The sketch uses Wokwi's default `Wokwi-GUEST` WiFi network. The public MQTT broker is a shared test service; it may occasionally be unavailable.

## View MQTT messages
Use MQTT Explorer or another MQTT client and connect with:
- Host: `broker.hivemq.com`
- Port: `1883` (TCP, no TLS)
- Topic: `level3-demo/esp32/readings/kavash-001`

Subscribe to the topic to see messages like:

```json
{"sequence":1,"uptime_ms":2000,"analog":2048}
```

Change `MQTT_TOPIC` to a unique topic before testing and subscribe to that same topic. Public topics are not private and may contain unrelated messages. Do not send sensitive data.

## Test offline buffering and reconnect
1. Start the simulation and confirm `Published:` messages appear.
2. Temporarily make the broker unreachable (for example, change `MQTT_HOST` to an invalid hostname and restart).
3. Confirm `Sampled` messages continue and `buffered` increases.
4. Restore `MQTT_HOST` to `broker.hivemq.com` and restart or restore network access.
5. Confirm MQTT reconnects and queued messages publish.
6. Capture screenshots of the Serial Monitor showing offline buffering and recovery, plus the MQTT subscriber showing received messages. Add the real screenshots to an `evidence/` folder in your GitHub repository.

Screenshots are not included because evidence must come from your own successful run.

## Arduino IDE
1. Install ESP32 board support.
2. Install **PubSubClient** by Nick O'Leary using Library Manager.
3. Select your ESP32 board and port, then upload `sketch.ino`.
4. Set `WIFI_SSID` and `WIFI_PASSWORD` for your own WiFi. Never commit real passwords to a public repository.
5. Open Serial Monitor at 115200 baud.

## PlatformIO
Open this folder as a PlatformIO project and build/upload the `esp32dev` environment.

## GitHub submission
Create a **public** repository named `Level-3-Network-Connected-Device` and upload all four files to the repository root. Add your captured screenshots under `evidence/` before submitting the repository URL.
