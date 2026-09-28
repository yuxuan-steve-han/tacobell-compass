#pragma once
#include <Arduino.h>

// Tilt-compensated compass: QMC5883L magnetometer (Wire1, GPIO 15/16, powered from VSYS)
// plus the board's QMI8658 accelerometer (Wire, GPIO 6/7).

// Starts both I2C buses and loads the saved calibration. False if either sensor is missing.
bool compassBegin();

// Reads both sensors and updates the heading; call 20-50 times per second.
bool compassUpdate();

// Degrees clockwise from true north that the top of the screen faces, or NAN if unavailable.
float compassHeading();

bool compassIsCalibrated();

// Blocking: rotate the device slowly in every direction for `ms`, then the result is saved.
void compassCalibrate(uint32_t ms);

// Detection result for the Serial log.
const char *compassStatus();

// Mapped accel/mag values, for checking axis directions.
void compassDebugPrint();
