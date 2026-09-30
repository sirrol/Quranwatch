#pragma once
/*
  UiLayout.h
  ------------------------------------------------------------
  Draws the normal watch face (the 9 information zones) on a
  200x200 monochrome canvas.

  Layout:
    +--------+----------------+----------+
    | Moon   |  Info (Hijri   |  Quran   |   row 1 (0-64)
    | phase  |  date, battery)|  bookmark|
    +--------+----------------+----------+
    | Hijri  |     Hour       | Quran %  |
    | 4x3    |   (quarter-    | bar      |   row 2 (64-160)
    | grid   |    hour dots)  |          |
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
constexpr int16_t R2_COL3_X0 = 146, R2_COL3_X1 = 200; // Quran % bar
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
// at the given built-in-font size.
inline void drawCenteredText(Adafruit_GFX &d, const char *text, int16_t cx,
                              int16_t topY, uint8_t size) {
  qwSetText(d, size);
  int16_t w = qwTextInkWidth((uint8_t)strlen(text), size);
  d.setCursor(cx - w / 2, topY);
  d.print(text);
}

// Draws a thick vertical progress bar. Fill grows from the bottom.
inline void drawVerticalBar(Adafruit_GFX &d, int16_t x, int16_t y,
                             int16_t w, int16_t h, uint8_t percent) {
  d.drawRect(x, y, w, h, qwInk());
  int16_t fillH = (int16_t)((int32_t)(h - 2) * percent / 100);
  if (fillH > 0) {
    d.fillRect(x + 1, y + (h - 1 - fillH), w - 2, fillH, qwInk());
  }
}

// ---------------------------------------------------------------
// Zone 1: Moon phase
// ---------------------------------------------------------------
inline void drawZoneMoon(Adafruit_GFX &d, time_t utcNow) {
  using namespace UiZones;
  MoonData moon = calculateMoonPhase(utcNow);

  int16_t cx = (R1_COL1_X0 + R1_COL1_X1) / 2;
  int16_t cy = ROW1_Y0 + 28;
  drawMoonIcon(d, cx, cy, 50, moon);

  d.setTextColor(qwInk());
  qwSetText(d, 1);
  char buf[8];
  snprintf(buf, sizeof(buf), "%u%%", moonIlluminationPercent(moon));
  int16_t w = qwTextWidth((uint8_t)strlen(buf), 1);
  d.setCursor(R1_COL1_X1 - w - 3, ROW1_Y1 - qwTextHeight(1) - 3);
  d.print(buf);
}

// ---------------------------------------------------------------
// Zone 3: Info - Hijri day + month abbreviation, year, battery.
// All three lines are centered. Battery line inverts with a "!"
// once the charge is at or below LOW_BATTERY_PERCENT.
// ---------------------------------------------------------------
static const char *HIJRI_MONTH_SHORT[12] = {"Muh", "Saf", "R1",  "R2",
                                             "J1",  "J2",  "Raj", "Sha",
                                             "Ram", "Shw", "DQ",  "DH"};

inline void drawZoneInfo(Adafruit_GFX &d, const HijriDate &hijri,
                          float batteryVoltage) {
  using namespace UiZones;
  int16_t cx = (R1_COL2_X0 + R1_COL2_X1) / 2;
  d.setTextColor(qwInk());

  // Line 1: day + month abbreviation, centered as one block (size 2).
  char dayBuf[4];
  snprintf(dayBuf, sizeof(dayBuf), "%02d", hijri.day);
  const char *mon = (hijri.month >= 1 && hijri.month <= 12)
                        ? HIJRI_MONTH_SHORT[hijri.month - 1]
                        : "?";
  int16_t dayW = qwTextInkWidth((uint8_t)strlen(dayBuf), 2);
  int16_t monW = qwTextInkWidth((uint8_t)strlen(mon), 2);
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

  // Line 3: battery, centered. Inverted with "!" when low.
  uint8_t batPct = batteryVoltageToPercent(batteryVoltage);
  bool lowBat = batPct <= LOW_BATTERY_PERCENT;
  char bat[16];
  qwSetText(d, 1);
  if (lowBat) {
    snprintf(bat, sizeof(bat), "BAT %u%%!", batPct);
    int16_t w = qwTextInkWidth((uint8_t)strlen(bat), 1);
    int16_t tx = cx - w / 2;
    int16_t ty = ROW1_Y0 + 50;
    d.fillRect(tx - 3, ty - 2, w + 6, qwTextHeight(1) + 4, qwInk());
    d.setTextColor(qwPaper());
    d.setCursor(tx, ty);
    d.print(bat);
    d.setTextColor(qwInk());
  } else {
    snprintf(bat, sizeof(bat), "BAT %u%%", batPct);
    drawCenteredText(d, bat, cx, ROW1_Y0 + 50, 1);
  }
}

// ---------------------------------------------------------------
// Zone 4: Quran bookmark
// ---------------------------------------------------------------
inline void drawZoneBookmark(Adafruit_GFX &d, const QuranBookmark &bm) {
  using namespace UiZones;
  int16_t cx = (R1_COL3_X0 + R1_COL3_X1) / 2;
  d.setTextColor(qwInk());
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
        d.fillRect(cellX0 + 2, cellY0 + 2, cellW - 4, cellH - 4, qwInk());
      }

      d.setTextColor(isCurrent ? qwPaper() : qwInk());
      char buf[4];
      snprintf(buf, sizeof(buf), "%02d", monthNum);
      drawCenteredText(d, buf, cellCx, cellCy - qwTextHeight(1) / 2, 1);
    }
  }
  d.setTextColor(qwInk());
}

// ---------------------------------------------------------------
// Zone 5: Hour, with four quarter-hour dots underneath.
//   - already-passed quarters: solid black dot
//   - current quarter:         black dot with a white center (ring)
//   - upcoming quarters:       empty (outline only)
// ---------------------------------------------------------------
inline void drawZoneHour(Adafruit_GFX &d, int hour24, int minute) {
  using namespace UiZones;
  int16_t cx = (R2_COL2_X0 + R2_COL2_X1) / 2;

  d.setTextColor(qwInk());
  char buf[4];
  snprintf(buf, sizeof(buf), "%02d", hour24);
  int16_t hourTop = ROW2_Y0 + 16;
  drawCenteredText(d, buf, cx, hourTop, 5);

  const int16_t r = 6;
  const int16_t spacing = 16;
  int16_t dotsCy = ROW2_Y0 + 76;
  int filled = minute / 15 + 1;
  if (filled > 4) filled = 4;
  int current = filled - 1;

  for (int i = 0; i < 4; i++) {
    int16_t dx = cx + (int16_t)((i - 1.5f) * spacing);
    if (i < current) {
      d.fillCircle(dx, dotsCy, r, qwInk());
    } else if (i == current) {
      d.fillCircle(dx, dotsCy, r, qwInk());
      d.fillCircle(dx, dotsCy, r - 3, qwPaper());
    } else {
      d.drawCircle(dx, dotsCy, r, qwInk());
      d.drawCircle(dx, dotsCy, r - 1, qwInk());
    }
  }
}

// ---------------------------------------------------------------
// Zone 7/8/9 area: overall Quran progress only (labelled "Quran %").
// ---------------------------------------------------------------
inline void drawZoneQuranBars(Adafruit_GFX &d, const QuranBookmark &bm) {
  using namespace UiZones;
  int16_t areaX0 = R2_COL3_X0 + 6, areaX1 = R2_COL3_X1 - 6;
  int16_t barW = areaX1 - areaX0;
  int16_t barTop = ROW2_Y0 + 22, barBottom = ROW2_Y1 - 20;
  int16_t barH = barBottom - barTop;
  uint8_t percent = totalQuranPercent(bm);

  d.setTextColor(qwInk());
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

  const int16_t xs[4] = {0, W / 3, 2 * W / 3, W - 1};
  const int16_t ys[3] = {ROW3_Y0, ROW3_Y0 + (ROW3_Y1 - ROW3_Y0) / 2, H - 1};

  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int16_t x0 = xs[col], x1 = xs[col + 1];
    int16_t y0 = ys[row], y1 = ys[row + 1];

    bool isCurrent = ((int)current == i);
    if (isCurrent) {
      d.fillRect(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, qwInk());
    }

    int h, m;
    minutesToHM(mins[i], h, m);
    char buf[10];
    snprintf(buf, sizeof(buf), "%s %02d:%02d", labels[i], h, m);

    qwSetText(d, 1);
    d.setTextColor(isCurrent ? qwPaper() : qwInk());
    int16_t textW = qwTextInkWidth((uint8_t)strlen(buf), 1);
    d.setCursor(x0 + (x1 - x0 - textW) / 2,
                y0 + (y1 - y0 - qwTextHeight(1)) / 2);
    d.print(buf);
  }
  d.setTextColor(qwInk());
}

// ---------------------------------------------------------------
// Separators between all zones
// ---------------------------------------------------------------
inline void drawSeparators(Adafruit_GFX &d) {
  using namespace UiZones;
  d.drawFastHLine(0, ROW1_Y1, W, qwInk());
  d.drawFastHLine(0, ROW2_Y1, W, qwInk());
  d.drawFastVLine(R1_COL2_X0, ROW1_Y0, ROW1_Y1 - ROW1_Y0, qwInk());
  d.drawFastVLine(R1_COL3_X0, ROW1_Y0, ROW1_Y1 - ROW1_Y0, qwInk());
  d.drawFastVLine(R2_COL2_X0, ROW2_Y0, ROW2_Y1 - ROW2_Y0, qwInk());
  d.drawFastVLine(R2_COL3_X0, ROW2_Y0, ROW2_Y1 - ROW2_Y0, qwInk());
  d.drawFastVLine(W / 3, ROW3_Y0, ROW3_Y1 - ROW3_Y0, qwInk());
  d.drawFastVLine(2 * W / 3, ROW3_Y0, ROW3_Y1 - ROW3_Y0, qwInk());
  d.drawFastHLine(0, ROW3_Y0 + (ROW3_Y1 - ROW3_Y0) / 2, W, qwInk());
  d.drawRect(0, 0, W, H, qwInk()); // outer border
}

// ---------------------------------------------------------------
// Top-level entry point
// ---------------------------------------------------------------
inline void drawMainFace(Adafruit_GFX &d, const AppSettings &settings,
                          const QuranBookmark &bookmark, int gYear, int gMonth,
                          int gDay, int hour24, int minute, time_t utcNow,
                          float batteryVoltage) {
  d.fillScreen(qwPaper());

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