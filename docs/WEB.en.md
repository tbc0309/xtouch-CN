# Web installer and legacy configuration

[简体中文](WEB.md) | English | [README](../README.en.md)

Open **[Web installer](https://tbc0309.github.io/xtouch-CN/)**. Chinese browser variants use Chinese; all others use English. Switch languages in the header.

## Installation

Use desktop Chrome/Edge and a USB data cable with the supported [CYD board](HARDWARE.md). Load the installer, select its serial port and confirm installation. To retain settings, leave **Erase device** unchecked.

On first boot, calibrate the touchscreen and scan the hotspot QR code. Open `192.168.4.1` to join your 2.4GHz Wi-Fi. Open Web management from Settings, change the initial password, add LAN/cloud printers and save. SD is optional.

Serial permission and installation dialogs are supplied by ESP Web Tools in English. Phones and embedded browsers do not support serial flashing. See [Updates](UPDATING.en.md) for original SD upgrades and CN OTA.

## Legacy configuration (optional)

The footer links to the **[legacy generator](https://tbc0309.github.io/xtouch-CN/legacy.html)** (v0.9.215 only). It creates `xtouch.json` for SD import and accepts original single-printer configurations. It edits Wi-Fi, printer and startup fields, not full cloud account backups. Use the device portal for normal setup and full backup management.

Entries stay in page memory and are not uploaded or automatically saved. Preview masks credentials; downloaded files contain actual values.

## Website maintenance

- Static HTML/CSS/JavaScript: `web/`. No server database or analytics scripts.
- Configuration validation: `web/config.js`; translations: `web/app.js`.
- ESP Web Tools 10.1.0 is bundled into the site's `vendor/` directory. No external CDN script is loaded while completing the form.
- `.github/workflows/pages.yml` deploys Pages on website updates or published releases, without recompiling firmware.
- `scripts/build_web.py` checks application length and MD5, the application embedded in the full image, and configuration-partition boundaries before generating the web manifest. Artifacts come from the matching Release.

Local preview requires Node.js, Python and the two matching Release images in `release-images/`:

```sh
cd web
npm ci
npm run bundle
cd ..
python scripts/build_web.py --firmware-dir release-images --output _site
python -m http.server 8895 --directory _site
```

Open `http://localhost:8895`. ESP Web Tools is licensed under Apache-2.0; dependency notices are included in the package and generated script comments.
