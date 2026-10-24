# Supported thermometers

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
