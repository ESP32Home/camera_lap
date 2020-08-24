# camera_lap
ESP32 AI Thinker camera application that publishes an mqtt jpg image every lap time and then go to sleep

## Board
* esp32cam
* define : CAMERA_MODEL_AI_THINKER

## sensor
* OV2640

## Runtime

| step ready | Timestamp sec |
|-----------|------|
| Flash Boot | 1.447 |
| Config | 1.523 |
| Camera | 1.873 |
| Wifi | 6.094 |
| MQTT | 6.139 |
| Capture | 6.263 |
| Publish | ? |
