# QuranWatch

**A prayer-times, Hijri-calendar and Quran-reading-tracker watch face for the Watchy e-paper smartwatch.**

QuranWatch turns a Watchy (v2 and compatible clones) into a dedicated companion for daily worship. Everything is computed on the watch itself. It needs no phone, no Wi-Fi and no account, and it shows only numbers and short abbreviations, so it is not tied to any interface language.

```
+--------+----------------+----------+
| Moon   |  27 Ram        |  S011    |
| phase  |  1448          |  :036    |
|  96%   |  BAT 91%       |          |
+--------+----------------+----------+
| 01 02 03|      14       | Quran %  |
| 04 05 06|     ●●○○      |  ████    |
| 07 08 09|               |  ████    |
| (10)11 12|              |  42%     |
+--------+----------------+----------+
| F 06:19 | S 07:46 | D 13:05        |
| A 15:51 | M 18:22 | I 19:49        |
+--------------------------------------+
```

## Why QuranWatch

- **Fully offline.** Prayer times, the Hijri date and the moon phase are calculated on the watch from your coordinates and the date. Nothing is fetched from the internet.
- **Everything at a glance.** Nine information zones fit on the 200×200 screen: moon, Hijri date, battery, Quran bookmark, hour, Hijri month grid, reading progress and the five daily prayers plus sunrise.
- **Set up on the watch.** Latitude, longitude, time zone, DST, prayer method, Asr method, Hijri correction, time, date and bookmark are all edited with the four buttons. There is no companion app and no reflashing to change a setting.
- **Easy to scroll.** Values such as latitude, longitude, hour, minute, day, month, surah and verse step by small increments on a tap and by larger ones on a longer press.
- **Vibration at prayer time (optional).** The watch gives a short double pulse (100 ms on, 120 ms off, 100 ms on) at Fajr, Dhuhr, Asr, Maghrib and Isha. It can be switched off in Settings.
- **E-paper readability.** The display is high contrast, always on, and readable in sunlight. The current prayer is shown as an inverted (white-on-black) cell.
- **Works on Watchy clones.** Time and date are stored as an offset in flash and the RTC chip is never written, which avoids write problems seen on some Watchy v2 clones.
- **Maintainable code.** The project is split into small single-purpose files (prayer times, Hijri calendar, moon, Quran data, storage, UI, input), so changes stay local.

## Features

### Prayer times
- Fajr, sunrise (S), Dhuhr, Asr, Maghrib and Isha, shown in a 2×3 grid.
- Calculated from the sun's position (equation of time, declination and hour angle) for your latitude and longitude.
- Calculation methods: **MWL, ISNA, Egyptian, Karachi, Umm al-Qura, Tehran, JAKIM, Moonsighting Committee (fixed-angle approximation), UOIF (12°/12°)**.
- Asr method: **Standard (Shafi/Maliki/Hanbali)** or **Hanafi**.
- The current prayer period is highlighted by inverting its cell.

### Hijri calendar
- A 12-month grid (01 to 12) with the **current Hijri month circled**.
- The information zone shows the Hijri day with a month abbreviation (Muh, Saf, R1, R2, J1, J2, Raj, Sha, Ram, Shw, DQ, DH) and the Hijri year.
- Manual correction of −3 to +3 days to align with your local moon-sighting authority.

### Moon phase
- Drawn mathematically, with no bitmaps, so the shape follows the real illuminated fraction day after day.
- Uses the Meeus phase-angle formulas, which are accurate to a fraction of a percent, and shows the illuminated percentage in the corner of the zone.

### Quran reading tracker
- A bookmark stored as **surah:verse**. No names and no text are displayed.
- A progress bar shows the percentage of the whole Quran (6,236 verses) reached by the bookmark.
- The bookmark is edited on the watch. Holding MENU for 3 seconds jumps straight to the editor.

### Clock
- The hour is shown large, with four dots underneath that fill each quarter of the hour (`●○○○` → `●●○○` → `●●●○` → `●●●●`).

## Controls

| Button | On the watch face | In menus and editors |
|---|---|---|
| **MENU** (short press) | Open the main menu | Enter the highlighted item / next field |
| **MENU** (hold 3 s) | Open the Quran bookmark editor | — |
| **UP** | — | Move up / increase (hold to scroll faster) |
| **DOWN** | — | Move down / decrease (hold to scroll faster) |
| **BACK** | — | Save and return to the watch face |

The main menu has four entries: **Settings**, **Set time**, **Set date** and **Quran bookmark**. Button names follow the Watchy library. On some units the physical positions may differ, and only the pin macros need to be swapped.

## Installation

Requirements: [PlatformIO](https://platformio.org/) and a Watchy v2 or a compatible clone.

```bash
git clone <your-repo-url>
cd QuranWatch
pio run -t upload
```

`platformio.ini`:

```ini
[env:watchy_v2]
platform = espressif32 @ ~6.6.0
board = esp32dev
framework = arduino
lib_deps = sqfmi/Watchy
lib_ldf_mode = deep+
board_build.partitions = min_spiffs.csv
upload_speed = 115200
monitor_speed = 115200
```

**First use:** open **Menu → Settings** and enter your latitude, longitude, UTC offset and DST setting, then choose a prayer method. Then use **Set time** and **Set date**. All settings are kept in flash.

## Project structure

```
QuranWatch/
├── platformio.ini
└── src/
    ├── main.cpp          # Watchy lifecycle, time offset, prayer vibration
    ├── AppConfig.h       # settings, prayer methods, defaults
    ├── QuranData.h       # verses per surah, bookmark and progress maths
    ├── MoonPhase.h       # lunar phase calculation and drawing
    ├── PrayerTimes.h     # solar prayer-time calculation
    ├── HijriCalendar.h   # Gregorian to Hijri conversion
    ├── Storage.h         # settings saved in flash
    ├── UiCommon.h        # text helpers
    ├── UiLayout.h        # watch face drawing
    └── InputHandler.h    # buttons, menus and editors
```

## Accuracy and limitations

- **Prayer times** follow standard solar-angle calculations. Accuracy depends on correct coordinates, UTC offset and DST settings. Check the times against your local mosque or authority.
- **The Hijri calendar** uses the standard arithmetic (tabular) conversion, not a real crescent-visibility calculation. It can differ by a day from a sighting-based calendar, so use the ±3 day correction to match your local authority.
- **Moonsighting Committee** uses a fixed 18°/18° approximation of its seasonal method.
- **The moon** is drawn with the lit side on the right when waxing (northern hemisphere view).
- **If the RTC loses power** (for example a fully discharged battery), set the time and date again.
- Developed and tested on a Watchy v2 clone. Other hardware revisions may need small adjustments, and the Watchy library API can change between releases.

## Credits

- [Watchy](https://github.com/sqfmi/Watchy) by SQFMI, the hardware and base library.
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) for graphics.
- Lunar phase formulas from Jean Meeus, *Astronomical Algorithms*.

## License

GPL-3.0 license
