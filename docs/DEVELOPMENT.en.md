# Development guide

[简体中文](DEVELOPMENT.md) | English | [README](../README.en.md)

## Build outputs

The environment is `esp32dev`. The application is `.pio/build/esp32dev/firmware.bin`; packaged images are written to adjacent `xtouch-bin/ota/` and `xtouch-bin/webusb/`. Building does not change `version.json` or the repository's `ota.json`.

## Code structure

- `src/ui/`: LVGL screens, components, and interactions. `ui_loaders.c` handles navigation and switching notices.
- `src/ui/i18n/`: six independent language headers. `strings.c` provides shared translation lookup; application code uses `XTOUCH_TEXT_*`.
- `src/xtouch/config.h`, `printers.h`: configuration parsing for up to 100 `printers` entries and legacy single-printer `mqtt` input.
- `src/xtouch/mqtt.h`: one active printer connection, state cleanup on switching, and reconnect backoff.
- `src/xtouch/storage.h/.cpp`: atomic internal SPIFFS writes, SD synchronization, timestamp/revision conflict resolution, and deletion markers. Other modules access persistent configuration through this API.
- `src/xtouch/settings.h`: persistent settings; language values are `zh`, `zh-TW`, `en`, `fr`, `ru`, and `ja`.
- `src/xtouch/firmware.h`, `net.h`, `ota-certs.h`: version comparison, HTTPS downloads, verification, and installation.
- `resources/User_Setup.h`: hardware and display configuration. `resources/lv_conf.h`: global fonts and LVGL memory settings.

## Fonts and event lifecycle

Interface text uses W7 14px; large numbers use W7 28px. Icons use dedicated upstream fonts. Grayscale subsets cover static interface text in all six languages, rather than arbitrary external job or printer names.

Add new strings to all six language headers and `strings.h/.c`. Regenerate fonts using the instructions below, then run `python scripts/check_localization.py`. The original TTF is not distributed. Traditional Chinese strings are maintained independently and need no conversion tool at build time.

Printer and language switches are queued and applied in the main loop after the current event returns. This prevents deleting the widget handling an event. Screen transitions release the old screen before constructing the next, without refreshing the intermediate blank screen. Avoid caching multiple full screens in the limited LVGL memory pool.

See [Storage](STORAGE.en.md). Running `uploadfs` or erasing the SPIFFS partition deletes internal configuration and is not a normal firmware update step. Confirm SD backups before doing so.

## Font generation

Select Simplified Chinese, Traditional Chinese, English, French, Russian or Japanese in Settings. Changes take effect immediately, are saved internally in `/xtouch/settings.json`, and are mirrored to SD when available. Translation files live in `src/ui/i18n/`; `strings.c` provides a shared lookup table.

- Translation selection and strings: `src/ui/i18n/`. Add new text to all six languages.
- Global fonts: `LV_FONT_DEFAULT` and `XTOUCH_FONT_NUMERIC` in `resources/lv_conf.h`.
- Font generation: `scripts/localize_zh.py`; it generates fonts without rewriting application code.
- Coverage check: `python scripts/check_localization.py`.

To regenerate the subsets, install Node.js and supply a Tencent W7 TTF that you are authorized to use:

```powershell
$env:XTOUCH_W7_FONT='C:\fonts\Tencent-W7.ttf'
$env:XTOUCH_NOTO_FONT='C:\fonts\NotoSansCJKkr-Regular.otf'
python -m pip install fonttools
python scripts/localize_zh.py
python scripts/check_localization.py
python -m platformio run
```

The original TTF is not distributed. Font rights belong to their respective owners; confirm the applicable permissions before use or redistribution. The vendor fault database is included. To refresh it, install Node.js and build with `XTOUCH_REFRESH_ERRORS=1`.

## Release process

1. Update `version.json`, build, and verify translations, fonts, and artifacts.
2. Locate `../xtouch-bin/ota/xtouch-2.8.bin` and `../xtouch-bin/webusb/xtouch-2.8-full.bin`.
3. Copy generated `../xtouch-bin/ota/ota.json` to the repository root. Verify its size and MD5 against the final application image.
4. Commit source and manifest. Create a Release with a tag matching `version.json`, and upload both BIN files and `ota.json`.
5. Verify downloading, validation, installation, and reboot from an older version, then check that the same version is not installed repeatedly.

Normal builds leave the source version and repository release manifest unchanged. Publish verified artifacts. When certificate chains change, update the trusted roots in `ota-certs.h`; keep TLS verification enabled.

Printer artwork, AMS grouping and chamber telemetry: [Printer models and MQTT capabilities](PRINTER-MODELS.en.md).
