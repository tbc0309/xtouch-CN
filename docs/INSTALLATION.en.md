# Installation, updates and accounts

[简体中文](INSTALLATION.md) | English | [Documentation](README.en.md)

[Install, update and recover](#install) · [Wi-Fi and account management](#accounts) · [Retrieve and import a Token](#token)

<a id="install"></a>

## Install, update and recover

### Release files

| Board | Application for updates | Full image for USB installation |
|---|---|---|
| ESP32-2432S028R | `xtouch-2.8.bin` | `xtouch-2.8-full.bin` |
| ESP32-S3-Touch-LCD-2.8 V2 | `esp32-s3-touch-lcd-2.8-v2.bin` | `esp32-s3-touch-lcd-2.8-v2-full.bin` |
| ESP32-P4-WIFI6-Touch-LCD-4.3 | `esp32-p4-wifi6-touch-lcd-4.3.bin` | `esp32-p4-wifi6-touch-lcd-4.3-full.bin` |

Board images are not interchangeable. Application images support portal, OTA and SD updates. Each board's `ota.json` is an automatic update manifest requiring no manual editing.

Do not use the full image for OTA or rename it to the SD card's `firmware.bin`. Manually writing a full image at `0x0` overwrites NVS and loses device credential keys. Saved encrypted credentials may become unusable even if SPIFFS files remain. To retain settings, use the web installer without erasing.

### Online installation

Open the **[Web installer](https://tbc0309.github.io/xTouch-CN/)** in desktop Chrome or Edge. The page remembers the last selected board. Connect the selected board with a USB data cable, load the installer, select its port and follow the prompts. Leave **Erase device** unchecked to keep internal settings and calibration. CYD needs touch calibration after first installation; S3/P4 do not. Scan to configure Wi-Fi, then add printers in the device portal. SD is optional.

If S3/P4 cannot enter download mode automatically, power it on, hold BOOT, press and release RESET, then release BOOT. Reconnect the serial port and install again.

The installer checks the chip, Flash capacity and MD5 of each written segment. Normal installation preserves NVS and SPIFFS user data; **Erase device** erases the entire chip before installation. SD backups remain and may be imported at startup; remove the card for a fresh configuration. Encrypted credentials restored from backups may need to be entered again after erasing.

Alternatively, use the footer's [Configuration generator](https://tbc0309.github.io/xTouch-CN/config.html?lang=en) to enter Wi-Fi, LAN printers, an existing Access Token, a time zone and an NTP server. Import the downloaded JSON in the device portal, or extract the SD package at the card root. Files are generated locally in your browser and contain plaintext credentials; keep them private. Tokens and printer connections still require verification in the device portal.

### Online OTA

Enable OTA in Settings. On the next startup, the device connects to Wi-Fi and reads its [board-specific OTA manifest](DEVELOPMENT.en.md#release). Only a higher version is installed.

The application is downloaded from GitHub Releases over HTTPS, with certificate, complete length, and MD5 checks. With SD, the verified image is staged on the card, then installed into the inactive application partition after reboot. Without SD, the image is streamed directly to the inactive partition and selected only after verification. Internal Wi-Fi, printer, language, and touch calibration settings are retained.

SD staging needs enough free space for the selected board’s application image and temporary files. Direct installation does not require that card space. Both methods need network access to GitHub, its download CDN, and NTP. Keep power connected and leave the card inserted during an SD-based update. Clock synchronization or network failures skip the update and allow normal startup. Image downloads have at most two attempts. MD5 checks transfer integrity; HTTPS validates the download source.

The OTA source is `tbc0309/xTouch-CN`. Firmware configured with another update source must first be replaced with CN using the online installer or SD.

Tap the version row in Settings three times, with no more than 0.8 seconds between taps, to check and install a newer release even when startup OTA is disabled. Manifest checks have a 15-second budget covering clock synchronization, DNS, TLS and manifest reception. Wi-Fi setup, printer connection and downloading a confirmed newer firmware use separate timing. After a manual check, the printer connection resumes.

### SD updates and recovery

Configuration and backup behavior is described in [Storage](MAINTENANCE.en.md#storage).

Power off, copy the matching board's application image to the SD root as `firmware.bin`, and start the device. Before a manual update, remove `/xtouch/ota.json` and leftover `.part` files so an old manifest cannot conflict with the new image. Update files are removed after processing.

Manual SD installation without a manifest checks the ESP image format and partition size. OTA additionally checks the manifest's MD5 and length. If the device cannot boot, reinstall with the online installer. SD configuration contains access credentials; keep it private.



<a id="accounts"></a>

## Wi-Fi and account management

All boards share the web portal. Settings follow compiled capabilities. CYD, S3 V2 and P4 require their own firmware images, which are not interchangeable.

### Setup and printers

1. Without Wi-Fi settings, scan the on-screen hotspot QR code and open `http://192.168.4.1/`. Search and choose from Nearby Wi-Fi networks, or enter the name manually.
2. Once connected, visit the device address or scan **Web management** in screen settings. Use **Back to settings** below the QR code to return.
3. The initial administrator password is `12345678`; changing it is mandatory at first login. A new login replaces the previous browser session.
4. Add a LAN printer with its model, address, access code and serial number, then click **Verify & save device**. Success saves and locks the fields. Double-click to edit connection details and verify again; name changes save automatically. Add cloud printers by selecting linked devices under the corresponding account.
5. Valid changes save and apply automatically without restarting. Editing or reordering printers does not switch the printer currently shown on the screen.

Changes to connection details, account region or credentials are saved only after successful verification. Names and labels still save automatically.

The screen and management portal support Simplified Chinese, Traditional Chinese, English, French, German, Russian, Japanese and Korean. Their languages are saved independently on the device as `language` and `portalLanguage`, both defaulting to English. Changing one does not change the other. Sign-in and management share the portal language without browser-language detection or local language preferences. A choice made before sign-in is saved after successful authentication. Initial Wi-Fi setup initializes both values from the selected language; they can then be changed independently.

The public installer, configuration generator and error lookup default to English with manual Chinese/English selection. Link language parameters carry the page choice without changing device settings.

### Bluetooth provisioning (optional module)

S3/P4 builds with the optional BLE provisioning module offer Wi-Fi or Bluetooth when no network is configured. Select Bluetooth and scan the on-screen QR code inside the official **ESP BLE Provisioning** app. Choose a 2.4 GHz Wi-Fi network and enter its password. For manual discovery, use the `PROV_` prefix and proof of possession `abcd1234`. The BLE service closes after successful provisioning or three minutes. To change networks later, use Bluetooth at the bottom of the screen settings. Printer and account settings are retained. Builds without this module keep the Wi-Fi hotspot setup flow.

The initial setup chooser uses English. First-time Bluetooth provisioning initializes both languages to English. Provisioning opened from screen settings follows the current display language and retains existing language settings.

### Bambu accounts

Choose the account region, paste an **Access Token**, then validate it to retrieve linked printers. See [Get and import a Token](#token).

Successful validation collapses account details and lists linked printers for selection. Opening account management refreshes valid saved accounts; temporary network failures do not immediately invalidate them. Clearing a local login does not revoke the server Token.

Accounts and printer ownership are checked at startup and every six hours; MQTT authentication rejection also triggers a check. If a Token expires or is rejected, obtain a new one from the official website and update the existing account card.

LAN devices use cyan identifiers and Token accounts use purple. Selecting an invalid cloud profile opens a QR code targeting its account-management card.

### xTouch settings, standby and sound

Touch sounds, warning sounds and automatic deep sleep default to off. All three boards support notification sounds. Volume is adjustable on the screen and portal from 0–100%, default 20%.

Network standby turns off the display and reduces power while retaining MQTT and print wake. Optional deep sleep requires a known idle, online printer without faults, maintenance or transfers. It disconnects the portal and MQTT and cannot receive immediate print wake. Wake restarts the device without clearing configuration. All three boards support touch deep-sleep wake, and hardware reset restarts the device.

Software indicator options appear only for controllable LEDs; the RGB status LED defaults to on. S3/P4 power and charge LEDs are hardware-wired.

### Maintenance and updates

Maintenance supports configuration download, password changes, application firmware upload and reset. Upload the same board's application image, never `-full.bin`; see [Release packaging](DEVELOPMENT.en.md#release). Chip, size and partition boundaries are checked before restarting.

Three quick taps on the screen version check OTA; eight open reset confirmation. Reset clears internal configuration and the administrator password, replaces encryption keys and imports SD configuration if present. Otherwise initial setup starts. Startup OTA is configurable; manifest checks have a 15-second limit.

[Credential storage and backup safety](MAINTENANCE.en.md#security) · [Cloud authorization](PRINTERS.en.md#authorization)

### Automatic deep sleep interval

In xTouch settings → Deep sleep, enable automatic idle deep sleep and set the delay after network standby, default 60 minutes. Automatic deep sleep is off by default. Deep sleep has no timer wake: touch the screen or press RESET to start. Ordinary network standby and print wake are unaffected.

<a id="token"></a>

## Retrieve and import a Token

An Access Token authenticates a Bambu account. It is different from a printer's LAN access code. Use a desktop browser.

### Retrieve the Token

| Region | Official website |
|---|---|
| China | [bambulab.cn](https://bambulab.cn/) |
| Global | [bambulab.com](https://bambulab.com/) |

1. Open the website for your account region and sign in.
2. Press **F12** and open **Application**. Use the toolbar's `»` menu if it is hidden.
3. Expand **Storage → Cookies** and select the current Bambu website domain.
4. Find the cookie named **token** and copy its **Value**, not the entire row or cookie name.

If missing, confirm successful login, refresh and check the matching domain. Do not submit Tokens to third-party collection sites. No console script is required.

### Import into xTouch-CN

1. Open device web management and choose **Bambu accounts → Add Bambu account**.
2. Select the matching account region.
3. Paste the Token and verify it. Linked printers appear after successful verification.
4. Select printers; valid changes save and apply automatically.

Renew an existing account by updating its card instead of adding a duplicate. If the Token expires or is invalid, sign in to the official website again and repeat these steps.

Tokens are account credentials. Keep them out of screenshots, chats and public repositories; backups also contain Tokens. Successful login does not authorize every printer command; see [Cloud control](PRINTERS.en.md#authorization).

Cookie retrieval follows the [BambuHelper Token guide](https://bambuhelper.stolaris.dev/cloud-token.html). Website layouts and login policies can change.

## Thermometer (Xiaomi BLE)

Included by default in S3/P4 release firmware; unavailable on CYD. Enter the MAC in management, plus the BLE Key for encrypted broadcasts. The model is detected from the broadcast product ID; no model selection is needed. It appears after a matching broadcast is received.

| Product name | Model identifier | Product ID | Verification |
|---|---|---|---|
| Linptech Temperature and Humidity Sensor | `KS2BB` | `0x520B` | Tested on S3 |
| Xiaomi Mijia Bluetooth Thermometer 2 | `LYWSD03MMC` | `0x055B` | Tested on S3/P4 |
| Mijia Smart Thermometer 3 | `MJWSD05MMC` | `0x2832`, `0x4C47` | Protocol support; awaiting physical-device validation |
| Mijia Smart Thermometer 3 mini | `MJWSD06MMC` | `0x55B5`, `0x5BEA` | Protocol support; awaiting physical-device validation |
| Xiaomi Mijia Digital Thermometer Pro | `LYWSD02MMC` | `0x16E4`, `0x2542` | Protocol support; awaiting physical-device validation |
| Xiaomi Digital Thermometer | `XMWSDJ04MMC` | `0x1203` | Protocol support; awaiting physical-device validation |
| Mijia Bluetooth Thermometer | `LYWSDCGQ` | `0x01AA` | Protocol support; awaiting physical-device validation |
| Xiaomi Mijia Digital Thermometer Pro | `LYWSD02` | `0x045B` | Protocol support; awaiting physical-device validation |
| Qingping Bluetooth Thermometer | `CGG1` | `0x0347`, `0x0B48` | Protocol support; awaiting physical-device validation |
| Qingping Bluetooth Thermometer Lite | `CGDK2` | `0x066F` | Protocol support; awaiting physical-device validation |
| Qingping Thermometer Lite | `CGDK3` | `0x4F59` | Protocol support; awaiting physical-device validation |
| Qingping Bluetooth Alarm Clock | `CGD1` | `0x0576` | Protocol support; awaiting physical-device validation |
| MiaoMiaoce Thermometer Clock | `MHO-C303` | `0x06D3` | Protocol support; awaiting physical-device validation |
| MiaoMiaoce Bluetooth Thermometer | `MHO-C401` | `0x0387` | Protocol support; awaiting physical-device validation |
| Qingping Bluetooth Clock | `CGC1` | `0x0C3C` | Protocol support; awaiting physical-device validation |
| Miaoboshi Thermometer MBS-17 | `MBS17` | `0x5DB1` | Protocol support; awaiting physical-device validation |
| Yanmi Smart Thermometer | `ESM787` | `0x78DB` | Protocol support; awaiting physical-device validation |
| Qingping Thermometer | `CGG3` | `0x5808` | Protocol and synthetic-packet tests; no matching physical device |
| Mijia Smart Thermometer 3 Pro | `MJWSD07MMC` | `0x9AB9` | Temperature/humidity only; protocol and synthetic-packet tests, no physical device |

Supports listed product IDs using public MiBeacon v2/v3 or authenticated AES-CCM v4/v5, with temperature/humidity objects 0x1004/0x1006/0x100D and 0x4801/0x4802/0x4808. Public broadcasts need only the MAC. Encrypted broadcasts require a correct BLE Key. Legacy v2/v3 encryption, unknown product IDs and other service UUIDs are excluded; this is not universal MiBeacon support. ATC, PVVX and BTHome custom-firmware formats are not included. Other hardware revisions or broadcast formats with the same product name may differ. Temperature is displayed to one decimal place. Humidity is truncated to a whole percentage for every model. Readings update from broadcasts; unchanged values may not be reported immediately. Home display options are Hide, Alternate (default), and Sensor only. Set the alternating interval with the 1–30 second slider; the default is 5 seconds. `~` marks older readings.

### Get the MAC and BLE Key

Use [Xiaomi Cloud Tokens Extractor](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor):

1. Add the thermometer in Mi Home and complete Bluetooth binding on your phone.
2. On Windows, download the program from the project's Releases, or install dependencies and run its Python version following its instructions.
3. Sign in to the Xiaomi account used for binding and select the matching server region, such as mainland China (`cn`).
4. Find the thermometer's `MAC` and `BLE KEY` in the device list and enter them in management.

Random fictional example; only the name and model are retained from the example product:

```yaml
NAME:     领普温湿度传感器
   ID:       blt.3.ee9d0ffd6159ce
   BLE KEY:  48449a24fb4394668c1b6a1d2cd5cfa3
   MAC:      02:86:B2:90:CE:4B
   IP:       192.0.2.48
   TOKEN:    67c7a9af1dd424a539997a4814043e03
   MODEL:    linp.sensor_ht.ks2bb
```

Only `MAC` and `BLE KEY` are used; `ID`, `IP`, and cloud `TOKEN` are not needed. Encrypt & bind ties the key to this control screen. Re-enter it on another screen or after resetting the device vault.

Protocol and model references: [Linp KS2](https://github.com/Tinnci/linpk2), [Xiaomi BLE](https://github.com/Bluetooth-Devices/xiaomi-ble), [ESPHome Xiaomi BLE](https://esphome.io/components/sensor/xiaomi_ble/).

### Mi Home catalog and compatibility

The Mi Home temperature/humidity category is a product catalog, not a MiBeacon compatibility list. This module recognizes the 19 models and 23 product IDs above. A product ID identifies an advertising model; it is not a device-specific `blt.…` account ID. The model is detected automatically from advertisements.

- Zigbee Xiaomi/Aqara sensors cannot be received directly by this BLE module.
- Wi-Fi-only devices, including U-FAMILY U1 and RSH Wi-Fi variants, are outside its BLE advertisement scope.
- Other catalog products (LineHope/TH1/TH1S, TH2, KS1Pro, Deli, MiKO, MBS-7/16/20, RSH, Anseny, HW and similar brands) have not been matched to a supported MiBeacon product ID and measurement payload. They are not listed as supported based on appearance or product name.
- MJWSD07MMC provides temperature/humidity only; CO₂, history, clock/display control and active report requests are not implemented.
- Sensors determine advertisement timing and change thresholds. Home alternation timing changes the display, not the sensor reporting rate.

References: [Xiaomi BLE devices](https://github.com/Bluetooth-Devices/xiaomi-ble/blob/main/src/xiaomi_ble/devices.py), [Passive BLE Monitor parser](https://github.com/custom-components/ble_monitor/blob/master/custom_components/ble_monitor/ble_parser/xiaomi.py), [ESPHome Xiaomi BLE](https://esphome.io/components/sensor/xiaomi_ble/), [Linp KS2](https://github.com/Tinnci/linpk2). Models without physical-test labels still require validation on the corresponding hardware.

### Acquisition and display timing

After binding, the screen passively listens for advertisements for 30 ms of each 100 ms scan interval. It does not connect to the sensor or request a new measurement. The sensor determines its reporting interval and change thresholds, so no fixed delivery time is guaranteed.

Temperature and humidity can arrive separately. The home value appears only after both have been received for the bound sensor; each report age is tracked independently. A key-authentication failure never updates a reading. Restarting or changing the binding clears the readings and waits for fresh broadcasts; changing display mode or alternation timing keeps the current readings.

Home checks the latest reading every second. Alternate switches between AMS and the sensor at the configured 1–30 second interval (default 5); without AMS it shows the sensor continuously. Sensor only replaces AMS; Hide keeps receiving but hides the sensor on Home. The clock stays immediately after the camera and hides if the remaining width is insufficient. The portal refreshes report ages approximately every 5 seconds while signed in.

After either reading is more than 10 minutes old, the last-known value remains visible with `~`. This means an older received value, not a confirmed sensor fault. Saving or reloading a page cannot force a new sensor report. Keep the sensor nearby and allow time for its next measurement advertisement.
