#pragma once
#include <TFT_eSPI.h>

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t C_BG        = rgb565(54, 57, 154);   // #36399a
constexpr uint16_t C_ACCENT    = rgb565(239, 24, 151);  // #ef1897
constexpr uint16_t C_HIGHLIGHT = rgb565(254, 224, 18);  // #fee012
constexpr uint16_t C_TEXT      = rgb565(255, 255, 255); // #ffffff
constexpr uint16_t C_RING      = rgb565(167, 123, 202); // #a77bca

constexpr int CX = 120, CY = 120;
constexpr int RING_R_OUT = 118, RING_R_IN = 110;
constexpr int FONT_H = 7;

// Angles are degrees clockwise from the top and may wrap past 360.
void drawRingArc(TFT_eSprite &spr, int startDeg, int lenDeg, uint16_t color);

// Top-left at (x, y), white with a magenta drop shadow. Any scale > 0 works.
void drawLogo(TFT_eSprite &spr, int x, int y, float scale);

// Pixel font: A-Z (lowercase drawn as uppercase), 0-9, . ! : - and space.
int pixelTextWidth(const char *s, int scale);
int drawPixelText(TFT_eSprite &spr, const char *s, int x, int y, int scale, uint16_t color);
int drawPixelTextShadowed(TFT_eSprite &spr, const char *s, int x, int y, int scale);
void drawPixelTextCentered(TFT_eSprite &spr, const char *s, int cx, int y, int scale, uint16_t color);
