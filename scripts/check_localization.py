"""Validate generated glyph coverage without connecting to a printer."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
font = (root / 'src/ui/fonts/ui_font_tencent_w7_14.c').read_text('utf-8')
covered = {int(cp,16) for cp in re.findall(r'/\* U\+([0-9A-Fa-f]+)',font)}
missing = []
for path in (root / 'src').rglob('*'):
    if path.suffix not in ('.c', '.cpp', '.h') or 'fonts' in path.parts:
        continue
    source = path.read_text('utf-8-sig')
    # Only string literals are relevant; comments must not enlarge the subset.
    for value in re.findall(r'"((?:\\.|[^"\\])*)"', source):
        for ch in value:
            if ord(ch)>=160 and ch.isprintable() and ord(ch) not in covered:
                missing.append((str(path.relative_to(root)), ch))
assert not missing, f'Missing W7 glyphs: {missing}'
assert 'lv_font_get_glyph_dsc_fmt_txt' in font
assert '.fallback = &lv_font_montserrat_14' in font
config = (root / 'resources/lv_conf.h').read_text('utf-8')
assert '#define LV_FONT_DEFAULT &ui_font_tencent_w7_14' in config
assert 'LV_FONT_DECLARE(ui_font_tencent_w7_14)' in config
assert 'LV_FONT_DECLARE(ui_font_tencent_w7_28)' in config
translations = []
for filename, suffix in [('zh_cn.h','ZH'),('zh_tw.h','TW'),('en.h','EN'),('fr.h','FR'),('de.h','DE'),('ru.h','RU'),('ja.h','JA'),('ko.h','KO')]:
    source = (root / 'src/ui/i18n' / filename).read_text('utf-8')
    translations.append(dict(re.findall(r'#define (XTOUCH_TEXT_\w+)_' + suffix + r' (".*")', source)))
assert all(t.keys()==translations[0].keys() for t in translations), 'Language keys differ'
for key in translations[0]:
    formats = [re.findall(r'%(?:\d+\$)?[dsu]', language[key]) for language in translations]
    assert all(f==formats[0] for f in formats), f'Format mismatch: {key}'
print(f'PASS: {len(translations[0])} complete eight-language translation keys')
print(f'PASS: {len(covered)} font glyphs; all localized source strings covered')
