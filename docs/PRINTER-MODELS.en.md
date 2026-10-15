# Printer artwork and MQTT capabilities

[简体中文](PRINTER-MODELS.md) | English | [README](../README.en.md)

The idle home card uses the reported printer model: `info.module[name=ota].product_name` or `print.printer_type`. The xTouch logo remains visible until a valid model is reported, and for unknown models. Configured `printerModel` hints do not select artwork. Custom names remain on the device button and switch notice.

BambuStudio illustrations are converted to 112 × 112 row-compressed images with a 128-color palette and shared duplicate pixel data. They reside in program Flash and do not require an SD card. See [asset attribution and generation](../resources/printer-images/NOTICE.md). Artwork availability does not imply complete control compatibility or hardware validation for every depicted model.

The filament page displays up to four AMS tabs at once; more units scroll horizontally. The display cache holds up to eight standard four-slot units. Browsing tabs does not issue filament-switch commands. Home swatches show the browsed AMS group; tapping cycles connected groups and returning Home resets to the first group. Gray indicates an unknown color; green/orange outlines indicate feeding/RFID reading. AMS HT and mixed or multi-extruder arrangements use different identifier rules and require separate support.

Chamber telemetry accepts legacy `chamber_temper` / `ctt` and packed `device.ctc.info.temp` (low 16 bits: actual; high 16 bits: target). Missing/invalid values preserve the previous reading. The model registry distinguishes active chamber heating from temperature reporting. This change adds telemetry and capability metadata, not chamber-heating commands; the connected P1S has no active chamber heater. New multi-nozzle and dryer controls require independent validation.

Primary sources: [model configuration](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/resources/printers/C12.json), [chamber parser](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceCore/DevChamber.cpp), [AMS numbering](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceCore/DevFilaSystem.cpp), [version response parser](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceManager.cpp).

## Resource budget

Fifteen model entries share thirteen encoded images: 113,143 bytes (110.5 KiB), down from 153,920 bytes. Transparent margins are cropped; aspect ratio is preserved inside a 112px square. RLE preserves the 128-color indexed result. The decoder reads the RGB565 palette and row offsets directly from Flash, emitting one scanline without a full-frame allocation. No SD card or network download is required. The generator verifies each row and enforces a 128 KiB deduplicated artwork budget.

The single-AMS header adds humidity percentage (`humidity_raw`) and temperature (`temp`). Slot cards show valid remaining percentages (`remain`). Humidity levels are not percentages; unsupported/invalid remaining values are not zero. Multiple-unit tabs retain compact names. Delta fields preserve cached values; removal and invalid values invalidate their readings. No additional polling is introduced. See [MQTT field recommendations](MQTT-CAPABILITIES.en.md).

## Switching and remaining-filament alerts

The compact device button scrolls long names. A short tap cycles through up to four configured profiles; with more than four it opens the scrolling list. Holding opens the list in either case, with the current device marked by a green dot. Selecting a row queues the change; the close control or outside area dismisses the list. Releasing a long press does not also cycle devices. The single-object picker is drawn natively and removed asynchronously.

Slot IDs A1–A4 are pale blue. Known remaining percentages are red at 0–10%, yellow at 11–20%, and normal white above 20%. Unknown values remain omitted, and material text keeps its existing color.

The home AMS strip shows its group letter only when multiple units are connected and cycles through connected groups when tapped. Returning to Home resets it to the first group. A configured external spool is shown to the right with a `|` separator.
