#pragma once
/*
  AppConfig.h
  ------------------------------------------------------------
  Central place for:
   - the watchySettings struct required by the Watchy library
   - our own application settings (location, prayer method, ...)
   - the Quran bookmark (surah / verse)
   - default values used the very first time the watch boots
    (before anything has been saved to flash)

  IMPORTANT
  The exact field names of `watchySettings` have changed a
  little between Watchy library versions. If this struct
  literal fails to compile, open the installed
  `Watchy.h` (in .pio/libdeps/watchy_v2/Watchy/src/Watchy.h)
  and match the field order/names shown there - the values
  themselves (lat/lon/gmtOffset) do not change, only the
  struct shape occasionally does.
*/

#include <Arduino.h>
#include <Watchy.h>

// ---------------------------------------------------------------
// 1) Watchy library settings (used for NTP sync, not for prayer
//    time math - our own AppSettings below is what actually
//    drives the religious calculations).
// ---------------------------------------------------------------
static watchySettings QURANWATCH_SETTINGS = {
    "000000",       // cityID       - unused by this project
    "48.8566",      // lat          - replace with your city (default: Paris)
    "2.3522",       // lon          - replace with your city (default: Paris)
    "",             // weatherAPIKey - unused
    "",             // weatherURL    - unused
    "metric",       // weatherUnit
    "en",           // weatherLang
    0,              // weatherUpdateInterval
    "pool.ntp.org", // ntpServer
    3600,           // gmtOffset (seconds) - default UTC+1 (Paris winter)
    false           // vibrateOClock
};

// ---------------------------------------------------------------
// 2) Prayer calculation methods (angle presets).
//    Umm al-Qura is a special case: Isha is a FIXED interval after
//    Maghrib (90 minutes, 120 during Ramadan) rather than an angle,
//    so it is handled separately in PrayerTimes.h.
// ---------------------------------------------------------------
enum PrayerMethod : uint8_t {
  METHOD_MWL = 0,       // Muslim World League      : Fajr 18.0  Isha 17.0
  METHOD_ISNA,          // ISNA                      : Fajr 15.0  Isha 15.0
  METHOD_EGYPTIAN,      // Egyptian General Authority: Fajr 19.5  Isha 17.5
  METHOD_KARACHI,       // Univ. of Islamic Sciences : Fajr 18.0  Isha 18.0
  METHOD_UMM_AL_QURA,   // Umm al-Qura, Makkah       : Fajr 18.5  Isha fixed
  METHOD_TEHRAN,        // Univ. of Tehran           : Fajr 17.7  Isha 14.0
  METHOD_JAKIM,         // JAKIM Malaysia            : Fajr 20.0  Isha 18.0
  METHOD_MOONSIGHTING,  // Moonsighting Committee    : Fajr 18.0  Isha 18.0
  METHOD_UOIF,          // UOIF (France)             : Fajr 12.0  Isha 12.0
  METHOD_COUNT
};

enum AsrMethod : uint8_t {
  ASR_STANDARD = 0, // Shafi/Maliki/Hanbali: shadow length = object length
  ASR_HANAFI   = 1  // Hanafi              : shadow length = 2x object length
};

struct PrayerAngles {
  float fajrAngle;
  float ishaAngle;      // ignored for METHOD_UMM_AL_QURA
  uint16_t ishaFixedMin; // only used for METHOD_UMM_AL_QURA (minutes after Maghrib)
};

// Indexed by PrayerMethod. New entries appended at the end so any
// value already saved to flash from a previous build still points
// at the same method.
static const PrayerAngles PRAYER_METHOD_TABLE[METHOD_COUNT] = {
    {18.0f, 17.0f, 0},   // MWL
    {15.0f, 15.0f, 0},   // ISNA
    {19.5f, 17.5f, 0},   // Egyptian
    {18.0f, 18.0f, 0},   // Karachi
    {18.5f, 0.0f, 90},   // Umm al-Qura (fixed 90 min, use 120 in Ramadan if desired)
    {17.7f, 14.0f, 0},   // Tehran
    {20.0f, 18.0f, 0},   // JAKIM
    {18.0f, 18.0f, 0},   // Moonsighting Committee (uses a seasonal formula normally;
                         // fixed angle is used here as a practical approximation)
    {12.0f, 12.0f, 0},   // UOIF (France)
};

static const char *PRAYER_METHOD_NAME[METHOD_COUNT] = {
    "MWL", "ISNA", "EGYPT", "KARACHI", "UMM_QURA", "TEHRAN",
    "JAKIM", "MOONSIGHT", "UOIF"};

// ---------------------------------------------------------------
// 3) Our own application settings - this is what the config
//    screen edits, and what gets saved to / loaded from flash.
// ---------------------------------------------------------------
struct AppSettings {
  float latitude;
  float longitude;
  int32_t utcOffsetSeconds; // e.g. +3600 for UTC+1
  bool daylightSaving;      // add +1h on top of utcOffsetSeconds when true
  int8_t hijriAdjustment;   // manual correction in days, typically -2..+2
  uint8_t prayerMethod;     // see PrayerMethod
  uint8_t asrMethod;        // see AsrMethod
  bool prayerVibrate;       // vibrate at Fajr, Dhuhr, Asr, Maghrib, Isha
};

// Defaults: Paris, no DST, no Hijri correction, MWL method, standard Asr.
static const AppSettings DEFAULT_APP_SETTINGS = {
    48.8566f, // latitude
    2.3522f,  // longitude
    3600,     // UTC+1
    false,    // DST off
    0,        // Hijri adjustment
    METHOD_MWL,
    ASR_STANDARD,
    true};

// ---------------------------------------------------------------
// 4) Quran bookmark (marque-page).
// ---------------------------------------------------------------
struct QuranBookmark {
  uint8_t surah; // 1..114
  uint16_t verse; // 1..versesInSurah(surah)
};

static const QuranBookmark DEFAULT_BOOKMARK = {1, 1};
