#pragma once
#include <TFT_eSPI.h>

// Each call draws one full frame into spr and pushes it. t = ms since the screen was entered.

// Returns true once the intro has finished.
bool drawIntroScreen(TFT_eSprite &spr, uint32_t t);

void drawLoadingScreen(TFT_eSprite &spr, uint32_t t, int satellites);

// bearingDeg: direction to the Taco Bell, clockwise from true north.
// headingDeg: direction the top of the screen faces; NAN = no compass (drawn north-up).
void drawCompassScreen(TFT_eSprite &spr, uint32_t t, float distanceM, float bearingDeg, float headingDeg);
