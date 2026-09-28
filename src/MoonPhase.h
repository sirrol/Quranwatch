#pragma once
/*
  MoonPhase.h
  ------------------------------------------------------------
  Astronomical lunar phase, no bitmaps.

  v2: the mean-synodic-month model used before could be off by
  4-5 percentage points because the Moon's real orbit is
  elliptical. It is replaced by the Meeus "Astronomical
  Algorithms" phase-angle formula (main periodic terms of the
  Moon's and Sun's positions), which is accurate to a fraction of
  a percent, day after day.

  calculateMoonPhase() needs a real UTC timestamp. Do NOT pass
  time(nullptr) on the Watchy: its system clock is not restored
  after deep sleep. Build the UTC time from the RTC-based time
  instead (see main.cpp).
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <math.h>
#include <time.h>

#ifndef GxEPD_BLACK
#define GxEPD_BLACK 0
#endif
#ifndef GxEPD_WHITE
#define GxEPD_WHITE 1
#endif

struct MoonData {
  double illumination; // 0.0 (new) .. 1.0 (full)
  bool waxing;         // true: lit side grows (lit on the right, northern hemisphere)
};

inline double moonJulianDateFromUnix(time_t utcTime) {
  return 2440587.5 + ((double)utcTime / 86400.0);
}

inline double moonNorm360(double x) {
  x = fmod(x, 360.0);
  if (x < 0) x += 360.0;
  return x;
}

inline MoonData calculateMoonPhase(time_t utcTime) {
  const double DEG = PI / 180.0;
  double T = (moonJulianDateFromUnix(utcTime) - 2451545.0) / 36525.0;

  double D = moonNorm360(297.8501921 + 445267.1114034 * T - 0.0018819 * T * T);
  double M = moonNorm360(357.5291092 + 35999.0502909 * T);
  double Mp = moonNorm360(134.9633964 + 477198.8675055 * T + 0.0087414 * T * T);

  // Phase angle i (0 = full, 180 = new), Meeus ch. 48 / 46.
  double i = 180.0 - D - 6.289 * sin(Mp * DEG) + 2.100 * sin(M * DEG) -
             1.274 * sin((2 * D - Mp) * DEG) - 0.658 * sin(2 * D * DEG) -
             0.214 * sin(2 * Mp * DEG) - 0.110 * sin(D * DEG);

  MoonData m;
  m.illumination = (1.0 + cos(i * DEG)) / 2.0;
  if (m.illumination < 0.0) m.illumination = 0.0;
  if (m.illumination > 1.0) m.illumination = 1.0;
  m.waxing = (D < 180.0); // elongation 0..180 = waxing
  return m;
}

inline uint8_t moonIlluminationPercent(const MoonData &m) {
  return (uint8_t)round(m.illumination * 100.0);
}

// Draws the lit part (black) and the dark part (white), centered
// at (cx, cy), no bitmap. The terminator is an ellipse whose width
// at each row follows the illuminated fraction exactly.
inline void drawMoonIcon(Adafruit_GFX &display, int16_t cx, int16_t cy,
                          int16_t diameter, const MoonData &moon) {
  int16_t radius = diameter / 2;
  if (radius < 2) return;
  double k = moon.illumination;

  for (int16_t dy = -radius; dy <= radius; dy++) {
    double w = sqrt(max(0.0, (double)radius * radius - (double)dy * dy));
    int16_t rowWidth = (int16_t)round(w);
    if (rowWidth <= 0) continue;

    // Terminator position on this row.
    double t = moon.waxing ? (1.0 - 2.0 * k) * w : (2.0 * k - 1.0) * w;

    for (int16_t dx = -rowWidth; dx <= rowWidth; dx++) {
      bool lit = moon.waxing ? (dx >= t) : (dx <= t);
      display.drawPixel(cx + dx, cy + dy, lit ? GxEPD_WHITE : GxEPD_BLACK);
    }
  }
  display.drawCircle(cx, cy, radius, GxEPD_BLACK);
}
