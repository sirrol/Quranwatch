#pragma once
/*
  UiLayout.h
  ------------------------------------------------------------
  Draws the normal watch face (the 9 information zones) on a
  200x200 monochrome canvas.

  v2 changes (fixing the overlapping-text bug from the photos):
   - All text now uses the built-in monospaced GFX font via
     qwSetText()/qwTextWidth() (see UiCommon.h) instead of the
     proportional FreeSans fonts, so every string's pixel width
     is known exactly and can never spill into a neighboring
     zone.
   - Zone 3 (info) now shows only the Hijri date - day/month on
     one line, year on the next - plus battery, as requested.
     The Gregorian date is no longer shown here (it saves the
     vertical space needed to keep everything legible). The
     Gregorian date is still used internally for every
     calculation - it's just not displayed twice.

  Layout (unchanged from before):
    +--------+----------------+----------+
    | Moon   |  Info (Hijri   |  Quran   |   row 1 (0-64)
    | phase  |  date, battery)|  bookmark|
    +--------+----------------+----------+
    | Hijri  |     Hour       | S V P    |
    | 4x3    |   (progress    | bars     |   row 2 (64-160)
    | grid   |    square)     |          |
    +--------+----------------+----------+
    |         Prayer times (2x3)          |   row 3 (160-200)
    +--------------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include "UiCommon.h"
#include "AppConfig.h"
#include "QuranData.h"
#include "MoonPhase.h"
#include "HijriCalendar.h"
#include "PrayerTimes.h"

#ifndef GxEPD_BLACK
#define GxEPD_BLACK 0
#endif
#ifndef GxEPD_WHITE
#define GxEPD_WHITE 1
#endif

// ---------------------------------------------------------------
// Layout constants (200 x 200 canvas)
// ---------------------------------------------------------------
namespace UiZones {
constexpr int16_t W = 200, H = 200;

constexpr int16_t ROW1_Y0 = 0,   ROW1_Y1 = 64;
constexpr int16_t ROW2_Y0 = 64,  ROW2_Y1 = 160;
constexpr int16_t ROW3_Y0 = 160, ROW3_Y1 = 200;

constexpr int16_t R1_COL1_X0 = 0,   R1_COL1_X1 = 64;  // moon
constexpr int16_t R1_COL2_X0 = 64,  R1_COL2_X1 = 136; // info
constexpr int16_t R1_COL3_X0 = 136, R1_COL3_X1 = 200; // bookmark

constexpr int16_t R2_COL1_X0 = 0,   R2_COL1_X1 = 72;  // hijri grid
constexpr int16_t R2_COL2_X0 = 72,  R2_COL2_X1 = 146; // hour
constexpr int16_t R2_COL3_X0 = 146, R2_COL3_X1 = 200; // S/V/P bars
} // namespace UiZones

// ---------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------
inline uint8_t batteryVoltageToPercent(float v) {
  // Simple LiPo curve approximation (3.3V = 0%, 4.2V = 100%).
  if (v >= 4.2f) return 100;
  if (v <= 3.3f) return 0;
  float pct = (v - 3.3f) / (4.2f - 3.3f) * 100.0f;
  return (uint8_t)pct;
}

// Centers `text` horizontally around cx, top of the glyphs at topY,
// at the given built-in-font size. Width is computed exactly (no
// guessing) from the monospaced 6*size-per-character metric.
inline void drawCenteredText(Adafruit_GFX &d, const char *text, int16_t cx,
                              int16_t topY, uint8_t size) {
  qwSetText(d, size);
  int16_t w = qwTextWidth((uint8_t)strlen(text), size);
  d.setCursor(cx - w / 2, topY);
  d.print(text);
}

// Draws a thick vertical progress bar. Fill grows from the bottom.
inline void drawVerticalBar(Adafruit_GFX &d, int16_t x, int16_t y,
                             int16_t w, int16_t h, uint8_t percent) {
  d.drawRect(x, y, w, h, GxEPD_BLACK);
  int16_t fillH = (int16_t)((int32_t)(h - 2) * percent / 100);
  if (fillH > 0) {
    d.fillRect(x + 1, y + (h - 1 - fillH), w - 2, fillH, GxEPD_BLACK);
  }
}

// ---------------------------------------------------------------
// Zone 1: Moon phase
// ---------------------------------------------------------------
inline void drawZoneMoon(Adafruit_GFX &d, time_t utcNow) {
  using namespace UiZones;
  MoonData moon = calculateMoonPhase(utcNow);

  // Bigger moon, placed slightly high so the corner text has room.
  int16_t cx = (R1_COL1_X0 + R1_COL1_X1) / 2;
  int16_t cy = ROW1_Y0 + 28;
  drawMoonIcon(d, cx, cy, 50, moon);

  // Small percentage, bottom-right corner of the zone.
  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  char buf[8];
  snprintf(buf, sizeof(buf), "%u%%", moonIlluminationPercent(moon));
  int16_t w = qwTextWidth((uint8_t)strlen(buf), 1);
  d.setCursor(R1_COL1_X1 - w - 3, ROW1_Y1 - qwTextHeight(1) - 3);
  d.print(buf);
}

// ---------------------------------------------------------------
// Zone 3: Info - Hijri day + month abbreviation, year, battery.
// All three lines are centered in the zone.
// ---------------------------------------------------------------
static const char *HIJRI_MONTH_SHORT[12] = {"Muh", "Saf", "R1",  "R2",
                                             "J1",  "J2",  "Raj", "Sha",
                                             "Ram", "Shw", "DQ",  "DH"};

inline void drawZoneInfo(Adafruit_GFX &d, const HijriDate &hijri,
                          float batteryVoltage) {
  using namespace UiZones;
  int16_t cx = (R1_COL2_X0 + R1_COL2_X1) / 2;
  d.setTextColor(GxEPD_BLACK);

  // Line 1: day + month abbreviation, centered as one block (size 2).
  char dayBuf[4];
  snprintf(dayBuf, sizeof(dayBuf), "%02d", hijri.day);
  const char *mon = (hijri.month >= 1 && hijri.month <= 12)
                        ? HIJRI_MONTH_SHORT[hijri.month - 1]
                        : "?";
  int16_t dayW = qwTextWidth((uint8_t)strlen(dayBuf), 2);
  int16_t monW = qwTextWidth((uint8_t)strlen(mon), 2);
  int16_t gap = 4;
  int16_t x = cx - (dayW + gap + monW) / 2;
  qwSetText(d, 2);
  d.setCursor(x, ROW1_Y0 + 6);
  d.print(dayBuf);
  d.setCursor(x + dayW + gap, ROW1_Y0 + 6);
  d.print(mon);

  // Line 2: year, centered.
  char year[8];
  snprintf(year, sizeof(year), "%04d", hijri.year);
  drawCenteredText(d, year, cx, ROW1_Y0 + 26, 2);

  // Line 3: battery, centered.
  char bat[16];
  snprintf(bat, sizeof(bat), "BAT %u%%", batteryVoltageToPercent(batteryVoltage));
  drawCenteredText(d, bat, cx, ROW1_Y0 + 50, 1);
}

// ---------------------------------------------------------------
// Zone 4: Quran bookmark
// ---------------------------------------------------------------
inline void drawZoneBookmark(Adafruit_GFX &d, const QuranBookmark &bm) {
  using namespace UiZones;
  int16_t cx = (R1_COL3_X0 + R1_COL3_X1) / 2;
  d.setTextColor(GxEPD_BLACK);
  char buf[16];
  snprintf(buf, sizeof(buf), "S%03u", bm.surah);
  drawCenteredText(d, buf, cx, ROW1_Y0 + 10, 2);
  snprintf(buf, sizeof(buf), ":%03u", bm.verse);
  drawCenteredText(d, buf, cx, ROW1_Y0 + 34, 2);
}

// ---------------------------------------------------------------
// Zone 2: Hijri 4x3 month grid. The current month is shown in an
// inverted box (white digits on a black cell).
// ---------------------------------------------------------------
inline void drawZoneHijriGrid(Adafruit_GFX &d, const HijriDate &hijri) {
  using namespace UiZones;
  int16_t cellW = (R2_COL1_X1 - R2_COL1_X0) / 3;
  int16_t cellH = (ROW2_Y1 - ROW2_Y0) / 4;

  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 3; col++) {
      int monthNum = row * 3 + col + 1;
      int16_t cellX0 = R2_COL1_X0 + col * cellW;
      int16_t cellY0 = ROW2_Y0 + row * cellH;
      int16_t cellCx = cellX0 + cellW / 2;
      int16_t cellCy = cellY0 + cellH / 2;

      bool isCurrent = isCurrentHijriMonth(monthNum, hijri);
      if (isCurrent) {
        // Inverted box, inset by 2 px so it stays clear of the separators.
        d.fillRect(cellX0 + 2, cellY0 + 2, cellW - 4, cellH - 4, GxEPD_BLACK);
      }

      d.setTextColor(isCurrent ? GxEPD_WHITE : GxEPD_BLACK);
      char buf[4];
      snprintf(buf, sizeof(buf), "%02d", monthNum);
      drawCenteredText(d, buf, cellCx, cellCy - qwTextHeight(1) / 2, 1);
    }
  }
  d.setTextColor(GxEPD_BLACK);
}

/// ---------------------------------------------------------------
// Zone 5: Hour, with four quarter-hour dots underneath.
//   - already-passed quarters: solid black dot
//   - current quarter:         black dot with a white center (ring)
//   - upcoming quarters:       empty (outline only)
// filled = 1 + minute/15  (00-14: 1, 15-29: 2, 30-44: 3, 45-59: 4),
// the LAST of these `filled` dots is the current one.
// ---------------------------------------------------------------
inline void drawZoneHour(Adafruit_GFX &d, int hour24, int minute) {
  using namespace UiZones;
  int16_t cx = (R2_COL2_X0 + R2_COL2_X1) / 2;

  // Hour digits (size 5 = 30x40 px per character).
  d.setTextColor(GxEPD_BLACK);
  char buf[4];
  snprintf(buf, sizeof(buf), "%02d", hour24);
  int16_t hourTop = ROW2_Y0 + 16;
  drawCenteredText(d, buf, cx, hourTop, 5);

  // Four quarter-hour dots below the hour.
  const int16_t r = 6;          // dot radius
  const int16_t spacing = 16;   // distance between dot centers
  int16_t dotsCy = ROW2_Y0 + 76;
  int filled = minute / 15 + 1;
  if (filled > 4) filled = 4;
  int current = filled - 1; // index (0-3) of the current quarter

  for (int i = 0; i < 4; i++) {
    int16_t dx = cx + (int16_t)((i - 1.5f) * spacing);
    if (i < current) {
      // already passed: solid dot
      d.fillCircle(dx, dotsCy, r, GxEPD_BLACK);
    } else if (i == current) {
      // current quarter: ring (black dot, white center)
      d.fillCircle(dx, dotsCy, r, GxEPD_BLACK);
      d.fillCircle(dx, dotsCy, r - 3, GxEPD_WHITE);
    } else {
      // upcoming: empty outline, 2px so it stays visible on e-ink
      d.drawCircle(dx, dotsCy, r, GxEPD_BLACK);
      d.drawCircle(dx, dotsCy, r - 1, GxEPD_BLACK);
    }
  }
}

// ---------------------------------------------------------------
// Zones 7/8/9: S / V / P thick vertical progress bars
// ---------------------------------------------------------------
// Zone 7/8/9 area now shows just one thing: overall Quran
// progress (P) - the per-surah (S) and per-verse-in-surah (V)
// bars were removed as requested. The freed width goes to a
// single wider, easier-to-read bar.
inline void drawZoneQuranBars(Adafruit_GFX &d, const QuranBookmark &bm) {
  using namespace UiZones;
  int16_t areaX0 = R2_COL3_X0 + 6, areaX1 = R2_COL3_X1 - 6;
  int16_t barW = areaX1 - areaX0; // full available width, one bar only
  int16_t barTop = ROW2_Y0 + 22, barBottom = ROW2_Y1 - 20;
  int16_t barH = barBottom - barTop;
  uint8_t percent = totalQuranPercent(bm);

  d.setTextColor(GxEPD_BLACK);
  drawCenteredText(d, "Quran %", areaX0 + barW / 2, ROW2_Y0 + 8, 1);
  drawVerticalBar(d, areaX0, barTop, barW, barH, percent);

  char pctBuf[6];
  snprintf(pctBuf, sizeof(pctBuf), "%u%%", percent);
  drawCenteredText(d, pctBuf, areaX0 + barW / 2, barBottom + 6, 1);
}

// ---------------------------------------------------------------
// Zone 6: Prayer times. The current prayer's cell is inverted
// (white text on a black cell).
// ---------------------------------------------------------------
inline void drawZonePrayerTimes(Adafruit_GFX &d, const PrayerTimesResult &t,
                                 PrayerIndex current) {
  using namespace UiZones;
  const char *labels[6] = {"F", "S", "D", "A", "M", "I"};
  int mins[6] = {t.fajrMin, t.shuruqMin, t.dhuhrMin, t.asrMin, t.maghribMin,
                 t.ishaMin};

  // Cell edges = the separator lines drawn by drawSeparators().
  const int16_t xs[4] = {0, W / 3, 2 * W / 3, W - 1};
  const int16_t ys[3] = {ROW3_Y0, ROW3_Y0 + (ROW3_Y1 - ROW3_Y0) / 2, H - 1};

  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int16_t x0 = xs[col], x1 = xs[col + 1];
    int16_t y0 = ys[row], y1 = ys[row + 1];

    bool isCurrent = ((int)current == i);
    if (isCurrent) {
      // Fill strictly inside the separator lines so they stay intact.
      d.fillRect(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, GxEPD_BLACK);
    }

    int h, m;
    minutesToHM(mins[i], h, m);
    char buf[10];
    snprintf(buf, sizeof(buf), "%s %02d:%02d", labels[i], h, m);

    qwSetText(d, 1);
    d.setTextColor(isCurrent ? GxEPD_WHITE : GxEPD_BLACK);
    int16_t textW = qwTextWidth((uint8_t)strlen(buf), 1);
    d.setCursor(x0 + (x1 - x0 - textW) / 2,
                y0 + (y1 - y0 - qwTextHeight(1)) / 2);
    d.print(buf);
  }
  d.setTextColor(GxEPD_BLACK);
}

// ---------------------------------------------------------------
// Separators between all zones
// ---------------------------------------------------------------
inline void drawSeparators(Adafruit_GFX &d) {
  using namespace UiZones;
  d.drawFastHLine(0, ROW1_Y1, W, GxEPD_BLACK);
  d.drawFastHLine(0, ROW2_Y1, W, GxEPD_BLACK);
  d.drawFastVLine(R1_COL2_X0, ROW1_Y0, ROW1_Y1 - ROW1_Y0, GxEPD_BLACK);
  d.drawFastVLine(R1_COL3_X0, ROW1_Y0, ROW1_Y1 - ROW1_Y0, GxEPD_BLACK);
  d.drawFastVLine(R2_COL2_X0, ROW2_Y0, ROW2_Y1 - ROW2_Y0, GxEPD_BLACK);
  d.drawFastVLine(R2_COL3_X0, ROW2_Y0, ROW2_Y1 - ROW2_Y0, GxEPD_BLACK);
  d.drawFastVLine(W / 3, ROW3_Y0, ROW3_Y1 - ROW3_Y0, GxEPD_BLACK);
  d.drawFastVLine(2 * W / 3, ROW3_Y0, ROW3_Y1 - ROW3_Y0, GxEPD_BLACK);
  d.drawFastHLine(0, ROW3_Y0 + (ROW3_Y1 - ROW3_Y0) / 2, W, GxEPD_BLACK);
  d.drawRect(0, 0, W, H, GxEPD_BLACK); // outer border
}

// ---------------------------------------------------------------
// Top-level entry point
// ---------------------------------------------------------------
inline void drawMainFace(Adafruit_GFX &d, const AppSettings &settings,
                          const QuranBookmark &bookmark, int gYear, int gMonth,
                          int gDay, int hour24, int minute, time_t utcNow,
                          float batteryVoltage) {
  d.fillScreen(GxEPD_WHITE);

  HijriDate hijri =
      gregorianToHijri(gYear, gMonth, gDay, settings.hijriAdjustment);
  PrayerTimesResult prayers = calculatePrayerTimes(gYear, gMonth, gDay, settings);
  int nowMinutes = hour24 * 60 + minute;
  PrayerIndex current = currentPrayerIndex(nowMinutes, prayers);

  drawZoneMoon(d, utcNow);
  drawZoneInfo(d, hijri, batteryVoltage);
  drawZoneBookmark(d, bookmark);
  drawZoneHijriGrid(d, hijri);
  drawZoneHour(d, hour24, minute);
  drawZoneQuranBars(d, bookmark);
  drawZonePrayerTimes(d, prayers, current);
  drawSeparators(d);
}
