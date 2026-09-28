#include "compass.h"
#include <math.h>

// Magnetic declination at your location, east positive:
// https://www.ngdc.noaa.gov/geomag/calculators/magcalc.shtml
const float DECLINATION_DEG = 0.0f;

namespace {

bool present = false;

// TODO: read the magnetometer and set deg = atan2 of its horizontal axes, oriented so
// that 0 means the top of the screen faces magnetic north. Return false on a failed read.
bool readMagneticHeading(float &deg) {
  (void)deg;
  return false;
}

} // namespace

bool compassBegin() {
  // TODO: initialise the magnetometer (I2C bus is shared with the QMI8658; see DEV_I2C_Init).
  present = false;
  return present;
}

float compassHeading() {
  float mag;
  if (!present || !readMagneticHeading(mag)) return NAN;
  float h = fmodf(mag + DECLINATION_DEG, 360.0f);
  return h < 0 ? h + 360.0f : h;
}
