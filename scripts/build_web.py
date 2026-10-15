"""Build a static installer using verified, already-released firmware artifacts."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--firmware-dir', required=True, type=Path)
parser.add_argument('--output', default='_site', type=Path)
args = parser.parse_args()
manifest = json.loads((root / 'ota.json').read_text('utf-8'))
version = json.loads((root / 'version.json').read_text('utf-8'))['version']
if manifest['version'] != version:
    raise SystemExit('Source version and OTA manifest differ')
app = (args.firmware_dir / 'xtouch-2.8.bin').read_bytes()
full = (args.firmware_dir / 'xtouch-2.8-full.bin').read_bytes()
if len(app) != manifest['size'] or hashlib.md5(app).hexdigest() != manifest['md5']:
    raise SystemExit('Application size or MD5 mismatch')
if full[0x10000:0x10000 + len(app)] != app or len(full) >= 0x3d0000:
    raise SystemExit('Full image does not match the application or overlaps configuration')
if full[0x1000] != 0xe9 or full[0x8000:0x8002] != b'\xaa\x50':
    raise SystemExit('Bootloader or partition table missing')
source = root / 'web'
if not (source / 'vendor/install-button.js').is_file():
    raise SystemExit('Run npm ci and npm run bundle in web/ first')
output = args.output.resolve()
if output == root or root in output.parents and output.name not in ('_site', 'web-preview'):
    raise SystemExit('Choose a dedicated _site or web-preview output directory')
output.mkdir(parents=True, exist_ok=True)
# Content-based names prevent Pages/browser caches from mixing old and new modules.
config_source = (source / 'config.js').read_text('utf-8')
config_name = 'config-' + hashlib.sha256(config_source.encode()).hexdigest()[:12] + '.js'
(output / config_name).write_text(config_source, 'utf-8')
app_source = (source / 'app.js').read_text('utf-8').replace('./config.js', './' + config_name)
app_name = 'app-' + hashlib.sha256(app_source.encode()).hexdigest()[:12] + '.js'
(output / app_name).write_text(app_source, 'utf-8')
css_source = (source / 'style.css').read_text('utf-8')
css_name = 'style-' + hashlib.sha256(css_source.encode()).hexdigest()[:12] + '.css'
(output / css_name).write_text(css_source, 'utf-8')
html = (source / 'index.html').read_text('utf-8')
html = html.replace('src="app.js"', f'src="{app_name}"').replace('href="style.css"', f'href="{css_name}"')
(output / 'index.html').write_text(html, 'utf-8')
shutil.copytree(source / 'vendor', output / 'vendor', dirs_exist_ok=True)
shutil.copytree(source / 'node_modules/esp-web-tools', output / 'licenses/esp-web-tools',
                ignore=lambda directory,names: [n for n in names if n not in ('LICENSE','LICENSE.md','LICENSE.txt')],
                dirs_exist_ok=True)
(output / 'firmware').mkdir(exist_ok=True)
shutil.copy2(args.firmware_dir / 'xtouch-2.8-full.bin', output / 'firmware/xtouch-2.8-full.bin')
metadata = {'version': version, 'fullPath': 'firmware/xtouch-2.8-full.bin',
            'fullSha256': hashlib.sha256(full).hexdigest(), 'size':len(full), 'appUrl':manifest['url']}
web_manifest = {'name':'xtouch-CN','version':version,'new_install_prompt_erase':True,
                'new_install_improv_wait_time':0,'builds':[{'chipFamily':'ESP32','improv':False,
                'parts':[{'path':'firmware/xtouch-2.8-full.bin','offset':0}]}]}
for name,value in [('firmware.json',metadata),('manifest.json',web_manifest)]:
    (output / name).write_text(json.dumps(value,indent=2)+'\n','utf-8')
(output / '.nojekyll').write_text('','utf-8')
print(f'Built {version}: verified application, merged image and same-origin installer')
