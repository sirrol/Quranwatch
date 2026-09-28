#pragma once
/*
  Storage.h
  ------------------------------------------------------------
  Saves/loads AppSettings and the Quran bookmark to the ESP32's
  non-volatile storage (flash), using the Preferences library
  (built into the Arduino-ESP32 core - no extra lib_deps needed).

  This survives power loss / battery changes, unlike RTC_DATA_ATTR
  (which only survives deep sleep, not a full power-off).
*/

#include <Arduino.h>
#include <Preferences.h>
#include "AppConfig.h"

namespace QWStorage {

static const char *NS = "quranwatch";

inline void loadSettings(AppSettings &settings) {
  Preferences prefs;
  prefs.begin(NS, true); // read-only
  settings.latitude = prefs.getFloat("lat", DEFAULT_APP_SETTINGS.latitude);
  settings.longitude = prefs.getFloat("lon", DEFAULT_APP_SETTINGS.longitude);
  settings.utcOffsetSeconds =
      prefs.getInt("utcOff", DEFAULT_APP_SETTINGS.utcOffsetSeconds);
  settings.daylightSaving =
      prefs.getBool("dst", DEFAULT_APP_SETTINGS.daylightSaving);
  settings.hijriAdjustment =
      (int8_t)prefs.getChar("hijriAdj", DEFAULT_APP_SETTINGS.hijriAdjustment);
  settings.prayerMethod =
      prefs.getUChar("prMethod", DEFAULT_APP_SETTINGS.prayerMethod);
  settings.asrMethod =
      prefs.getUChar("asrMethod", DEFAULT_APP_SETTINGS.asrMethod);
  settings.prayerVibrate =
      prefs.getBool("prVib", DEFAULT_APP_SETTINGS.prayerVibrate);
  prefs.end();
}

inline void saveSettings(const AppSettings &settings) {
  Preferences prefs;
  prefs.begin(NS, false); // read/write
  prefs.putFloat("lat", settings.latitude);
  prefs.putFloat("lon", settings.longitude);
  prefs.putInt("utcOff", settings.utcOffsetSeconds);
  prefs.putBool("dst", settings.daylightSaving);
  prefs.putChar("hijriAdj", settings.hijriAdjustment);
  prefs.putUChar("prMethod", settings.prayerMethod);
  prefs.putUChar("asrMethod", settings.asrMethod);
  prefs.putBool("prVib", settings.prayerVibrate);
  prefs.end();
}

inline void loadBookmark(QuranBookmark &bm) {
  Preferences prefs;
  prefs.begin(NS, true);
  bm.surah = prefs.getUChar("surah", DEFAULT_BOOKMARK.surah);
  bm.verse = prefs.getUShort("verse", DEFAULT_BOOKMARK.verse);
  prefs.end();
}

inline void saveBookmark(const QuranBookmark &bm) {
  Preferences prefs;
  prefs.begin(NS, false);
  prefs.putUChar("surah", bm.surah);
  prefs.putUShort("verse", bm.verse);
  prefs.end();
}
// Time offset (seconds) added to the RTC's own time to get the time
// shown on the watch. Lets us "set" the time/date without ever writing
// to the RTC chip.
inline int32_t loadTimeOffset() {
  Preferences prefs;
  prefs.begin(NS, true);
  int32_t v = prefs.getInt("tOff", 0);
  prefs.end();
  return v;
}

inline void saveTimeOffset(int32_t offsetSeconds) {
  Preferences prefs;
  prefs.begin(NS, false);
  prefs.putInt("tOff", offsetSeconds);
  prefs.end();
}
} // namespace QWStorage
