"""Package fixed-name USB/OTA images without modifying source version.json."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

Import("env")


def post_build_action(source, target, env):
    root = Path(env.subst("$PROJECT_DIR"))
    build = Path(env.subst("$BUILD_DIR"))
    output = root.parent / "xtouch-bin"
    version = json.loads((root / "version.json").read_text("utf-8"))["version"]
    ota = output / "ota"
    web = output / "webusb"
    ota.mkdir(parents=True, exist_ok=True)
    web.mkdir(parents=True, exist_ok=True)
    app = ota / "xtouch-2.8.bin"
    shutil.copy2(build / "firmware.bin", app)
    image = app.read_bytes()
    manifest = {
        "version": version,
        "url": f"https://github.com/tbc0309/xtouch-CN/releases/download/{version}/xtouch-2.8.bin",
        "md5": hashlib.md5(image).hexdigest(),
        "size": len(image),
    }
    (ota / "ota.json").write_text(json.dumps(manifest, indent=2) + "\n", "utf-8")
    esptool = Path(env.PioPlatform().get_package_dir("tool-esptoolpy")) / "esptool.py"
    subprocess.run([
        env.subst("$PYTHONEXE"), str(esptool), "--chip", "ESP32", "merge_bin",
        "-o", str(web / "xtouch-2.8-full.bin"), "--flash_mode", "dio", "--flash_size", "4MB",
        "0x1000", str(build / "bootloader.bin"), "0x8000", str(build / "partitions.bin"),
        "0x10000", str(build / "firmware.bin"),
    ], check=True)
    (web / "webusb.manifest.json").write_text(json.dumps({
        "name": "xtouch-CN", "version": version,
        "builds": [{"chipFamily": "ESP32", "parts": [{"path": "xtouch-2.8-full.bin", "offset": 0}]}],
    }, indent=2) + "\n", "utf-8")


env.AddPostAction("buildprog", post_build_action)
