#pragma once
/*
  PrayerTimes.h
  ------------------------------------------------------------
  Standard solar-angle prayer time calculation (the same family
  of equations - equation of time + sun declination + hour angle
  - used by most prayer-time engines and commercial prayer
  watches). Given accurate latitude/longitude/UTC-offset this is
  typically accurate to within about a minute.

  All returned times are LOCAL minutes-since-midnight (0..1439),
  already adjusted for utcOffsetSeconds + DST.
*/

#include <Arduino.h>
#include <math.h>
#include <time.h>
#include "AppConfig.h"

struct PrayerTimesResult {
  int fajrMin;
  int shuruqMin; // sunrise
  int dhuhrMin;
  int asrMin;
  int maghribMin; // sunset
  int ishaMin;
};

// Prayer index used to highlight the "current" prayer in zone 6.
enum PrayerIndex : uint8_t {
  PRAYER_FAJR = 0,
  PRAYER_SHURUQ,
  PRAYER_DHUHR,
  PRAYER_ASR,
  PRAYER_MAGHRIB,
  PRAYER_ISHA,
  PRAYER_NONE
};

namespace PrayerMath {

inline double deg2rad(double d) { return d * PI / 180.0; }
inline double rad2deg(double r) { return r * 180.0 / PI; }

// Julian Day at 0h UTC for a given Gregorian date.
inline double julianDay(int year, int month, int day) {
  if (month <= 2) {
    year -= 1;
    month += 12;
  }
  int a = year / 100;
  int b = 2 - a + a / 4;
  return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day +
         b - 1524.5;
}

// Equation of time (minutes) and sun declination (degrees) for a
// given Julian Day, using standard low-precision solar
// coordinates (accurate to a fraction of a degree - plenty for
// prayer-time purposes).
inline void sunPosition(double jd, double &eqTimeMinutes, double &declDeg) {
  double D = jd - 2451545.0; // days since J2000.0
  double g = fmod(357.529 + 0.98560028 * D, 360.0); // mean anomaly
  double q = fmod(280.459 + 0.98564736 * D, 360.0); // mean longitude
  double L = fmod(q + 1.915 * sin(deg2rad(g)) + 0.020 * sin(deg2rad(2 * g)),
                   360.0); // apparent longitude
  double e = 23.439 - 0.00000036 * D; // obliquity of the ecliptic

  double RA = rad2deg(atan2(cos(deg2rad(e)) * sin(deg2rad(L)), cos(deg2rad(L))));
  RA = fmod(RA, 360.0);
  if (RA < 0) RA += 360.0;

  eqTimeMinutes = (q - RA) * 4.0; // 4 minutes per degree
  if (eqTimeMinutes > 20) eqTimeMinutes -= 1440;
  if (eqTimeMinutes < -20) eqTimeMinutes += 1440;

  declDeg = rad2deg(asin(sin(deg2rad(e)) * sin(deg2rad(L))));
}

// Local solar time (in hours) at which the sun crosses the given
// angle below the horizon (negative angle = below horizon, used
// for Fajr/Isha; positive small angle used for sunrise/sunset).
// afternoon = true for the afternoon crossing (Asr/Isha/Maghrib),
// false for the morning crossing (Fajr/Shuruq).
inline double timeForAngle(double angleDeg, double latitude, double declDeg,
                            double eqTimeMinutes, double longitude,
                            double utcOffsetHours, bool afternoon) {
  double latRad = deg2rad(latitude);
  double declRad = deg2rad(declDeg);
  double cosH = (sin(deg2rad(angleDeg)) - sin(latRad) * sin(declRad)) /
                (cos(latRad) * cos(declRad));
  if (cosH > 1.0) cosH = 1.0;   // sun never reaches this angle (polar summer)
  if (cosH < -1.0) cosH = -1.0; // sun always below/above (polar winter)
  double H = rad2deg(acos(cosH)); // hour angle in degrees

  double noon = 12.0 - (longitude / 15.0) - (eqTimeMinutes / 60.0) +
                utcOffsetHours;
  double offsetHours = H / 15.0;
  return afternoon ? (noon + offsetHours) : (noon - offsetHours);
}

// Asr hour angle: the angle at which shadow length = shadowFactor
// * object length + shadow at noon.
inline double asrAngle(double latitude, double declDeg, double shadowFactor) {
  double latRad = deg2rad(latitude);
  double declRad = deg2rad(declDeg);
  double x = shadowFactor + tan(fabs(latRad - declRad));
  double angle = rad2deg(atan(1.0 / x));
  return angle; // altitude angle above horizon for Asr
}

inline int hoursToMinutesOfDay(double hours) {
  // wrap into 0..24
  while (hours < 0) hours += 24.0;
  while (hours >= 24.0) hours -= 24.0;
  int minutes = (int)round(hours * 60.0);
  if (minutes >= 1440) minutes -= 1440;
  if (minutes < 0) minutes += 1440;
  return minutes;
}

} // namespace PrayerMath

inline PrayerTimesResult calculatePrayerTimes(int year, int month, int day,
                                               const AppSettings &settings) {
  using namespace PrayerMath;

  double jd = julianDay(year, month, day);
  double eqTime, decl;
  sunPosition(jd, eqTime, decl);

  double utcOffsetHours =
      (settings.utcOffsetSeconds / 3600.0) + (settings.daylightSaving ? 1.0 : 0.0);

  const PrayerAngles &angles = PRAYER_METHOD_TABLE[settings.prayerMethod];

  double fajrH = timeForAngle(-angles.fajrAngle, settings.latitude, decl,
                               eqTime, settings.longitude, utcOffsetHours,
                               false);
  double shuruqH = timeForAngle(-0.833, settings.latitude, decl, eqTime,
                                 settings.longitude, utcOffsetHours, false);
  double dhuhrH = 12.0 - (settings.longitude / 15.0) - (eqTime / 60.0) +
                  utcOffsetHours + (1.0 / 60.0); // +1 min safety margin (standard convention)
  double maghribH = timeForAngle(-0.833, settings.latitude, decl, eqTime,
                                  settings.longitude, utcOffsetHours, true);

  double shadowFactor = (settings.asrMethod == ASR_HANAFI) ? 2.0 : 1.0;
  double asrAlt = asrAngle(settings.latitude, decl, shadowFactor);
  double asrH = timeForAngle(asrAlt, settings.latitude, decl, eqTime,
                              settings.longitude, utcOffsetHours, true);

  double ishaH;
  if (settings.prayerMethod == METHOD_UMM_AL_QURA) {
    ishaH = maghribH + (angles.ishaFixedMin / 60.0);
  } else {
    ishaH = timeForAngle(-angles.ishaAngle, settings.latitude, decl, eqTime,
                          settings.longitude, utcOffsetHours, true);
  }

  PrayerTimesResult result;
  result.fajrMin = hoursToMinutesOfDay(fajrH);
  result.shuruqMin = hoursToMinutesOfDay(shuruqH);
  result.dhuhrMin = hoursToMinutesOfDay(dhuhrH);
  result.asrMin = hoursToMinutesOfDay(asrH);
  result.maghribMin = hoursToMinutesOfDay(maghribH);
  result.ishaMin = hoursToMinutesOfDay(ishaH);
  return result;
}

// Which prayer period "owns" the current time-of-day (for
// highlighting in zone 6).
inline PrayerIndex currentPrayerIndex(int nowMinutes,
                                       const PrayerTimesResult &t) {
  if (nowMinutes >= t.ishaMin || nowMinutes < t.fajrMin) return PRAYER_ISHA;
  if (nowMinutes >= t.maghribMin) return PRAYER_MAGHRIB;
  if (nowMinutes >= t.asrMin) return PRAYER_ASR;
  if (nowMinutes >= t.dhuhrMin) return PRAYER_DHUHR;
  if (nowMinutes >= t.shuruqMin) return PRAYER_SHURUQ;
  if (nowMinutes >= t.fajrMin) return PRAYER_FAJR;
  return PRAYER_ISHA; // before Fajr = still within last night's Isha period
}

inline void minutesToHM(int minutes, int &h, int &m) {
  minutes = ((minutes % 1440) + 1440) % 1440;
  h = minutes / 60;
  m = minutes % 60;
}