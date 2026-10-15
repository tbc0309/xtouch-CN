# xtouch-CN

[简体中文](README.md) | English

A multilingual version of [xTouch](https://github.com/xperiments-in/xtouch) for **2.8-inch, 240×320, ESP32** hardware, providing status monitoring and controls for Bambu Lab printers. Based on upstream `0.9.213`, it uses Tencent W7 for interface text and large numbers.

## Features

- Simplified Chinese, Traditional Chinese, and English, selectable in Settings and remembered across restarts.
- Up to four printer profiles, selected using the white, red, green, and blue indicators at the top.
- A separate AMS icon and a two-second printer name notice when switching printers.
- Internal configuration and touch calibration storage. After initialization, the device can run without an SD card and synchronize backups when a card is inserted.
- Project OTA updates: SD staging when a card is available, or direct installation to the inactive application partition without one.
- Silent, Standard, Sport, and Ludicrous speed modes.
- A global LVGL font covering dynamic controls, with dedicated icon fonts.
- Separate translation files and generated 14px and 28px grayscale font subsets.

HMS fault descriptions use the upstream English database. The font subset covers built-in interface text; external Chinese text, such as print job names, may contain unsupported characters.

## Download and installation

Download from [Releases](https://github.com/tbc0309/xtouch-CN/releases):

| File | Purpose |
| --- | --- |
| `xtouch-2.8-full.bin` | Full image for initial USB installation, flashed at `0x0` |
| `xtouch-2.8.bin` | Application image for OTA or SD updates; rename to `firmware.bin` for SD installation. Direct USB application updates require identifying the active partition. |

Initial setup requires an SD card containing the printer connection configuration. Once imported into internal storage, the device can operate without the card; settings and touch calibration remain on the device. See [Internal storage and SD synchronization](docs/STORAGE.en.md). Follow the [upstream instructions](README.upstream.md) for Wi-Fi setup, printer credentials, and touch calibration. Hardware support is limited to the upstream 2.8-inch design; different display drivers or pin assignments require adaptation.

```sh
python -m esptool --chip esp32 --port COM5 --baud 115200 write_flash 0x0 xtouch-2.8-full.bin
```

Replace `COM5` with the device's serial port. Enable OTA in Settings to check for project updates at the next startup. See [Updates and recovery](docs/UPDATING.en.md).

## Device interface

Screenshots from the 2.8-inch device at its native landscape resolution of 320×240, shown in Simplified Chinese. The interface language can be changed in Settings.

| Home | Temperature and fans |
| --- | --- |
| ![Home](docs/images/home.png) | ![Temperature and fans](docs/images/temperature.png) |

| Motion controls | Filament |
| --- | --- |
| ![Motion controls](docs/images/control.png) | ![Filament](docs/images/filament.png) |

| Settings and language | Project link |
| --- | --- |
| ![Settings and language](docs/images/settings.png) | ![Project link](docs/images/settings-project.png) |

## Multiple printers

Fill in the [example configuration](examples/xtouch.multiple.json), save it as `/xtouch.json` in the SD card root, and restart. All printers share the Wi-Fi settings. The `printers` array supports four slots, corresponding to the white, red, green, and blue indicators.

- `name`: optional custom name. Empty names default to `Printer 1`, etc. The name appears for two seconds during a switch.
- `host`, `accessCode`, `serialNumber`, and `printerModel`: connection parameters for each printer. The access code must contain eight characters. Replace the example addresses and serial numbers with actual values.
- `activePrinter`: default startup slot, from `0` to `3`. If unavailable, the first valid slot is selected.
- Missing, `null`, or incomplete entries are gray and disabled. Use `null` to leave an empty slot. Entries after the fourth are ignored.
- Configured slots use their assigned colors; the active slot has a gold border. Selecting another slot displays a switching notice.
- Only one printer is connected at a time. Switching disconnects the previous printer and clears its status, temperatures, job details, and faults. Offline printers use retry backoff, and other slots remain selectable.
- Switching does not save the selection; the next startup uses `activePrinter`. Legacy single-printer `mqtt` configurations are backed up as `.bak` and converted to the first slot.
- Custom Chinese names are limited by the font subset. Use Latin characters and numbers, or regenerate the font with the required characters.

Each printer requires LAN connectivity and its own address, access code, and serial number.

## Build from source

Install Python and PlatformIO, then run in the repository directory:

```sh
python -m pip install platformio
python -m platformio run
```

The build environment is `esp32dev`. Generated fonts are included; a normal build needs neither the original TTF nor Node.js. The application is generated at `.pio/build/esp32dev/firmware.bin`. Packaged files are placed in the adjacent `xtouch-bin/ota/` and `xtouch-bin/webusb/` directories. Building does not change `version.json`. See the [Development guide](docs/DEVELOPMENT.en.md).

## Languages and fonts

Select 简体中文, 繁體中文, or English at the top of Settings. Changes take effect immediately, are saved internally in `/xtouch/settings.json`, and are mirrored to SD when available. Translation files are `src/ui/i18n/zh_cn.h`, `zh_tw.h`, and `en.h`.

- Translation selection and strings: `src/ui/i18n/`. Add new text to all three languages.
- Global fonts: `LV_FONT_DEFAULT` and `XTOUCH_FONT_NUMERIC` in `resources/lv_conf.h`.
- Font generation: `scripts/localize_zh.py`; it generates fonts without rewriting application code.
- Coverage check: `python scripts/check_localization.py`.

To regenerate the subsets, install Node.js and supply a Tencent W7 TTF that you are authorized to use:

```powershell
$env:XTOUCH_W7_FONT='C:\fonts\Tencent-W7.ttf'
python scripts/localize_zh.py
python scripts/check_localization.py
python -m platformio run
```

The original TTF is not distributed. Font rights belong to their respective owners; confirm the applicable permissions before use or redistribution. The vendor fault database is included. To refresh it, install Node.js and build with `XTOUCH_REFRESH_ERRORS=1`.

## Source and licensing

Upstream project: [xperiments-in/xtouch](https://github.com/xperiments-in/xtouch). Upstream code, copyright notices, and history are retained. The code follows the [upstream license](LICENSE.md); see [third-party licenses](LICENSE-3RD-PARTY.md) for dependencies.

An independently maintained, unofficial localization.
