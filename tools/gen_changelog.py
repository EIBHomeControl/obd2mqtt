#!/usr/bin/env python3
"""Erzeugt aus changelog.json: CHANGELOG.md (de), CHANGELOG.en.md (en) und das JS-Array in src/web_ui.h.
Aufruf: python3 tools/gen_changelog.py  (im Projektordner)"""
import json, re
c = json.load(open('changelog.json', encoding='utf-8'))
for lang, fn, title in (('de', 'CHANGELOG.md', 'Changelog'), ('en', 'CHANGELOG.en.md', 'Changelog')):
    out = [f'# {title}\n']
    for e in c:
        out.append(f"## {e['v']}")
        out += [f'- {x}' for x in e[lang]]
        out.append('')
    open(fn, 'w', encoding='utf-8').write('\n'.join(out))
p = 'src/web_ui.h'
s = open(p, encoding='utf-8').read()
js = json.dumps(c, ensure_ascii=False)
s, n = re.subn(r'const CHANGELOG=\[.*?\];\n', lambda m: 'const CHANGELOG=' + js + ';\n', s, count=1, flags=re.S)
assert n == 1, 'CHANGELOG nicht gefunden'
open(p, 'w', encoding='utf-8').write(s)
print('Changelog erzeugt:', len(c), 'Versionen')
