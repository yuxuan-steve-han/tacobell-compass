#include "nearest.h"
#include "tacobell.h"
#include <math.h>

namespace {
const float DEG = 0.017453292f;
const float EARTH_R = 6371000.0f;
}

Nearest findNearestTacoBell(float lat, float lon) {
  float phi1 = lat * DEG, cosPhi1 = cosf(phi1);

  // The haversine term grows monotonically with distance, so rank by it directly.
  int best = 0;
  float bestA = 2.0f;
  for (int i = 0; i < TACOBELL_COUNT; i++) {
    float phi2 = TACOBELL_LOCATIONS[i][0] * DEG;
    float sLat = sinf((phi2 - phi1) * 0.5f);
    float sLon = sinf((TACOBELL_LOCATIONS[i][1] - lon) * DEG * 0.5f);
    float a = sLat * sLat + cosPhi1 * cosf(phi2) * sLon * sLon;
    if (a < bestA) {
      bestA = a;
      best = i;
    }
  }

  float phi2 = TACOBELL_LOCATIONS[best][0] * DEG;
  float dLon = (TACOBELL_LOCATIONS[best][1] - lon) * DEG;
  float brg = atan2f(sinf(dLon) * cosf(phi2),
                     cosPhi1 * sinf(phi2) - sinf(phi1) * cosf(phi2) * cosf(dLon)) / DEG;
  if (brg < 0) brg += 360.0f;

  return {2.0f * EARTH_R * asinf(sqrtf(bestA)), brg};
}
