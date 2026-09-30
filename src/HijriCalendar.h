#pragma once
/*
  HijriCalendar.h
  ------------------------------------------------------------
  HONESTY NOTE (please read before relying on this for fasting/
  Eid decisions): a *true* Odeh-criterion conversion requires
  computing real crescent visibility (moon altitude, elongation,
  local horizon, atmospheric extinction...) at every candidate
  sunset near a given location - that needs a much larger
  astronomical stack than fits in a watch firmware.

  What this file implements instead is the standard tabular/
  arithmetic Hijri calendar (the widely-used "Kuwaiti algorithm"
  found in most open-source Hijri converters), which assumes a
  fixed leap-year pattern rather than real moon sightings. Real
  sighting-based calendars can differ from this by a day in
  either direction near the start of a month.

  That's exactly what `hijriAdjustment` (settable on the watch,
  -3..+3 days) is for: once you notice the arithmetic date is
  off relative to your local moon-sighting authority, dial it in
  and it will stay correct until the next multi-day drift (rare).

  The lunar PHASE shown in zone 1 (MoonPhase.h) is fully
  independent of this file and is always astronomically exact.
*/

#include <Arduino.h>

struct HijriDate {
  int day;
  int month; // 1..12
  int year;
};

inline long gregorianToJDN(int year, int month, int day) {
  long a = (14 - month) / 12;
  long y = year + 4800 - a;
  long m = month + 12 * a - 3;
  return day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
}

// dayAdjustment: manual correction in days (typically -3..+3).
inline HijriDate jdnToHijri(long jdn, int dayAdjustment) {
  long l = jdn + dayAdjustment - 1948440 + 10632;
  long n = (l - 1) / 10631;
  l = l - 10631 * n + 354;
  long j = ((10985 - l) / 5316) * ((50 * l) / 17719) +
           (l / 5670) * ((43 * l) / 15238);
  l = l - ((30 - j) / 15) * ((17719 * j) / 50) -
      (j / 16) * ((15238 * j) / 43) + 29;
  long month = (24 * l) / 709;
  long day = l - (709 * month) / 24;
  long year = 30 * n + j - 30;

  HijriDate h;
  h.day = (int)day;
  h.month = (int)month;
  h.year = (int)year;
  return h;
}

inline HijriDate gregorianToHijri(int gYear, int gMonth, int gDay,
                                   int dayAdjustment) {
  long jdn = gregorianToJDN(gYear, gMonth, gDay);
  return jdnToHijri(jdn, dayAdjustment);
}

// For zone 2 - is this hijri month (1..12) the current one?
inline bool isCurrentHijriMonth(int monthInGrid, const HijriDate &today) {
  return monthInGrid == today.month;
}