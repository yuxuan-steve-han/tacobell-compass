# Taco Bell Compass

A handheld compass that points to the nearest Taco Bell.

## Hardware

- Waveshare ESP32-S3-Touch-LCD-1.28
- ATGM336H GPS (TX to GPIO 17)
- QMC5883L (GY-271) compass
- 103035 LiPo, about 1000 mAh

## Libraries

Install from the Arduino IDE:

- `esp32` by Espressif Systems (Boards Manager)
- `TFT_eSPI` by Bodmer (configure `User_Setup.h` for the GC9A01 display)
- `TinyGPSPlus` by Mikal Hart

## Build

Open `tacobell_compass.ino`, select the ESP32-S3 board, enable PSRAM, and upload.

`case.scad` / `case.stl` is the enclosure. `tools/` regenerates the logo and font headers.
