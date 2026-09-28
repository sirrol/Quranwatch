/*
  main.cpp - QuranWatch
  ------------------------------------------------------------
  Target : Watchy v2 (ESP32) - including clones, PlatformIO
  Screen : 200 x 200 monochrome e-paper

  Time/date: the RTC chip is NEVER written. Displayed time =
  RTC time + stored offset (see Storage.h).
  Prayer vibration: checked on every wake-up (once a minute).
*/

#include <Arduino.h>
#include <Watchy.h>
#include <TimeLib.h>

#include "AppConfig.h"
#include "QuranData.h"
#include "MoonPhase.h"
#include "HijriCalendar.h"
#include "PrayerTimes.h"
#include "Storage.h"
#include "UiLayout.h"
#include "InputHandler.h"

// Last prayer already signalled by vibration (survives deep sleep),
// so each prayer vibrates only once per day.
RTC_DATA_ATTR int32_t qwLastVibKey = -1;

class QuranWatch : public Watchy {
 public:
  explicit QuranWatch(const watchySettings &settings) : Watchy(settings) {}

  // RTC time + our stored offset. The RTC is re-read every time:
  // Watchy does NOT refresh currentTime on a button wake-up.
  tmElements_t effectiveTime() {
    RTC.read(currentTime);
    tmElements_t raw = currentTime;
    time_t t = makeTime(raw) + QWStorage::loadTimeOffset();
    tmElements_t out;
    breakTime(t, out);
    return out;
  }

  // Vibrate (100 ON / 120 OFF / 100 ON) at Fajr, Dhuhr, Asr, Maghrib
  // and Isha - not at sunrise - if the option is enabled.
  void checkPrayerVibration() {
    AppSettings settings;
    QWStorage::loadSettings(settings);
    if (!settings.prayerVibrate) return;

    tmElements_t now = effectiveTime();
    PrayerTimesResult t = calculatePrayerTimes(now.Year + 1970, now.Month,
                                               now.Day, settings);
    int nowMin = now.Hour * 60 + now.Minute;
    int times[5] = {t.fajrMin, t.dhuhrMin, t.asrMin, t.maghribMin, t.ishaMin};
    int32_t dayNum = (int32_t)(makeTime(now) / 86400);

    for (int i = 0; i < 5; i++) {
      int diff = nowMin - times[i];
      if (diff >= 0 && diff <= 1) { // tolerate a wake-up 1 minute late
        int32_t key = dayNum * 10 + i;
        if (key != qwLastVibKey) {
          qwLastVibKey = key;
          qwPrayerBuzz();
        }
        break;
      }
    }
  }

  void drawWatchFace() override {
    checkPrayerVibration();

    switch (qwScreen) {
      case SCREEN_MAIN_MENU:
        qwDrawMainMenu(display);
        break;
      case SCREEN_SETTINGS:
        qwDrawSettingsScreen(display);
        break;
      case SCREEN_SET_TIME:
        qwDrawSetTimeScreen(display);
        break;
      case SCREEN_SET_DATE:
        qwDrawSetDateScreen(display);
        break;
      case SCREEN_VERSE_EDIT:
        qwDrawVerseEditScreen(display);
        break;
      case SCREEN_WATCHFACE:
      default: {
        AppSettings settings;
        QuranBookmark bookmark;
        QWStorage::loadSettings(settings);
        QWStorage::loadBookmark(bookmark);

        tmElements_t now = effectiveTime();
        int gYear = now.Year + 1970;
        int gMonth = now.Month;
        int gDay = now.Day;
        int hour24 = now.Hour;
        int minute = now.Minute;

        // Real UTC time for the moon (time(nullptr) is not valid
        // after deep sleep on the Watchy).
        time_t utcNow = makeTime(now) - settings.utcOffsetSeconds -
                        (settings.daylightSaving ? 3600 : 0);
        float batteryVoltage = getBatteryVoltage();

        drawMainFace(display, settings, bookmark, gYear, gMonth, gDay,
                     hour24, minute, utcNow, batteryVoltage);
        break;
      }
    }
  }

  void handleButtonPress() override {
    tmElements_t eff = effectiveTime();

    QwCommit commit = handleQuranWatchButtons(
        *this, eff.Hour, eff.Minute, eff.Day, eff.Month, eff.Year + 1970);

    if (commit.commitTime || commit.commitDate) {
      tmElements_t desired = eff;
      if (commit.commitTime) {
        desired.Hour = commit.hour;
        desired.Minute = commit.minute;
      }
      if (commit.commitDate) {
        desired.Day = commit.day;
        desired.Month = commit.month;
        desired.Year = commit.year - 1970;
      }
      // Keep the RTC's own seconds so the offset is a whole number
      // of minutes (keeps the every-minute wake-up aligned).
      desired.Second = currentTime.Second;

      tmElements_t raw = currentTime;
      int32_t offset = (int32_t)(makeTime(desired) - makeTime(raw));
      QWStorage::saveTimeOffset(offset);
    }

    showWatchFace(false);
  }
};

QuranWatch quranWatch(QURANWATCH_SETTINGS);

void setup() {
  quranWatch.init();
}

void loop() {
  // Watchy manages deep sleep itself - nothing to do here.
}