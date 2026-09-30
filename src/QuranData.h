#pragma once
/*
  QuranData.h
  ------------------------------------------------------------
  Structural data only (number of verses per surah). No surah
  names and no verse text are stored, as requested - the watch
  only ever shows numbers.

  Also provides the bookmark math used to feed the three
  progress bars (zones S / V / P):
    S = surahs fully completed / 114
    V = verses read in the CURRENT surah / verses in that surah
    P = total verses read so far / 6236
  This assumes linear, sequential reading from 1:1 up to the
  bookmark, which matches how the feature was specified.
*/

#include <Arduino.h>
#include "AppConfig.h"

constexpr uint8_t QURAN_SURAH_COUNT = 114;
constexpr uint32_t QURAN_TOTAL_VERSES = 6236;

// Verse count for surah 1..114 (index 0 = surah 1).
static const uint16_t QURAN_VERSE_COUNT[QURAN_SURAH_COUNT] = {
    7,   286, 200, 176, 120, 165, 206, 75,  129, 109,
    123, 111, 43,  52,  99,  128, 111, 110, 98,  135,
    112, 78,  118, 64,  77,  227, 93,  88,  69,  60,
    34,  30,  73,  54,  45,  83,  182, 88,  75,  85,
    54,  53,  55,  78,  75,  62,  96,  29,  18,  45,
    37,  26,  30,  38,  16,  24,  77,  182, 78,  96,
    120, 55,  78,  96,  98,  135, 38,  98,  70,  46,
    37,  45,  21,  17,  19,  36,  25,  22,  17,  19,
    26,  30,  20,  15,  21,  24,  8,   8,   11,  11,
    8,   8,   15,  8,   8,   19,  5,   8,   8,   11,
    11,  8,   3,   9,   5,   4,   7,   3,   6,   3,
    5,   4,   5,   6};

inline uint16_t versesInSurah(uint8_t surah) {
  if (surah < 1 || surah > QURAN_SURAH_COUNT) return 0;
  return QURAN_VERSE_COUNT[surah - 1];
}

inline bool isValidBookmark(uint8_t surah, uint16_t verse) {
  if (surah < 1 || surah > QURAN_SURAH_COUNT) return false;
  uint16_t maxVerse = versesInSurah(surah);
  return (verse >= 1 && verse <= maxVerse);
}

// Move to next verse, rolling into the next surah if needed.
// Wraps from surah 114 back to surah 1.
inline void bookmarkNext(QuranBookmark &bm) {
  uint16_t maxVerse = versesInSurah(bm.surah);
  if (bm.verse < maxVerse) {
    bm.verse++;
  } else {
    bm.surah = (bm.surah >= QURAN_SURAH_COUNT) ? 1 : (bm.surah + 1);
    bm.verse = 1;
  }
}

// Move to previous verse, rolling into the previous surah if needed.
// Wraps from surah 1 back to surah 114.
inline void bookmarkPrev(QuranBookmark &bm) {
  if (bm.verse > 1) {
    bm.verse--;
  } else {
    bm.surah = (bm.surah <= 1) ? QURAN_SURAH_COUNT : (bm.surah - 1);
    bm.verse = versesInSurah(bm.surah);
  }
}

// Total verses read counting sequentially from 1:1 up to (and
// including) the bookmarked verse.
inline uint32_t totalVersesRead(const QuranBookmark &bm) {
  uint32_t total = 0;
  for (uint8_t s = 1; s < bm.surah; s++) {
    total += QURAN_VERSE_COUNT[s - 1];
  }
  total += bm.verse;
  return total;
}

// Zone 7 (S): surahs fully completed out of 114.
inline uint8_t surahsCompleted(const QuranBookmark &bm) {
  return bm.surah - 1;
}

// Zone 8 (V): progress within the current surah, 0-100.
inline uint8_t currentSurahPercent(const QuranBookmark &bm) {
  uint16_t maxVerse = versesInSurah(bm.surah);
  if (maxVerse == 0) return 0;
  return (uint8_t)((uint32_t)bm.verse * 100 / maxVerse);
}

// Zone 7 (S) as a percentage, 0-100.
inline uint8_t surahsCompletedPercent(const QuranBookmark &bm) {
  return (uint8_t)((uint32_t)surahsCompleted(bm) * 100 / QURAN_SURAH_COUNT);
}

// Zone 9 (P): total Quran progress, 0-100.
inline uint8_t totalQuranPercent(const QuranBookmark &bm) {
  return (uint8_t)((uint64_t)totalVersesRead(bm) * 100 / QURAN_TOTAL_VERSES);
}