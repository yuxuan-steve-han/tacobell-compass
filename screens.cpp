#include "screens.h"
#include "graphics.h"
#include "tacobell_logo.h"
#include <math.h>

namespace {

const float DEG = 0.017453292f;

// Loading screen layout. The intro animates the logo into this exact spot.
const int LOGO_SCALE = 2;
const float LOGO_CY = 28 + TB_LOGO_H * LOGO_SCALE / 2.0f;
const int TEXT_SCALE = 2;

// The intro's ring sweep ends where and as fast as the spinner starts.
const int SPINNER_LEN = 90;
const int SPINNER_START = 360 - SPINNER_LEN;
const uint32_t SPINNER_LAP_MS = 2160;

const uint32_t POP_END = 450, HOLD_END = 1200, SHRINK_END = 1900;
const float BIG_SCALE = 3.0f, BIG_CY = CY - 8;

const uint32_t LETTER_STAGGER_MS = 15, LETTER_DROP_MS = 140, SATS_DELAY_MS = 100;
const int LETTER_DROP_PX = 6;

const int INNER_R_OUT = 72, INNER_R_IN = 69;

float easeOutBack(float p) {
  const float c1 = 1.70158f, c3 = c1 + 1.0f;
  float q = p - 1.0f;
  return 1.0f + c3 * q * q * q + c1 * q * q;
}

float easeOutCubic(float p) {
  float q = 1.0f - p;
  return 1.0f - q * q * q;
}

float easeInOutCubic(float p) {
  return p < 0.5f ? 4 * p * p * p : 1 - powf(-2 * p + 2, 3) / 2;
}

// Point r px from the centre at deg clockwise from the top.
void polar(float deg, float r, int &x, int &y) {
  x = CX + (int)roundf(r * sinf(deg * DEG));
  y = CY - (int)roundf(r * cosf(deg * DEG));
}

// Letters drop in one at a time, snapped to whole font pixels.
// t < 0 means the reveal hasn't started yet.
int drawTextReveal(TFT_eSprite &spr, const char *s, int x, int y, int scale, bool shadowed,
                   uint16_t color, int32_t t) {
  char ch[2] = {0, 0};
  for (int i = 0; s[i]; i++) {
    ch[0] = s[i];
    int32_t local = t - i * (int32_t)LETTER_STAGGER_MS;
    int next = x + pixelTextWidth(ch, scale) + scale;
    if (local >= 0) {
      float p = local >= (int32_t)LETTER_DROP_MS ? 1.0f : (float)local / LETTER_DROP_MS;
      int dy = (int)roundf((1.0f - easeOutCubic(p)) * LETTER_DROP_PX / scale) * scale;
      if (shadowed) drawPixelTextShadowed(spr, ch, x, y - dy, scale);
      else drawPixelText(spr, ch, x, y - dy, scale, color);
    }
    x = next;
  }
  return x;
}

void drawLogoCentered(TFT_eSprite &spr, float cy, float scale) {
  drawLogo(spr, CX - (int)roundf(TB_LOGO_W * scale / 2), (int)roundf(cy - TB_LOGO_H * scale / 2), scale);
}

void drawRose(TFT_eSprite &spr, float heading) {
  drawRingArc(spr, 0, 360, C_RING);
  for (int d = 30; d < 360; d += 30) {
    if (d % 90 == 0) continue;
    int x, y;
    polar(d - heading, (RING_R_OUT + RING_R_IN) / 2, x, y);
    spr.fillRect(x - 2, y - 2, 4, 4, C_BG);
  }
  const char *names[] = {"N", "E", "S", "W"};
  for (int i = 0; i < 4; i++) {
    int x, y;
    polar(i * 90 - heading, 100, x, y);
    drawPixelTextCentered(spr, names[i], x, y - FONT_H, 2, i == 0 ? C_ACCENT : C_TEXT);
  }
}

// Triangle fan from the tip to an arc on the inner ring, so the base sits flush with it.
void drawMarker(TFT_eSprite &spr, float deg, uint32_t t) {
  const int HALF = 12, STEP = 3;
  const float baseR = (INNER_R_OUT + INNER_R_IN) / 2.0f;
  float tip = 88 + (t / 300 % 2) * 3;
  int tx, ty, ax, ay, bx, by;
  polar(deg, tip, tx, ty);
  polar(deg - HALF, baseR, ax, ay);
  for (int a = -HALF + STEP; a <= HALF; a += STEP) {
    polar(deg + a, baseR, bx, by);
    spr.fillTriangle(tx, ty, ax, ay, bx, by, C_ACCENT);
    ax = bx;
    ay = by;
  }
}

// Big number + small unit with bottoms aligned; long numbers drop a size to fit the inner ring.
void drawDistance(TFT_eSprite &spr, float m, int y) {
  char num[12];
  const char *unit = "MI";
  float mi = m / 1609.344f;
  if (mi < 0.1f) {
    snprintf(num, sizeof(num), "%d", (int)roundf(m * 3.28084f));
    unit = "FT";
  } else if (mi < 10.0f) {
    snprintf(num, sizeof(num), "%.1f", mi);
  } else {
    snprintf(num, sizeof(num), "%d", (int)roundf(mi));
  }

  int big = pixelTextWidth(num, 4) > 96 ? 3 : 4;
  int gap = 8;
  int x = CX - (pixelTextWidth(num, big) + gap + pixelTextWidth(unit, 2)) / 2;
  x = drawPixelTextShadowed(spr, num, x, y, big) - big;
  drawPixelText(spr, unit, x + gap, y + FONT_H * (big - 2), 2, C_TEXT);
}

} // namespace

bool drawIntroScreen(TFT_eSprite &spr, uint32_t t) {
  float scale = BIG_SCALE, cy = BIG_CY;
  int sweep = 0;
  if (t < POP_END) {
    scale = BIG_SCALE * easeOutBack((float)t / POP_END);
  } else if (t >= HOLD_END) {
    float p = t >= SHRINK_END ? 1.0f : (float)(t - HOLD_END) / (SHRINK_END - HOLD_END);
    float e = easeInOutCubic(p);
    scale += (LOGO_SCALE - BIG_SCALE) * e;
    cy += (LOGO_CY - BIG_CY) * e;
    // Hermite curve: starts at rest, ends at the spinner's speed
    float v1 = (float)(SHRINK_END - HOLD_END) / SPINNER_LAP_MS;
    sweep = (int)(360 * ((3 - 2 * p) * p * p + v1 * (p * p * p - p * p)));
  }

  spr.fillSprite(C_BG);
  int head = sweep < SPINNER_LEN ? sweep : SPINNER_LEN;
  drawRingArc(spr, 0, sweep, C_RING);
  drawRingArc(spr, sweep - head, head, C_ACCENT);
  if (scale > 0.05f) drawLogoCentered(spr, cy, scale);
  spr.pushSprite(0, 0);
  return t >= SHRINK_END;
}

void drawLoadingScreen(TFT_eSprite &spr, uint32_t t, int satellites) {
  spr.fillSprite(C_BG);
  drawRingArc(spr, 0, 360, C_RING);
  drawRingArc(spr, SPINNER_START + t % SPINNER_LAP_MS * 360 / SPINNER_LAP_MS, SPINNER_LEN, C_ACCENT);

  // bob in whole logo pixels so the grid stays crisp
  int bob = (int)roundf(sinf(t / 300.0f)) * LOGO_SCALE;
  drawLogoCentered(spr, LOGO_CY + bob, LOGO_SCALE);

  // left edge fixed so the text doesn't shift as the dots animate
  char buf[24];
  snprintf(buf, sizeof(buf), "TRIANGULATING%.*s", (int)(t / 400 % 4), "...");
  drawTextReveal(spr, buf, CX - pixelTextWidth("TRIANGULATING...", TEXT_SCALE) / 2, 152,
                 TEXT_SCALE, true, C_TEXT, t);

  snprintf(buf, sizeof(buf), "%d SATELLITE%s", satellites, satellites == 1 ? "" : "S");
  drawTextReveal(spr, buf, CX - pixelTextWidth(buf, TEXT_SCALE) / 2, 180,
                 TEXT_SCALE, false, C_HIGHLIGHT, (int32_t)t - (int32_t)SATS_DELAY_MS);

  spr.pushSprite(0, 0);
}

void drawCompassScreen(TFT_eSprite &spr, uint32_t t, float distanceM, float bearingDeg, float headingDeg) {
  bool noCompass = isnan(headingDeg);
  float heading = noCompass ? 0.0f : headingDeg;

  spr.fillSprite(C_BG);
  drawRose(spr, heading);
  spr.drawSmoothArc(CX, CY, INNER_R_OUT, INNER_R_IN, 0, 360, C_TEXT, C_BG, false);
  drawMarker(spr, bearingDeg - heading, t);

  drawPixelTextCentered(spr, "LOCATED!", CX, CY - 36, 2, C_HIGHLIGHT);
  drawDistance(spr, distanceM, CY - 10);
  if (noCompass) drawPixelTextCentered(spr, "NORTH UP", CX, CY + 30, 1, C_RING);

  spr.pushSprite(0, 0);
}
