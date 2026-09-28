# Taco Bell Compass

A handheld compass that points to the nearest Taco Bell.

## Hardware

- Waveshare ESP32-S3-Touch-LCD-1.28
- ATGM336H GPS (TX to GPIO 17)
- QMC5883L (GY-271) compass
- 103035 LiPo, about 1000 mAh

## Build

Install `TFT_eSPI` (configured for the GC9A01 display) and `TinyGPSPlus`, then open `tacobell_compass.ino` in the Arduino IDE. Enable PSRAM.

`case.scad` / `case.stl` is the enclosure. `tools/` regenerates the logo and font headers.
