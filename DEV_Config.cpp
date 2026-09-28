#include "DEV_Config.h"

void DEV_I2C_Init(void) {
  Wire.setPins(DEV_SDA_PIN, DEV_SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);
}

void DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint16_t)addr, (size_t)len);
  for (uint32_t i = 0; i < len; i++) {
    pData[i] = Wire.available() ? Wire.read() : 0;
  }
}
