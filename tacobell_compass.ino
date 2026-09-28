#include <TFT_eSPI.h>
#include <TinyGPSPlus.h>

#include "compass.h"
#include "nearest.h"
#include "screens.h"

#define GPS_RX_PIN 17   // <- GPS TX
#define GPS_TX_PIN -1   // not connected
#define GPS_BAUD   9600
#define GPS_DEBUG  1    // echo raw NMEA and a status line to Serial

const uint32_t FRAME_MS = 33;
const uint32_t FIX_TIMEOUT_MS = 5000;
const uint32_t SEARCH_MS = 1000;

enum class Screen { Intro, Loading, Compass };

TFT_eSPI tft;
TFT_eSprite spr = TFT_eSprite(&tft);
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  if (!compassBegin()) Serial.println("No compass found, drawing north-up");

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // 16-bit needs ~115 KB (enable PSRAM); fall back to 8-bit if that fails
  spr.setColorDepth(16);
  if (!spr.createSprite(240, 240)) {
    Serial.println("16-bit sprite failed, falling back to 8-bit");
    spr.setColorDepth(8);
    spr.createSprite(240, 240);
  }
}

void readGps() {
  while (gpsSerial.available()) {
    char ch = gpsSerial.read();
    if (GPS_DEBUG) Serial.write(ch);
    gps.encode(ch);
  }
}

// chars=0: nothing on the RX pin. Many failed checksums: wrong baud rate.
void printGpsStatus(uint32_t now) {
  static uint32_t last = 0;
  if (!GPS_DEBUG || now - last < 5000) return;
  last = now;
  Serial.printf("\n[gps] chars=%lu ok=%lu failed=%lu sats=%d fix=%d\n",
                (unsigned long)gps.charsProcessed(), (unsigned long)gps.passedChecksum(),
                (unsigned long)gps.failedChecksum(),
                gps.satellites.isValid() ? (int)gps.satellites.value() : -1,
                gps.location.isValid() ? 1 : 0);
}

void loop() {
  readGps();
  uint32_t now = millis();
  printGpsStatus(now);

  bool fix = gps.location.isValid() && gps.location.age() < FIX_TIMEOUT_MS;

  static Nearest nearest;
  static uint32_t lastSearch = 0;
  if (fix && (lastSearch == 0 || now - lastSearch >= SEARCH_MS)) {
    nearest = findNearestTacoBell(gps.location.lat(), gps.location.lng());
    lastSearch = now;
  }

  static uint32_t lastFrame = 0;
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  static Screen screen = Screen::Intro;
  static uint32_t screenStart = now;
  uint32_t t = now - screenStart;
  Screen next = screen;

  switch (screen) {
    case Screen::Intro:
      if (drawIntroScreen(spr, t)) next = Screen::Loading;
      break;
    case Screen::Loading:
      drawLoadingScreen(spr, t, gps.satellites.isValid() ? gps.satellites.value() : 0);
      if (fix) next = Screen::Compass;
      break;
    case Screen::Compass:
      drawCompassScreen(spr, t, nearest.distanceM, nearest.bearingDeg, compassHeading());
      if (!fix) next = Screen::Loading;
      break;
  }

  if (next != screen) {
    screen = next;
    screenStart = now;
  }
}
