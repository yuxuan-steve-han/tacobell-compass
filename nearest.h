#pragma once

struct Nearest {
  float distanceM;
  float bearingDeg;   // clockwise from true north
};

Nearest findNearestTacoBell(float lat, float lon);
