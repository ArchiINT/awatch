# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`awatch` is an ESP32-S3 smartwatch firmware built with ESP-IDF. It drives an ST7789 240×280 LCD over SPI2 and renders fonts, shapes, and time.

## Build & Flash Commands

Requires ESP-IDF to be sourced (`. $IDF_PATH/export.sh`).

```sh
idf.py build                        # Compile
idf.py flash                        # Flash to device
idf.py monitor                      # Serial monitor
idf.py flash monitor                # Flash then monitor
idf.py menuconfig                   # Configure sdkconfig options
idf.py -p /dev/ttyUSB0 flash        # Flash to specific port
```

No tests exist in this project.

## Architecture

### Component Structure

- **`main/`** — App entry point (`app_main` in `awatch_test.c`). Initialises SPI + display, then runs the main loop.
- **`st7789_sdi/`** — Custom ESP-IDF component for the ST7789 display driver. Registered via `EXTRA_COMPONENT_DIRS` in the top-level `CMakeLists.txt`.

### st7789_sdi Component

| File | Role |
|------|------|
| `st7789_init.c/.h` | SPI bus init, low-level `send_cmd`/`send_data`, display init sequence, backlight, `fill_color` |
| `st7789_draw.c/.h` | Pixel, buffer, character, string, thick-pixel, and circle drawing |
| `fonts.c/.h` | `FontDef_t` struct; active fonts selected by `#define` macros in `fonts.h` |
| `font5x7.c/.h` | Raw bitmap data for the 5×7 font |

### Pin Mapping (SPI2 / ESP32-S3)

| Signal | GPIO |
|--------|------|
| MOSI (SDA) | 2 |
| CLK (SCL) | 1 |
| CS | 5 |
| DC | 4 |
| RST | 3 |
| Backlight | 6 |

### Display

- ST7789, 240×280, RGB565 (16-bit colour)
- Colours are byte-swapped before sending: `swap = (color >> 8) | (color << 8)`
- `st7789_fill_color` uses a 1024-pixel chunk buffer to avoid large stack allocations
- `st7789_send_buffer(x, y, ex, ey, buf, len)` sets a window and bulk-writes pixels — use this for efficient multi-pixel draws

### Fonts

Enable/disable font sizes in `fonts.h` via `#define`:
- `FONT_11x18` — enabled
- `FONT_16x26` — enabled
- `FONT_16x28` — enabled (numbers only)
- `FONT_6x8`, `FONT_7x9` — commented out

`get_char(font, c)` indexes into `font->data` at `(c - 0x20) * font->FontHeight` — each row is one `uint16_t` bitmask (MSB = leftmost pixel).

### Colour Utilities

`next_color()` — steps through the HSV hue wheel (+4° per call, full saturation/value), returns RGB565. Used to animate the circle colour in the main loop.

`random_color()` — golden-angle hue stepping (+137°), also returns RGB565.
