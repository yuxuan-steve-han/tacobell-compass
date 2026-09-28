#include <TFT_eSPI.h>
#include <TinyGPSPlus.h>
#include <Wire.h>

#include "compass.h"
#include "nearest.h"
#include "screens.h"

#define GPS_RX_PIN 17   // <- GPS TX
#define GPS_TX_PIN -1   // not connected
#define GPS_BAUD   9600
#define GPS_DEBUG  0   // echo raw NMEA and a byte/checksum count line to Serial
#define BUZZER_PIN 15
#define PIN_SCAN   1   // at boot, report what's wired to each header GPIO

const bool SCREEN_FLIPPED = true;   // display rotated 180 degrees; the heading follows it
const uint32_t FRAME_MS = 33;
const uint32_t FIX_TIMEOUT_MS = 5000;
const uint32_t SEARCH_MS = 1000;

enum class Screen { Intro, Loading, Compass };

TFT_eSPI tft;
TFT_eSprite spr = TFT_eSprite(&tft);
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
// Signal strength (SNR, dB; 0 = not heard) of each satellite in view, from the GSV sentences.
// gps.satellites only counts those used in a fix, which stays 0 until the first one.
const int MAX_SATS = 32;
const int GOOD_SNR_DB = 25;   // roughly what a fix needs, from at least 4 satellites
struct SatList {
  int snr[MAX_SATS];
  int n = 0;
};
SatList satsShown[2], satsBuilding[2];   // [0] = GPS, [1] = BeiDou

// Fields: $xxGSV,total msgs,msg num,in view,{prn,elevation,azimuth,snr} x up to 4[,signal id]
void parseGsv(char *line) {
  int sys = !strncmp(line, "$GPGSV", 6) ? 0
          : !strncmp(line, "$BDGSV", 6) || !strncmp(line, "$GBGSV", 6) ? 1 : -1;
  if (sys < 0) return;
  char *star = strchr(line, '*');
  if (star) *star = 0;
  char *f[24];
  int nf = 0;
  for (char *p = line; p && nf < 24;) {
    f[nf++] = p;
    p = strchr(p, ',');
    if (p) *p++ = 0;
  }
  if (nf < 4) return;
  int total = atoi(f[1]), num = atoi(f[2]);
  SatList &b = satsBuilding[sys];
  if (num == 1) b.n = 0;
  for (int i = 4; i + 3 < nf && b.n < MAX_SATS; i += 4) b.snr[b.n++] = atoi(f[i + 3]);
  if (num == total) satsShown[sys] = b;
}

int satellitesInView() {
  return satsShown[0].n + satsShown[1].n;
}

// Satellites whose signal is at least minSnr dB.
int satellitesAbove(int minSnr) {
  int count = 0;
  for (const SatList &s : satsShown)
    for (int i = 0; i < s.n; i++) count += s.snr[i] >= minSnr;
  return count;
}

// The board's six header GPIOs. Reports each pin's idle level, whether it toggles (a GPS TX
// sends a burst every second), and any I2C device answering between two idle-high pins.
void scanHeaderPins() {
  const int pins[] = {15, 16, 17, 18, 21, 33};
  const int n = sizeof(pins) / sizeof(pins[0]);
  int level[n], last[n], edges[n] = {0};

  for (int i = 0; i < n; i++) pinMode(pins[i], INPUT);
  delay(2);
  for (int i = 0; i < n; i++) level[i] = last[i] = digitalRead(pins[i]);
  for (uint32_t start = millis(); millis() - start < 1100;) {
    for (int i = 0; i < n; i++) {
      int v = digitalRead(pins[i]);
      if (v != last[i]) {
        edges[i]++;
        last[i] = v;
      }
    }
  }

  Serial.print("[pins]");
  for (int i = 0; i < n; i++)
    Serial.printf(" %d=%s", pins[i], edges[i] > 20 ? "toggling(GPS TX?)" : level[i] ? "high" : "low");
  Serial.println();

  // Only idle-high pairs: a low SCL makes every probe wait for the bus timeout.
  int devices = 0;
  for (int s = 0; s < n; s++) {
    for (int c = 0; c < n; c++) {
      if (s == c || !level[s] || !level[c] || edges[s] > 20 || edges[c] > 20) continue;
      Wire1.begin(pins[s], pins[c], 100000);
      for (uint8_t a = 0x08; a < 0x78; a++) {
        Wire1.beginTransmission(a);
        if (Wire1.endTransmission() == 0) {
          Serial.printf("[pins] I2C device 0x%02X at SDA=%d SCL=%d\n", a, pins[s], pins[c]);
          devices++;
        }
      }
      Wire1.end();
    }
  }
  if (!devices) Serial.println("[pins] no I2C device on any header pin pair");
}

void setup() {
  // Panel RAM powers up as noise; keep the backlight off until the first frame is drawn
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 2000) {}   // USB serial reconnects after reset; don't lose boot messages
  if (PIN_SCAN) scanHeaderPins();   // before the GPS UART and compass claim their pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  compassBegin();
  Serial.printf("[compass] %s\n", compassStatus());

  tft.init();
  digitalWrite(TFT_BL, LOW);   // init() turns the backlight on
  tft.setRotation(SCREEN_FLIPPED ? 2 : 0);
  tft.fillScreen(TFT_BLACK);

  // 16-bit needs ~115 KB (enable PSRAM); fall back to 8-bit if that fails
  spr.setColorDepth(16);
  if (!spr.createSprite(240, 240)) {
    Serial.println("16-bit sprite failed, falling back to 8-bit");
    spr.setColorDepth(8);
    spr.createSprite(240, 240);
  }
}

// compassHeading() is for the board's top edge; flipped, the top of the screen faces the other way.
float screenHeading() {
  float h = compassHeading();
  if (SCREEN_FLIPPED && !isnan(h)) h = fmodf(h + 180.0f, 360.0f);
  return h;
}

void readGps() {
  while (gpsSerial.available()) {
    char ch = gpsSerial.read();
    if (GPS_DEBUG) Serial.write(ch);
    gps.encode(ch);

    static char line[100];
    static int len = 0;
    if (ch == '$') len = 0;
    if (ch == '\r' || ch == '\n') {
      line[len] = 0;
      if (len) parseGsv(line);
      len = 0;
    } else if (len < (int)sizeof(line) - 1) {
      line[len++] = ch;
    }
  }
}

void printSnrs(const char *name, const SatList &s) {
  Serial.printf(" %s:", name);
  if (!s.n) Serial.print(" -");
  for (int i = 0; i < s.n; i++) Serial.printf(" %d", s.snr[i]);
}

// chars=0: nothing on the RX pin. Many failed checksums: wrong baud rate.
void printStatus(uint32_t now) {
  static uint32_t lastGps = 0;
  static bool hadFix = false;
  if (now - lastGps >= 1000) {
    lastGps = now;
    int heard = satellitesAbove(1), good = satellitesAbove(GOOD_SNR_DB);
    bool fix = gps.location.isValid();
    Serial.printf("[gps] in view: %d, heard: %d, good (>=%d dB): %d/4, used: %d, fix: %s, %lu s |",
                  satellitesInView(), heard, GOOD_SNR_DB, good,
                  gps.satellites.isValid() ? (int)gps.satellites.value() : 0, fix ? "yes" : "no",
                  (unsigned long)(now / 1000));
    printSnrs("GPS dB", satsShown[0]);
    printSnrs("BeiDou dB", satsShown[1]);
    Serial.println();
    if (fix && !hadFix) Serial.printf("[gps] first fix after %lu s\n", (unsigned long)(now / 1000));
    hadFix = fix;
  }

  static uint32_t last = 0;
  if (now - last < 5000) return;
  last = now;
  if (GPS_DEBUG)
    Serial.printf("\n[gps] chars=%lu ok=%lu failed=%lu sats=%d fix=%d\n",
                  (unsigned long)gps.charsProcessed(), (unsigned long)gps.passedChecksum(),
                  (unsigned long)gps.failedChecksum(),
                  gps.satellites.isValid() ? (int)gps.satellites.value() : -1,
                  gps.location.isValid() ? 1 : 0);
  Serial.printf("[compass] %s\n", compassStatus());
}

// Serial Monitor commands: c = calibrate compass (20 s, blocks), d = toggle compass debug output
bool compassDebug = false;

void handleSerial() {
  if (!Serial.available()) return;
  char ch = Serial.read();
  if (ch == 'c') {
    Serial.println("Calibrating for 20 s: rotate slowly in every direction (figure-eights, flips)...");
    compassCalibrate(20000);
    Serial.println("Saved.");
  } else if (ch == 'd') {
    compassDebug = !compassDebug;
  }
}

void loop() {
  readGps();
  handleSerial();
  uint32_t now = millis();
  printStatus(now);

  static uint32_t lastDebug = 0;
  if (compassDebug && now - lastDebug >= 200) {
    lastDebug = now;
    compassDebugPrint();
  }

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
  compassUpdate();

  static Screen screen = Screen::Intro;
  static uint32_t screenStart = now;
  uint32_t t = now - screenStart;
  Screen next = screen;

  switch (screen) {
    case Screen::Intro:
      if (drawIntroScreen(spr, t)) next = Screen::Loading;
      break;
    case Screen::Loading:
      drawLoadingScreen(spr, t, satellitesAbove(GOOD_SNR_DB));
      if (fix) next = Screen::Compass;
      break;
    case Screen::Compass:
      drawCompassScreen(spr, t, nearest.distanceM, nearest.bearingDeg, screenHeading());
      if (!fix) next = Screen::Loading;
      break;
  }

  static bool backlightOn = false;
  if (!backlightOn) {
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    backlightOn = true;
  }

  if (next != screen) {
    screen = next;
    screenStart = now;
  }
}
