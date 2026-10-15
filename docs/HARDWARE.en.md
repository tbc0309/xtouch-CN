# CYD hardware and pins

[简体中文](HARDWARE.md) | [README](../README.en.md)

![Board product appearance reference](images/cyd-board-product.jpg)

Product appearance reference; the screen shows the product demonstration UI.

Released images target the **ESP32-2432S028R resistive-touch version**: classic ESP32, a 2.8-inch ILI9341 TFT, and an XPT2046 touch controller. The tested unit has an ESP32-D0WD-V3, 4MB Flash and no PSRAM. The table documents the current source configuration; other CYD revisions may use different wiring.

The native display resolution is 240×320, with a 320×240 landscape UI. Display rendering uses `TFT_eSPI` with `ILI9341_2_DRIVER`; touch uses the separate `XPT2046_Touchscreen` library and is resistive, not capacitive. Initial touch calibration is stored internally and mirrored to SD when available.

## GPIO assignments

| Module | Signal | ESP32 GPIO |
| --- | --- | --- |
| TFT | MOSI | 13 |
| TFT | SCLK | 14 |
| TFT | CS | 15 |
| TFT | DC | 2 |
| TFT | RST | 12 |
| TFT | Backlight/PWM | 21 |
| XPT2046 | MOSI | 32 |
| XPT2046 | MISO | 39 |
| XPT2046 | CLK | 25 |
| XPT2046 | CS | 33 |
| XPT2046 | IRQ | 36 |
| microSD | MOSI | 23 |
| microSD | MISO | 19 |
| microSD | SCK | 18 |
| microSD | CS | 5 |

The TFT SPI write clock is configured at 40MHz, with a 20MHz read clock. Touch is handled by its dedicated library on separate HSPI pins. microSD is used for initial configuration import, backup synchronization and SD updates; operation without a card is supported after initialization.

## Compatibility

Check the board model, resistive/capacitive touch variant, display controller, touch controller and wiring. A “CYD” label, screen size or USB connector shape alone does not establish compatibility. The firmware does not automatically detect display or touch controllers.

ST7789 displays, other resolutions, GT911/FT6236 capacitive touch, ESP32-S3 and different GPIO wiring are outside the directly supported released-image configuration. They require driver, pin and initialization changes, a new build and hardware testing.

## Source configuration

- [Display driver and TFT pins](../resources/User_Setup.h)
- [Touch driver, pins and calibration](../src/devices/2.8/touch.h)
- [Display dimensions, backlight and SD pins](../src/devices/2.8/screen.h)
- [Build environment and dependencies](../platformio.ini)
- [Upstream hardware introduction](../README.upstream.md#required-hardware)
