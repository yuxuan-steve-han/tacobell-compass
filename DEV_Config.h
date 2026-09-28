// I2C helpers required by Waveshare's QMI8658 driver (display SPI is handled by TFT_eSPI).
#pragma once
#include <Arduino.h>
#include <Wire.h>

#define DEV_SDA_PIN (6)
#define DEV_SCL_PIN (7)

void DEV_I2C_Init(void);
void DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t value);
void DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t len);
