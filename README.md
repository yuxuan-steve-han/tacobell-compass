# Taco Bell Compass

A handheld compass that points to the nearest Taco Bell.

## Hardware

- Waveshare ESP32-S3-Touch-LCD-1.28
- ATGM336H GPS (TX to GPIO 17)
- QMC5883P (GY-271) compass (SDA to GPIO 33, SCL to GPIO 18, VCC to VSYS); QMC5883L and HMC5883L boards also work
- Buzzer on GPIO 15
- 103035 LiPo, about 1000 mAh

## Libraries

Install from the Arduino IDE:

- `esp32` by Espressif Systems (Boards Manager)
- `TFT_eSPI` by Bodmer (configure `User_Setup.h` for the GC9A01 display)
- `TinyGPSPlus` by Mikal Hart

## Build

Open `tacobell_compass.ino`, select the ESP32-S3 board, enable PSRAM, and upload.

To calibrate the compass, send `c` in the Serial Monitor (115200 baud) and rotate the device in every direction for 20 seconds.

`case.scad` / `case.stl` is the enclosure. `tools/` regenerates the logo, font and Taco Bell location headers (`python3 tools/make_tacobell.py tacobell.h` pulls the latest from OpenStreetMap; add missing places to `EXTRA` in that script).
