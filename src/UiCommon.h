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

inline void qwSetText(Adafruit_GFX &d, uint8_t size) {
  d.setFont(nullptr); // built-in classic font (not a custom GFXfont)
  d.setTextSize(size);
}

// Exact pixel width/height of a string of `len` characters at the
// given size, using the built-in font's fixed 6x8 cell.
inline int16_t qwTextWidth(uint8_t len, uint8_t size) { return 6 * size * len; }
inline int16_t qwTextHeight(uint8_t size) { return 8 * size; }
