"""Embed the readable, self-contained portal assets as deterministic gzip blobs."""
from pathlib import Path
import json
import zopfli.gzip
import subprocess
from generate_locales import generate
root=Path(__file__).resolve().parents[1]
parts=['#pragma once','#include <Arduino.h>']
for source,symbol in [('index.html','index'),('style.css','style'),('app.js','script')]:
 raw=(root/'portal'/source).read_bytes()
 if source=='index.html':raw=raw.replace(b'__FIRMWARE_VERSION__',json.loads((root/'version.json').read_text('utf-8'))['version'].encode('ascii'))
 if source=='app.js':raw=generate()+raw
 if source.endswith(('.js','.css')):
  raw=subprocess.run(['node',str(root/'scripts/minify-portal.cjs'),source.rsplit('.',1)[1]],input=raw,capture_output=True,check=True).stdout
 data=zopfli.gzip.compress(raw,numiterations=50)
 parts.append('static const uint8_t portal_'+symbol+'[] PROGMEM = {\n'+','.join(hex(v) for v in data)+'\n};')
target=root/'src/xtouch/portal_assets.h'
content='\n'.join(parts)+'\n'
if not target.exists() or target.read_text(encoding='utf-8')!=content:
 target.write_text(content,encoding='utf-8')
