# Configuration generator and web installer

[简体中文](WEB.md) | English | [README](../README.en.md)

Open **[Configuration & web installer](https://tbc0309.github.io/xtouch-CN/)**. The initial language follows the browser: Chinese variants use Chinese, all others use English. You can switch in the header.

## Create configuration

1. Enter Wi-Fi credentials; leave the password empty for an open network.
2. Enter each printer's address, LAN access code and serial number, and choose its model. Add up to 100 profiles, reorder/remove them, and choose the startup printer.
3. Check configuration. The preview masks passwords and access codes; the downloaded file contains the actual values.
4. Download `xtouch.json`, place it in the SD root, insert the card and boot to import.

Import accepts upstream single-printer `mqtt` and CN `printers` configurations. Import replaces the form, retaining only Wi-Fi, printer and startup-delay fields. Empty slots are removed and the startup index is adjusted. It does not edit calibration or other settings files. Review before exporting.

Entries remain in page memory: they are not sent to a server or automatically stored in the browser. Refreshing or closing loses unsaved entries. Downloaded configurations contain credentials and must remain private.

## Online installation

Use the supported [ESP32-2432S028R hardware](HARDWARE.en.md); screen size alone does not establish compatibility.

1. Open the HTTPS site in desktop Chrome / Edge, connect the board with a data cable and close other serial software.
2. Select Web installer, load the tool, then connect and select the serial port.
3. Confirm installation in ESP Web Tools. **To retain settings, leave Erase device unchecked. Erasing removes internal settings and calibration.**
4. Prepare the SD configuration and boot. Complete calibration if prompted.

The page supports Chinese and English; ESP Web Tools connection, progress and confirmation dialogs are currently English. If Web Serial, port access or networking is unavailable, use [SD installation](UPDATING.en.md). The website does not send connection configuration over serial: SD import is still required.

The installer writes the same-origin, released full image at `0x0`. Port selection and installation require user actions; the page does not install automatically. Mobile and embedded browsers are not supported targets.

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
