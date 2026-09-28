#include "compass.h"
#include <Preferences.h>
#include <Wire.h>

const int QMC_SDA = 15, QMC_SCL = 16;
const int IMU_SDA = 6, IMU_SCL = 7;

// Magnetic declination, east positive. Boston is about 14 degrees west:
// https://www.ngdc.noaa.gov/geomag/calculators/magcalc.shtml
const float DECLINATION_DEG = -14.2f;
const float SMOOTHING = 0.2f;        // 0..1, lower = smoother but slower needle
const uint32_t STALE_MS = 500;

namespace {

// Device frame: X = right, Y = front (top of the screen), Z = out of the screen.
// Each entry picks the raw sensor axis (0=x, 1=y, 2=z) feeding that device axis, and its sign.
// Defaults assume the GY-271's X arrow points to the front and the board lies flat;
// verify with compassDebugPrint(). The QMC5883P's axes differ from the L, so re-check if it's fitted.
struct AxisMap { uint8_t idx; int8_t sign; };
const AxisMap MAG_MAP[3] = {{1, -1}, {0, +1}, {2, +1}};
const AxisMap ACC_MAP[3] = {{0, +1}, {1, +1}, {2, +1}};

// GY-271 / HW-246 boards ship with any of these.
enum class MagChip { None, QMC5883L, QMC5883P, HMC5883L };
MagChip chip = MagChip::None;
uint8_t magAddr = 0;

uint8_t imuAddr = 0x6A;
bool magOk = false, imuOk = false;
float magOff[3] = {0, 0, 0}, magScale[3] = {1, 1, 1};
bool calibrated = false;
float acc[3], mag[3];
float sinH = 0, cosH = 1, heading = NAN;
uint32_t lastGood = 0;
char status[128] = "not started";

void writeReg(TwoWire &bus, uint8_t addr, uint8_t reg, uint8_t val) {
  bus.beginTransmission(addr);
  bus.write(reg);
  bus.write(val);
  bus.endTransmission();
}

bool readRegs(TwoWire &bus, uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t n) {
  bus.beginTransmission(addr);
  bus.write(reg);
  if (bus.endTransmission(false) != 0) return false;
  if (bus.requestFrom(addr, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = bus.read();
  return true;
}

bool readVec16(TwoWire &bus, uint8_t addr, uint8_t reg, float out[3]) {
  uint8_t b[6];
  if (!readRegs(bus, addr, reg, b, 6)) return false;
  for (int i = 0; i < 3; i++) out[i] = (int16_t)(b[2 * i] | (b[2 * i + 1] << 8));
  return true;
}

void mapAxes(const float raw[3], const AxisMap m[3], float out[3]) {
  for (int i = 0; i < 3; i++) out[i] = raw[m[i].idx] * m[i].sign;
}

bool probe(TwoWire &bus, uint8_t addr) {
  bus.beginTransmission(addr);
  return bus.endTransmission() == 0;
}

const char *chipName() {
  switch (chip) {
    case MagChip::QMC5883L: return "QMC5883L";
    case MagChip::QMC5883P: return "QMC5883P";
    case MagChip::HMC5883L: return "HMC5883L";
    default: return "none";
  }
}

bool magInit() {
  uint8_t id = 0;
  if (probe(Wire1, 0x0D)) {
    chip = MagChip::QMC5883L;
    magAddr = 0x0D;
    writeReg(Wire1, magAddr, 0x0B, 0x01);   // set/reset period
    writeReg(Wire1, magAddr, 0x09, 0x1D);   // continuous, 200 Hz, 8 G, 512 oversampling
  } else if (probe(Wire1, 0x2C) && readRegs(Wire1, 0x2C, 0x00, &id, 1) && id == 0x80) {
    chip = MagChip::QMC5883P;
    magAddr = 0x2C;
    writeReg(Wire1, magAddr, 0x29, 0x06);   // axis sign definition, per datasheet
    writeReg(Wire1, magAddr, 0x0B, 0x08);   // set/reset on, 8 G
    writeReg(Wire1, magAddr, 0x0A, 0xCD);   // normal mode, 200 Hz, oversampling 8
  } else if (probe(Wire1, 0x1E)) {
    chip = MagChip::HMC5883L;
    magAddr = 0x1E;
    writeReg(Wire1, magAddr, 0x00, 0x70);   // 8 samples averaged, 15 Hz
    writeReg(Wire1, magAddr, 0x01, 0x20);   // +/-1.3 G
    writeReg(Wire1, magAddr, 0x02, 0x00);   // continuous
  } else {
    return false;
  }
  delay(10);
  return true;
}

bool magReadRaw(float out[3]) {
  switch (chip) {
    case MagChip::QMC5883L: return readVec16(Wire1, magAddr, 0x00, out);
    case MagChip::QMC5883P: return readVec16(Wire1, magAddr, 0x01, out);
    case MagChip::HMC5883L: {
      uint8_t b[6];   // big-endian, ordered X, Z, Y
      if (!readRegs(Wire1, magAddr, 0x03, b, 6)) return false;
      out[0] = (int16_t)(b[0] << 8 | b[1]);
      out[2] = (int16_t)(b[2] << 8 | b[3]);
      out[1] = (int16_t)(b[4] << 8 | b[5]);
      return true;
    }
    default: return false;
  }
}

// A chip that answers but reads all 0xFF or never changes (e.g. a QMC5882A at 0x0D) is unusable.
bool magReturnsData() {
  float first[3], cur[3];
  if (!magReadRaw(first)) return false;
  for (int i = 0; i < 20; i++) {
    delay(15);
    if (!magReadRaw(cur)) return false;
    bool allFF = cur[0] == -1 && cur[1] == -1 && cur[2] == -1;
    if (!allFF && (cur[0] != first[0] || cur[1] != first[1] || cur[2] != first[2])) return true;
  }
  return false;
}

// Accelerometer only; register values follow Waveshare's QMI8658 driver.
bool imuInit() {
  for (uint8_t a : {0x6A, 0x6B}) {
    uint8_t who = 0;
    if (readRegs(Wire, a, 0x00, &who, 1) && who == 0x05) {
      imuAddr = a;
      writeReg(Wire, imuAddr, 0x02, 0x60);   // CTRL1: address auto-increment
      writeReg(Wire, imuAddr, 0x03, 0x06);   // CTRL2: +/-2 g, 125 Hz
      writeReg(Wire, imuAddr, 0x06, 0x00);   // CTRL5: filters off
      writeReg(Wire, imuAddr, 0x08, 0x01);   // CTRL7: accelerometer only
      delay(20);
      return true;
    }
  }
  return false;
}

void describeBus(TwoWire &bus, char *out, size_t n) {
  int len = snprintf(out, n, "bus has:");
  int found = 0;
  for (uint8_t a = 1; a < 127 && len < (int)n - 6; a++) {
    bus.beginTransmission(a);
    if (bus.endTransmission() == 0) {
      len += snprintf(out + len, n - len, " 0x%02X", a);
      found++;
    }
  }
  if (!found) snprintf(out + len, n - len, " nothing");
}

void loadCal() {
  Preferences p;
  p.begin("compass", true);
  calibrated = p.getBool("valid", false);
  if (calibrated) {
    p.getBytes("off", magOff, sizeof(magOff));
    p.getBytes("scale", magScale, sizeof(magScale));
  }
  p.end();
}

void saveCal() {
  Preferences p;
  p.begin("compass", false);
  p.putBytes("off", magOff, sizeof(magOff));
  p.putBytes("scale", magScale, sizeof(magScale));
  p.putBool("valid", true);
  p.end();
  calibrated = true;
}

} // namespace

bool compassBegin() {
  // The module's pull-ups hold an idle bus high; low means unpowered or not wired to these pins.
  pinMode(QMC_SDA, INPUT);
  pinMode(QMC_SCL, INPUT);
  delay(2);
  int sdaIdle = digitalRead(QMC_SDA), sclIdle = digitalRead(QMC_SCL);

  Wire.begin(IMU_SDA, IMU_SCL, 400000);
  Wire1.begin(QMC_SDA, QMC_SCL, 400000);
  bool found = magInit();
  magOk = found && magReturnsData();
  imuOk = imuInit();
  loadCal();

  if (found && !magOk) {
    snprintf(status, sizeof(status), "%s at 0x%02X answers but sends no data (QMC5882A or faulty?)",
             chipName(), magAddr);
  } else if (magOk && imuOk) {
    snprintf(status, sizeof(status), "%s at 0x%02X ok (%s)", chipName(), magAddr,
             calibrated ? "calibrated" : "not calibrated, send 'c'");
  } else if (!magOk) {
    char bus[40];
    describeBus(Wire1, bus, sizeof(bus));
    snprintf(status, sizeof(status), "no known magnetometer, %s, idle SDA=%d SCL=%d%s", bus,
             sdaIdle, sclIdle, sdaIdle && sclIdle ? "" : " (not powered or not wired)");
  } else {
    snprintf(status, sizeof(status), "QMI8658 accelerometer not found");
  }
  return magOk && imuOk;
}

bool compassUpdate() {
  if (!magOk || !imuOk) return false;
  float rawM[3], rawA[3];
  if (!magReadRaw(rawM) || !readVec16(Wire, imuAddr, 0x35, rawA)) return false;

  float m[3];
  mapAxes(rawM, MAG_MAP, m);
  for (int i = 0; i < 3; i++) mag[i] = (m[i] - magOff[i]) * magScale[i];
  mapAxes(rawA, ACC_MAP, acc);

  // At rest the accelerometer reports "up", so down = -acc. east = down x mag, north = east x down.
  float d[3] = {-acc[0], -acc[1], -acc[2]};
  float e[3] = {d[1] * mag[2] - d[2] * mag[1],
                d[2] * mag[0] - d[0] * mag[2],
                d[0] * mag[1] - d[1] * mag[0]};
  float n[3] = {e[1] * d[2] - e[2] * d[1],
                e[2] * d[0] - e[0] * d[2],
                e[0] * d[1] - e[1] * d[0]};
  float en = sqrtf(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
  float nn = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
  if (en < 1e-6f || nn < 1e-6f) return false;

  // front axis (0, 1, 0) projected onto east and north
  float h = atan2f(e[1] / en, n[1] / nn) * RAD_TO_DEG + DECLINATION_DEG;

  // smooth on the unit circle so 359 -> 0 doesn't swing the long way round
  float s = sinf(h * DEG_TO_RAD), c = cosf(h * DEG_TO_RAD);
  if (isnan(heading)) {
    sinH = s;
    cosH = c;
  } else {
    sinH += (s - sinH) * SMOOTHING;
    cosH += (c - cosH) * SMOOTHING;
  }
  heading = atan2f(sinH, cosH) * RAD_TO_DEG;
  if (heading < 0) heading += 360.0f;
  lastGood = millis();
  return true;
}

float compassHeading() {
  if (isnan(heading) || millis() - lastGood > STALE_MS) return NAN;
  return heading;
}

bool compassIsCalibrated() {
  return calibrated;
}

void compassCalibrate(uint32_t ms) {
  if (!magOk) return;
  float mn[3] = {1e9, 1e9, 1e9}, mx[3] = {-1e9, -1e9, -1e9};
  uint32_t start = millis();
  while (millis() - start < ms) {
    float raw[3], m[3];
    if (magReadRaw(raw)) {
      mapAxes(raw, MAG_MAP, m);
      for (int i = 0; i < 3; i++) {
        mn[i] = min(mn[i], m[i]);
        mx[i] = max(mx[i], m[i]);
      }
    }
    delay(10);
  }
  // hard-iron offset plus simple per-axis soft-iron scaling
  float span[3], avg = 0;
  for (int i = 0; i < 3; i++) {
    magOff[i] = (mx[i] + mn[i]) / 2.0f;
    span[i] = max((mx[i] - mn[i]) / 2.0f, 1.0f);
    avg += span[i] / 3.0f;
  }
  for (int i = 0; i < 3; i++) magScale[i] = avg / span[i];
  saveCal();
  snprintf(status, sizeof(status), "%s at 0x%02X ok (calibrated)", chipName(), magAddr);
}

const char *compassStatus() {
  return status;
}

void compassDebugPrint() {
  Serial.printf("acc X %7.0f Y %7.0f Z %7.0f | mag X %7.0f Y %7.0f Z %7.0f | heading %5.1f\n",
                acc[0], acc[1], acc[2], mag[0], mag[1], mag[2], heading);
}
