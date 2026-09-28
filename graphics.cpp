#include "graphics.h"
#include "pixel_font.h"
#include "tacobell_logo.h"
#include <math.h>

static_assert(FONT_H == PF_HEIGHT, "FONT_H must match pixel_font.h");

namespace {

// One 1-bit bitmap row (MSB = leftmost pixel), filled as horizontal runs.
void drawBitRow(TFT_eSprite &spr, const uint8_t *bits, int w, int x, int y0, int y1,
                float scale, uint16_t color) {
  int c = 0;
  while (c < w) {
    if (!(bits[c >> 3] & (0x80 >> (c & 7)))) { c++; continue; }
    int start = c;
    while (c < w && (bits[c >> 3] & (0x80 >> (c & 7)))) c++;
    int x0 = x + (int)floorf(start * scale), x1 = x + (int)floorf(c * scale);
    if (x1 > x0) spr.fillRect(x0, y0, x1 - x0, y1 - y0, color);
  }
}

void drawLogoLayer(TFT_eSprite &spr, int x, int y, float scale, uint16_t color) {
  for (int r = 0; r < TB_LOGO_H; r++) {
    int y0 = y + (int)floorf(r * scale), y1 = y + (int)floorf((r + 1) * scale);
    if (y1 > y0) drawBitRow(spr, TB_LOGO + r * TB_LOGO_STRIDE, TB_LOGO_W, x, y0, y1, scale, color);
  }
}

const uint8_t *glyphFor(char ch) {
  if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
  if (ch < PF_FIRST || ch > PF_LAST || PF_GLYPHS[ch - PF_FIRST][0] == 0) ch = ' ';
  return PF_GLYPHS[ch - PF_FIRST];
}

} // namespace

void drawRingArc(TFT_eSprite &spr, int startDeg, int lenDeg, uint16_t color) {
  if (lenDeg <= 0) return;
  if (lenDeg >= 360) {
    spr.drawSmoothArc(CX, CY, RING_R_OUT, RING_R_IN, 0, 360, color, C_BG, false);
    return;
  }
  // TFT_eSPI measures from the bottom; end < start is drawn as one arc through 0
  int s = ((startDeg + 180) % 360 + 360) % 360;
  int e = s + lenDeg;
  if (e > 360) e -= 360;
  spr.drawSmoothArc(CX, CY, RING_R_OUT, RING_R_IN, s, e, color, C_BG, true);
}

void drawLogo(TFT_eSprite &spr, int x, int y, float scale) {
  int shadow = (int)roundf(scale);
  drawLogoLayer(spr, x + shadow, y + shadow, scale, C_ACCENT);
  drawLogoLayer(spr, x, y, scale, C_TEXT);
}

int pixelTextWidth(const char *s, int scale) {
  int w = 0;
  for (; *s; s++) w += (glyphFor(*s)[0] + 1) * scale;
  return w > 0 ? w - scale : 0;
}

int drawPixelText(TFT_eSprite &spr, const char *s, int x, int y, int scale, uint16_t color) {
  for (; *s; s++) {
    const uint8_t *g = glyphFor(*s);
    for (int r = 0; r < FONT_H; r++) {
      drawBitRow(spr, &g[1 + r], g[0], x, y + r * scale, y + (r + 1) * scale, scale, color);
    }
    x += (g[0] + 1) * scale;
  }
  return x;
}

int drawPixelTextShadowed(TFT_eSprite &spr, const char *s, int x, int y, int scale) {
  drawPixelText(spr, s, x + scale, y + scale, scale, C_ACCENT);
  return drawPixelText(spr, s, x, y, scale, C_TEXT);
}

void drawPixelTextCentered(TFT_eSprite &spr, const char *s, int cx, int y, int scale, uint16_t color) {
  drawPixelText(spr, s, cx - pixelTextWidth(s, scale) / 2, y, scale, color);
}
