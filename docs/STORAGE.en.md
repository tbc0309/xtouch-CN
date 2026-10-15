# Internal storage and SD synchronization

[简体中文](STORAGE.md) | English | [README](../README.en.md)

## Storage locations

Configuration uses a 128KiB SPIFFS partition, independent of the two OTA application partitions. Usable filesystem capacity is smaller than the raw partition size.

| Data | Internal path and SD backup path |
| --- | --- |
| Wi-Fi, printer profiles, default startup printer | `/xtouch.json` |
| Language, brightness, sleep, fans, OTA settings | `/xtouch/settings.json` |
| Touch calibration | `/xtouch/touch.json` |
| Display orientation | `/xtouch/hardware.json` |

First initialization requires an SD card containing valid `xtouch.json`. Configuration and existing calibration are imported internally. The device can then boot, connect, switch printers, and save settings without SD. Valid internal calibration is reused at startup.

The initial import reads display orientation from internal `/eeprom.bin` when present. The filesystem is created only when the configuration partition is entirely blank. A mount failure on a populated partition displays an error and preserves the data.

## SD files

| File | Purpose | Synchronized internally? |
| --- | --- | --- |
| Root `xtouch.json` | Wi-Fi and up to 100 printers | Yes |
| `xtouch/settings.json` | Interface and device settings | Yes |
| `xtouch/touch.json` | Device-specific touch calibration | Yes, including deletion state |
| `xtouch/hardware.json` | Display orientation | Yes |
| `xtouch/ota.json` | Temporary download manifest | No; removed after update checking or installation |
| Root `firmware.bin` | Staged application | No; removed after processing |
| `*.bak` | Original configuration preserved during import | Kept on SD, not overwritten by normal saves |
| `*.previous` | Previous normal save | Maintained separately on each medium |
| `*.conflict`, `*.invalid` | Latest conflicting or invalid input | Not automatically loaded |
| `*.tmp`, `*.part` | Intermediate writes and downloads | No |

Only the four configuration files are synchronized. Temporary and unrelated files are excluded. SD is optional after initialization. Unneeded backups may be removed after confirming the configuration. Touch calibration belongs to its specific device.

## Importing upstream configuration

Before replacing an upstream JSON file, the firmware preserves and verifies an original `.bak` beside it. Existing `.bak` files are not overwritten by normal saves, which use `.previous`. A single-printer `mqtt` object becomes the first `printers` entry with `activePrinter: 0`; Wi-Fi, access code, serial number, model, and other existing fields are retained. Valid brightness, sleep, fan settings, and calibration are also imported. Missing language settings default to Simplified Chinese.

If backup fails, the original SD file is left intact. Retry with a writable card. Original backups also contain credentials and must remain private.
## Synchronization rules

Files are compared independently. Card insertion is normally detected within about ten seconds; probing is deferred briefly after touchscreen interaction. The board has no card-detect pin, so a bounded SPI probe checks an empty slot without repeated long SD-driver retries.

The firmware manages JSON `_storage` metadata: revision, content digest, modification time, and synchronization state. When editing Wi-Fi or printer details, this block may be kept or removed; it needs no manual maintenance.

1. Import valid SD data if no valid internal copy exists.
2. Do not re-import identical content. Complete any pending backup write.
3. If both modification times are credible and differ by more than FAT's approximately two-second precision, use the newer copy.
4. Without a synchronized clock, or when the order is uncertain, preserve unsynchronized internal changes. Otherwise accept valid externally edited SD data or a higher revision.
5. Preserve the latest conflict as `.conflict`, and invalid SD JSON as `.invalid` where possible. Oversized or unwritable files that cannot be backed up are not forcibly overwritten.

After Wi-Fi connects, NTP provides the clock. Until then, revisions and pending-change flags are used instead of recording a 1970 timestamp. A valid date is added once available. Offline edits made using unsynchronized clocks cannot always be ordered reliably; keep conflict backups until the configuration is confirmed.

Importing changed SD configuration while running triggers a restart to apply a consistent configuration. Writing a backup alone does not restart the device. Invalid Wi-Fi, printer, or calibration JSON cannot replace valid internal data.

## Saving

Changes are first written to an internal temporary file, read back, verified, and committed. The SD backup is then updated. With no card or a failed card write, internal changes remain pending synchronization. Identical content causes no repeated Flash write or revision increment.

Both storage media use temporary files, validation, and backup renames. Normal saves rotate `.previous`. If an internal primary file is damaged or a rename was interrupted, startup attempts recovery from a valid `.previous`, with support for earlier `.bak` backups. Incomplete JSON is not accepted as a new configuration.

Resetting calibration writes a deletion marker. Reinserting an SD card after an offline reset does not restore the stale calibration.

## Firmware updates

`firmware.bin` in the SD root is the staged application image. Detecting a recognizable image after card insertion restarts the device into installation. Manual images are checked for ESP format and partition size; an accompanying OTA manifest also requires matching length and MD5. Staged files are removed after processing.

Without SD, the small OTA manifest is cached internally and the application is streamed to the inactive partition. Selection changes only after complete download, MD5, and ESP image checks. Failure aborts installation and retains the previous application. Updating an application does not overwrite the configuration partition. See [Updates and recovery](UPDATING.en.md).

For a damaged configuration filesystem, first preserve SD backups. With the current partition table, recovery can erase only the `0x20000` bytes starting at `0x3D0000`, then restore from SD at startup. Full-chip erasure also deletes internal configuration; back up before erasing.

Configuration contains Wi-Fi passwords and printer access codes. Internal storage does not imply Flash encryption. Keep configuration and backups private.
