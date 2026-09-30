#pragma once
/*
  UiCommon.h
  ------------------------------------------------------------
  Why this file exists: the previous version used the Adafruit
  "FreeSans" family everywhere. Those are PROPORTIONAL fonts -
  every character has a different width, and that width is
  wider and taller than it looks in the font's name. In a dense
  200x200 grid with many small cells, strings routinely ran
  past the column they were drawn in and smeared into whatever
  was next to it - exactly the overlapping text seen in your
  photos.

  The fix: use the classic BUILT-IN GFX font (Adafruit_GFX's
  default 5x7 font), scaled with setTextSize(). It is perfectly
  monospaced: every character occupies exactly
    width  = 6 * size  pixels
    height = 8 * size  pixels
  which makes it possible to compute an exact width for any
  string and guarantee it never crosses a border.

  qwSetText(display, size) is the only thing you need: it
  selects the built-in font and applies the size.
*/

#include <Adafruit_GFX.h>
#include <string.h>

#ifndef GxEPD_BLACK
#define GxEPD_BLACK 0
#endif
#ifndef GxEPD_WHITE
#define GxEPD_WHITE 1
#endif

// Software display inversion, for clones whose controller ignores
// display.invertDisplay(). Set qwInvertColors once per frame (see
// main.cpp, before any drawing) from AppSettings.invertDisplay.
// Every draw call in this project uses qwInk() for "ink" (text,
// lines, filled shapes) and qwPaper() for "background/erase"
// instead of GxEPD_BLACK/GxEPD_WHITE directly, so flipping this one
// flag inverts the whole screen consistently.
inline bool qwInvertColors = false;
inline uint16_t qwInk() { return qwInvertColors ? GxEPD_WHITE : GxEPD_BLACK; }
inline uint16_t qwPaper() { return qwInvertColors ? GxEPD_BLACK : GxEPD_WHITE; }

inline void qwSetText(Adafruit_GFX &d, uint8_t size) {
  d.setFont(nullptr); // built-in classic font (not a custom GFXfont)
  d.setTextSize(size);
}

// Exact pixel width/height of a string of `len` characters at the
// given size, using the built-in font's fixed 6x8 cell.
inline int16_t qwTextWidth(uint8_t len, uint8_t size) { return 6 * size * len; }
inline int16_t qwTextHeight(uint8_t size) { return 8 * size; }

// True visible ink width, for CENTERING text: qwTextWidth() includes
// the 1-pixel trailing gap after the last character (needed for
// spacing to a following character, but never actually drawn), so
// using it to center text leaves the visible glyphs off-center by
// half that gap. This subtracts it.
inline int16_t qwTextInkWidth(uint8_t len, uint8_t size) {
  if (len == 0) return 0;
  return (6 * len - 1) * size;
}