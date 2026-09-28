#pragma once
/*
  InputHandler.h
  ------------------------------------------------------------
  Button state machine, menus and settings screens.
  New: PRAYER VIBRATE option (SETTINGS) + qwPrayerBuzz(), the
  100 ms ON / 120 ms OFF / 100 ms ON pattern.
*/

#include <Arduino.h>
#include <Watchy.h>
#include <Adafruit_GFX.h>
#include "UiCommon.h"
#include "AppConfig.h"
#include "QuranData.h"
#include "Storage.h"

#ifndef GxEPD_BLACK
#define GxEPD_BLACK 0
#endif
#ifndef GxEPD_WHITE
#define GxEPD_WHITE 1
#endif

// Vibration pattern: 100 ms ON -> 120 ms OFF -> 100 ms ON.
inline void qwPrayerBuzz() {
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  digitalWrite(VIB_MOTOR_PIN, HIGH);
  delay(100);
  digitalWrite(VIB_MOTOR_PIN, LOW);
  delay(120);
  digitalWrite(VIB_MOTOR_PIN, HIGH);
  delay(100);
  digitalWrite(VIB_MOTOR_PIN, LOW);
}

enum AppScreen : uint8_t {
  SCREEN_WATCHFACE = 0,
  SCREEN_MAIN_MENU,
  SCREEN_SETTINGS,
  SCREEN_SET_TIME,
  SCREEN_SET_DATE,
  SCREEN_VERSE_EDIT
};

enum MainMenuItem : uint8_t {
  MENU_SETTINGS = 0,
  MENU_SET_TIME,
  MENU_SET_DATE,
  MENU_BOOKMARK,
  MENU_ITEM_COUNT
};

enum SettingsField : uint8_t {
  FIELD_LATITUDE = 0,
  FIELD_LONGITUDE,
  FIELD_UTC_OFFSET,
  FIELD_DST,
  FIELD_PRAYER_METHOD,
  FIELD_ASR_METHOD,
  FIELD_HIJRI_ADJUST,
  FIELD_PRAYER_VIBRATE,
  FIELD_COUNT
};

struct TempTime {
  uint8_t hour;
  uint8_t minute;
  uint8_t subfield; // 0 = hour, 1 = minute
};

struct TempDate {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t subfield; // 0 = day, 1 = month, 2 = year
};

struct QwCommit {
  bool commitTime = false;
  uint8_t hour = 0, minute = 0;
  bool commitDate = false;
  uint16_t year = 0;
  uint8_t month = 0, day = 0;
};

// --- State that must survive deep sleep between button presses ---
RTC_DATA_ATTR AppScreen qwScreen = SCREEN_WATCHFACE;
RTC_DATA_ATTR uint8_t qwMenuIndex = 0;
RTC_DATA_ATTR uint8_t qwSettingsField = 0;
RTC_DATA_ATTR uint8_t qwVerseSubfield = 0;
RTC_DATA_ATTR AppSettings qwTempSettings = DEFAULT_APP_SETTINGS;
RTC_DATA_ATTR QuranBookmark qwTempBookmark = DEFAULT_BOOKMARK;
RTC_DATA_ATTR bool qwTempLoaded = false;
RTC_DATA_ATTR TempTime qwTempTime = {0, 0, 0};
RTC_DATA_ATTR TempDate qwTempDate = {2026, 1, 1, 0};

inline void qwEnsureTempLoaded() {
  if (!qwTempLoaded) {
    QWStorage::loadSettings(qwTempSettings);
    QWStorage::loadBookmark(qwTempBookmark);
    qwTempLoaded = true;
  }
}

inline uint8_t qwDaysInMonth(uint8_t month, uint16_t year) {
  static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 31;
  if (month == 2) {
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    return leap ? 29 : 28;
  }
  return days[month - 1];
}

// Tap = fine step, short hold = medium, longer hold = coarse.
inline int qwAccelStep(unsigned long heldMs, int fine, int med, int big) {
  if (heldMs > 1200) return big;
  if (heldMs > 400) return med;
  return fine;
}

inline void qwAdjustSettingsField(int8_t dir, unsigned long heldMs) {
  switch (qwSettingsField) {
    case FIELD_LATITUDE:
    case FIELD_LONGITUDE: {
      float step = (heldMs > 1200) ? 1.0f : (heldMs > 400 ? 0.1f : 0.01f);
      if (qwSettingsField == FIELD_LATITUDE) {
        qwTempSettings.latitude += dir * step;
        if (qwTempSettings.latitude > 90.0f) qwTempSettings.latitude = 90.0f;
        if (qwTempSettings.latitude < -90.0f) qwTempSettings.latitude = -90.0f;
      } else {
        qwTempSettings.longitude += dir * step;
        if (qwTempSettings.longitude > 180.0f) qwTempSettings.longitude = 180.0f;
        if (qwTempSettings.longitude < -180.0f) qwTempSettings.longitude = -180.0f;
      }
      break;
    }
    case FIELD_UTC_OFFSET:
      qwTempSettings.utcOffsetSeconds += dir * 1800;
      break;
    case FIELD_DST:
      qwTempSettings.daylightSaving = !qwTempSettings.daylightSaving;
      break;
    case FIELD_PRAYER_METHOD: {
      int m = (int)qwTempSettings.prayerMethod + dir;
      if (m < 0) m = METHOD_COUNT - 1;
      if (m >= METHOD_COUNT) m = 0;
      qwTempSettings.prayerMethod = (uint8_t)m;
      break;
    }
    case FIELD_ASR_METHOD:
      qwTempSettings.asrMethod = (qwTempSettings.asrMethod == ASR_STANDARD)
                                      ? ASR_HANAFI
                                      : ASR_STANDARD;
      break;
    case FIELD_HIJRI_ADJUST: {
      int a = (int)qwTempSettings.hijriAdjustment + dir;
      if (a < -3) a = -3;
      if (a > 3) a = 3;
      qwTempSettings.hijriAdjustment = (int8_t)a;
      break;
    }
    case FIELD_PRAYER_VIBRATE:
      qwTempSettings.prayerVibrate = !qwTempSettings.prayerVibrate;
      if (qwTempSettings.prayerVibrate) qwPrayerBuzz(); // preview
      break;
  }
}

inline void qwAdjustTimeField(int8_t dir, unsigned long heldMs) {
  if (qwTempTime.subfield == 0) { // hour, 0-23
    int step = qwAccelStep(heldMs, 1, 3, 6);
    int h = ((int)qwTempTime.hour + dir * step) % 24;
    if (h < 0) h += 24;
    qwTempTime.hour = (uint8_t)h;
  } else { // minute, 0-59
    int step = qwAccelStep(heldMs, 1, 5, 15);
    int m = ((int)qwTempTime.minute + dir * step) % 60;
    if (m < 0) m += 60;
    qwTempTime.minute = (uint8_t)m;
  }
}

inline void qwAdjustDateField(int8_t dir, unsigned long heldMs) {
  if (qwTempDate.subfield == 0) { // day
    int step = qwAccelStep(heldMs, 1, 5, 10);
    uint8_t maxDay = qwDaysInMonth(qwTempDate.month, qwTempDate.year);
    int d = ((int)qwTempDate.day - 1 + dir * step) % maxDay;
    if (d < 0) d += maxDay;
    qwTempDate.day = (uint8_t)(d + 1);
  } else if (qwTempDate.subfield == 1) { // month
    int step = qwAccelStep(heldMs, 1, 3, 6);
    int m = ((int)qwTempDate.month - 1 + dir * step) % 12;
    if (m < 0) m += 12;
    qwTempDate.month = (uint8_t)(m + 1);
    uint8_t maxDay = qwDaysInMonth(qwTempDate.month, qwTempDate.year);
    if (qwTempDate.day > maxDay) qwTempDate.day = maxDay;
  } else { // year - not cyclic, just clamp
    int step = qwAccelStep(heldMs, 1, 5, 10);
    int y = (int)qwTempDate.year + dir * step;
    if (y < 2020) y = 2020;
    if (y > 2099) y = 2099;
    qwTempDate.year = (uint16_t)y;
    uint8_t maxDay = qwDaysInMonth(qwTempDate.month, qwTempDate.year);
    if (qwTempDate.day > maxDay) qwTempDate.day = maxDay;
  }
}

inline void qwAdjustVerseField(int8_t dir, unsigned long heldMs) {
  if (qwVerseSubfield == 0) { // surah, 1-114, wraps
    int step = qwAccelStep(heldMs, 1, 5, 20);
    int s = ((int)qwTempBookmark.surah - 1 + dir * step) % QURAN_SURAH_COUNT;
    if (s < 0) s += QURAN_SURAH_COUNT;
    qwTempBookmark.surah = (uint8_t)(s + 1);
    if (qwTempBookmark.verse > versesInSurah(qwTempBookmark.surah)) {
      qwTempBookmark.verse = versesInSurah(qwTempBookmark.surah);
    }
  } else { // verse
    int step = qwAccelStep(heldMs, 1, 5, 20);
    if (step == 1) {
      if (dir > 0) bookmarkNext(qwTempBookmark);
      else bookmarkPrev(qwTempBookmark);
    } else {
      uint16_t maxV = versesInSurah(qwTempBookmark.surah);
      int v = (int)qwTempBookmark.verse + dir * step;
      if (v < 1) v = 1;
      if (v > (int)maxV) v = maxV;
      qwTempBookmark.verse = (uint16_t)v;
    }
  }
}

// ---------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------
inline void qwFieldValueString(char *out, size_t outLen, uint8_t field,
                                const AppSettings &s) {
  switch (field) {
    case FIELD_LATITUDE:
      snprintf(out, outLen, "%.2f", s.latitude);
      break;
    case FIELD_LONGITUDE:
      snprintf(out, outLen, "%.2f", s.longitude);
      break;
    case FIELD_UTC_OFFSET:
      snprintf(out, outLen, "UTC%+.1f", s.utcOffsetSeconds / 3600.0f);
      break;
    case FIELD_DST:
      snprintf(out, outLen, "%s", s.daylightSaving ? "ON" : "OFF");
      break;
    case FIELD_PRAYER_METHOD:
      snprintf(out, outLen, "%s", PRAYER_METHOD_NAME[s.prayerMethod]);
      break;
    case FIELD_ASR_METHOD:
      snprintf(out, outLen, "%s",
               s.asrMethod == ASR_HANAFI ? "HANAFI" : "STANDARD");
      break;
    case FIELD_HIJRI_ADJUST:
      snprintf(out, outLen, "%+d day", s.hijriAdjustment);
      break;
    case FIELD_PRAYER_VIBRATE:
      snprintf(out, outLen, "%s", s.prayerVibrate ? "ON" : "OFF");
      break;
  }
}

inline const char *qwFieldLabel(uint8_t field) {
  static const char *labels[FIELD_COUNT] = {
      "LATITUDE",      "LONGITUDE",  "TIME ZONE",    "DST",
      "PRAYER METHOD", "ASR METHOD", "HIJRI ADJUST", "PRAYER VIBRATE"};
  return labels[field];
}

inline void qwDrawTitle(Adafruit_GFX &d, const char *title) {
  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 2);
  d.setCursor(6, 4);
  d.print(title);
  d.drawFastHLine(0, 24, 200, GxEPD_BLACK);
}

inline void qwDrawMainMenu(Adafruit_GFX &d) {
  d.fillScreen(GxEPD_WHITE);
  qwDrawTitle(d, "MENU");

  static const char *items[MENU_ITEM_COUNT] = {"SETTINGS", "SET TIME",
                                                "SET DATE", "QURAN BOOKMARK"};
  int16_t y = 40;
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    bool selected = (i == qwMenuIndex);
    if (selected) {
      d.fillRect(4, y - 4, 192, 20, GxEPD_BLACK);
      d.setTextColor(GxEPD_WHITE);
    } else {
      d.setTextColor(GxEPD_BLACK);
    }
    qwSetText(d, 2);
    d.setCursor(10, y);
    d.print(items[i]);
    y += 28;
  }
  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  d.setCursor(6, 190);
  d.print("UP/DOWN=select  MENU=open  BACK=exit");
}

inline void qwDrawSettingsScreen(Adafruit_GFX &d) {
  d.fillScreen(GxEPD_WHITE);
  qwDrawTitle(d, "SETTINGS");

  int16_t y0 = 34, spacing = 16;
  for (int i = 0; i < FIELD_COUNT; i++) {
    int16_t y = y0 + i * spacing;
    bool selected = (i == qwSettingsField);
    if (selected) {
      d.fillRect(2, y - 3, 196, 14, GxEPD_BLACK);
      d.setTextColor(GxEPD_WHITE);
    } else {
      d.setTextColor(GxEPD_BLACK);
    }
    qwSetText(d, 1);
    d.setCursor(6, y);
    d.print(qwFieldLabel(i));

    char val[16];
    qwFieldValueString(val, sizeof(val), i, qwTempSettings);
    int16_t w = qwTextWidth((uint8_t)strlen(val), 1);
    d.setCursor(194 - w, y);
    d.print(val);
  }
  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  d.setCursor(4, 172);
  d.print("UP/DOWN=edit");
  d.setCursor(4, 182);
  d.print("MENU=next field  BACK=save");
}

inline void qwDrawValueBox(Adafruit_GFX &d, int16_t x, int16_t w,
                            const char *valueText, const char *label,
                            uint8_t valueSize, bool selected) {
  int16_t boxTop = 60, boxH = 46;
  if (selected) d.fillRect(x, boxTop, w, boxH, GxEPD_BLACK);
  else d.drawRect(x, boxTop, w, boxH, GxEPD_BLACK);
  d.setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);
  qwSetText(d, valueSize);
  int16_t vw = qwTextWidth((uint8_t)strlen(valueText), valueSize);
  int16_t vh = qwTextHeight(valueSize);
  d.setCursor(x + (w - vw) / 2, boxTop + (boxH - vh) / 2);
  d.print(valueText);

  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  int16_t lw = qwTextWidth((uint8_t)strlen(label), 1);
  d.setCursor(x + (w - lw) / 2, boxTop + boxH + 8);
  d.print(label);
}

inline void qwDrawSetTimeScreen(Adafruit_GFX &d) {
  d.fillScreen(GxEPD_WHITE);
  qwDrawTitle(d, "SET TIME");

  char hourBuf[4], minBuf[4];
  snprintf(hourBuf, sizeof(hourBuf), "%02u", qwTempTime.hour);
  snprintf(minBuf, sizeof(minBuf), "%02u", qwTempTime.minute);

  qwDrawValueBox(d, 20, 70, hourBuf, "HOUR", 4, qwTempTime.subfield == 0);
  qwDrawValueBox(d, 110, 70, minBuf, "MINUTE", 4, qwTempTime.subfield == 1);

  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  d.setCursor(6, 186);
  d.print("UP/DOWN=change  MENU=switch  BACK=save");
}

inline void qwDrawSetDateScreen(Adafruit_GFX &d) {
  d.fillScreen(GxEPD_WHITE);
  qwDrawTitle(d, "SET DATE");

  char dayBuf[4], monBuf[4], yearBuf[6];
  snprintf(dayBuf, sizeof(dayBuf), "%02u", qwTempDate.day);
  snprintf(monBuf, sizeof(monBuf), "%02u", qwTempDate.month);
  snprintf(yearBuf, sizeof(yearBuf), "%04u", qwTempDate.year);

  qwDrawValueBox(d, 4, 56, dayBuf, "DAY", 3, qwTempDate.subfield == 0);
  qwDrawValueBox(d, 68, 56, monBuf, "MONTH", 3, qwTempDate.subfield == 1);
  qwDrawValueBox(d, 132, 64, yearBuf, "YEAR", 2, qwTempDate.subfield == 2);

  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  d.setCursor(6, 186);
  d.print("UP/DOWN=change  MENU=switch  BACK=save");
}

inline void qwDrawVerseEditScreen(Adafruit_GFX &d) {
  d.fillScreen(GxEPD_WHITE);
  qwDrawTitle(d, "QURAN BOOKMARK");

  char surahBuf[8], verseBuf[8];
  snprintf(surahBuf, sizeof(surahBuf), "%03u", qwTempBookmark.surah);
  snprintf(verseBuf, sizeof(verseBuf), "%03u", qwTempBookmark.verse);

  qwDrawValueBox(d, 15, 85, surahBuf, "SURAH", 4, qwVerseSubfield == 0);
  qwDrawValueBox(d, 105, 85, verseBuf, "VERSE", 4, qwVerseSubfield == 1);

  d.setTextColor(GxEPD_BLACK);
  qwSetText(d, 1);
  d.setCursor(6, 186);
  d.print("UP/DOWN=change  MENU=switch  BACK=save");
}

// ---------------------------------------------------------------
// Button handling
// ---------------------------------------------------------------
inline QwCommit handleQuranWatchButtons(Watchy &watchy, int curHour,
                                         int curMinute, int curDay,
                                         int curMonth, int curYear) {
  QwCommit result;
  uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();

  if (wakeupBit & MENU_BTN_MASK) {
    unsigned long start = millis();
    while (digitalRead(MENU_BTN_PIN) == HIGH && (millis() - start) < 3000) {
      delay(20);
    }
    bool longPress = (millis() - start) >= 3000;

    if (qwScreen == SCREEN_WATCHFACE) {
      qwEnsureTempLoaded();
      if (longPress) {
        qwVerseSubfield = 0;
        qwScreen = SCREEN_VERSE_EDIT;
      } else {
        qwMenuIndex = 0;
        qwScreen = SCREEN_MAIN_MENU;
      }
    } else if (qwScreen == SCREEN_MAIN_MENU) {
      switch (qwMenuIndex) {
        case MENU_SETTINGS:
          qwEnsureTempLoaded();
          qwSettingsField = 0;
          qwScreen = SCREEN_SETTINGS;
          break;
        case MENU_SET_TIME:
          qwTempTime.hour = (uint8_t)curHour;
          qwTempTime.minute = (uint8_t)curMinute;
          qwTempTime.subfield = 0;
          qwScreen = SCREEN_SET_TIME;
          break;
        case MENU_SET_DATE:
          qwTempDate.year = (uint16_t)curYear;
          qwTempDate.month = (uint8_t)curMonth;
          qwTempDate.day = (uint8_t)curDay;
          qwTempDate.subfield = 0;
          qwScreen = SCREEN_SET_DATE;
          break;
        case MENU_BOOKMARK:
          qwEnsureTempLoaded();
          qwVerseSubfield = 0;
          qwScreen = SCREEN_VERSE_EDIT;
          break;
      }
    } else if (qwScreen == SCREEN_SETTINGS) {
      qwSettingsField = (qwSettingsField + 1) % FIELD_COUNT;
    } else if (qwScreen == SCREEN_SET_TIME) {
      qwTempTime.subfield = (qwTempTime.subfield + 1) % 2;
    } else if (qwScreen == SCREEN_SET_DATE) {
      qwTempDate.subfield = (qwTempDate.subfield + 1) % 3;
    } else if (qwScreen == SCREEN_VERSE_EDIT) {
      qwVerseSubfield = (qwVerseSubfield + 1) % 2;
    }
  } else if (wakeupBit & (UP_BTN_MASK | DOWN_BTN_MASK)) {
    bool isUp = (wakeupBit & UP_BTN_MASK) != 0;
    int pin = isUp ? UP_BTN_PIN : DOWN_BTN_PIN;
    int8_t dir = isUp ? +1 : -1;

    unsigned long start = millis();
    while (digitalRead(pin) == HIGH && (millis() - start) < 1500) {
      delay(20);
    }
    unsigned long heldMs = millis() - start;

    if (qwScreen == SCREEN_MAIN_MENU) {
      // List navigation: UP goes to the item above (lower index).
      int m = (int)qwMenuIndex - dir;
      if (m < 0) m = MENU_ITEM_COUNT - 1;
      if (m >= MENU_ITEM_COUNT) m = 0;
      qwMenuIndex = (uint8_t)m;
    } else if (qwScreen == SCREEN_SETTINGS) {
      qwAdjustSettingsField(dir, heldMs);
    } else if (qwScreen == SCREEN_SET_TIME) {
      qwAdjustTimeField(dir, heldMs);
    } else if (qwScreen == SCREEN_SET_DATE) {
      qwAdjustDateField(dir, heldMs);
    } else if (qwScreen == SCREEN_VERSE_EDIT) {
      qwAdjustVerseField(dir, heldMs);
    }
  } else if (wakeupBit & BACK_BTN_MASK) {
    if (qwScreen == SCREEN_MAIN_MENU) {
      qwScreen = SCREEN_WATCHFACE;
    } else if (qwScreen == SCREEN_SETTINGS) {
      QWStorage::saveSettings(qwTempSettings);
      qwScreen = SCREEN_WATCHFACE;
    } else if (qwScreen == SCREEN_SET_TIME) {
      result.commitTime = true;
      result.hour = qwTempTime.hour;
      result.minute = qwTempTime.minute;
      qwScreen = SCREEN_WATCHFACE;
    } else if (qwScreen == SCREEN_SET_DATE) {
      result.commitDate = true;
      result.year = qwTempDate.year;
      result.month = qwTempDate.month;
      result.day = qwTempDate.day;
      qwScreen = SCREEN_WATCHFACE;
    } else if (qwScreen == SCREEN_VERSE_EDIT) {
      QWStorage::saveBookmark(qwTempBookmark);
      qwScreen = SCREEN_WATCHFACE;
    }
  }

  return result;
}
