# Updates and recovery

[简体中文](UPDATING.md) | English | [README](../README.en.md)

## Release files

- `xtouch-2.8-full.bin`: full USB image for initial installation at `0x0`.
- `xtouch-2.8.bin`: application image for OTA and SD updates. OTA alternates between two application partitions; flashing only `0x10000` may not update the active partition. Use the full image for routine USB updates.
- `ota.json`: automatic update manifest with version, application URL, size, and MD5. No manual editing is needed.

Do not use the full USB image for OTA or rename it to the SD card's `firmware.bin`. Flashing the full image at `0x0` resets application selection and does not cover SPIFFS. A flashing tool's full-chip erase option still deletes internal configuration.

## Upgrade from upstream firmware using SD

$\color{red}{\text{Original xTouch firmware supports upgrading directly to xtouch-CN via SD card.}}$

1. Download this project's `xtouch-2.8.bin`, rename it `firmware.bin`, and put it in the SD root. Use the application image, not the full USB image or an upstream firmware image.
2. Keep the existing `xtouch.json`, settings, and calibration files in the `xtouch` folder. Remove any old `xtouch/ota.json`.
3. Insert the card and boot. The upstream SD installer writes the CN application to the inactive partition and restarts.
4. At first startup, CN backs up original configuration as `.bak`, converts it to the multiple-printer format, and imports it into internal storage. After initialization, the device can operate without SD.

CN firmware already using this project's OTA source can check for updates by enabling OTA and restarting. Configuration is retained; identical versions are not reinstalled.
## USB installation

Flash `xtouch-2.8-full.bin` at `0x0`:

```sh
python -m esptool --chip esp32 --port COM5 --baud 115200 write_flash 0x0 xtouch-2.8-full.bin
```

Replace `COM5` with your serial port. For a new device, prepare the SD configuration and complete calibration as described in the [README](../README.en.md#first-time-setup).

## Online OTA

Enable OTA in Settings. On the next startup, the device connects to Wi-Fi and reads the project's [ota.json](https://raw.githubusercontent.com/tbc0309/xtouch-CN/main/ota.json). Only a higher version is installed.

The application is downloaded from GitHub Releases over HTTPS, with certificate, complete length, and MD5 checks. With SD, the verified image is staged on the card, then installed into the inactive application partition after reboot. Without SD, the image is streamed directly to the inactive partition and selected only after verification. Internal Wi-Fi, printer, language, and touch calibration settings are retained.

SD staging needs at least 2MB of free card space. Direct installation does not require that card space. Both methods need network access to GitHub, its download CDN, and NTP. Keep power connected and leave the card inserted during an SD-based update. Clock synchronization or network failures skip the update and allow normal startup. Image downloads have at most two attempts. MD5 checks transfer integrity; HTTPS validates the download source.

The OTA source is `tbc0309/xtouch-CN`. Firmware configured with another update source must first be replaced with CN using USB or SD.

Tap the version row in Settings three times, with no more than 0.8 seconds between taps, to check and install a newer release even when startup OTA is disabled. Manifest checks have a 15-second budget covering clock synchronization, DNS, TLS and manifest reception. Wi-Fi setup, printer connection and downloading a confirmed newer firmware use separate timing. After a manual check, the printer connection resumes.

## SD updates and recovery

Configuration and backup behavior is described in [Storage](STORAGE.en.md).

Power off, copy `xtouch-2.8.bin` to the SD root as `firmware.bin`, and start the device. Before a manual update, remove `/xtouch/ota.json` and leftover `.part` files so an old manifest cannot conflict with the new image. Update files are removed after processing.

Manual SD installation without a manifest checks the ESP image format and partition size. OTA additionally checks the manifest's MD5 and length. If the device cannot boot, reinstall the full USB image at `0x0`. SD configuration contains access credentials; keep it private.
