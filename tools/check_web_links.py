#!/usr/bin/env python3
"""QA gate for the web/ site: verify every internal href / src / module-import +
each manifest entry resolves to a real file. Catches dangling references after
page additions / nav inserts / renames.

Run from repo root:  python tools/check_web_links.py
Exits non-zero (listing each gap) if any local reference is missing.
"""
import os, re, sys, json, glob

WEB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'web')

def external(u):
    return (not u or u.startswith(('http://', 'https://', '//', 'data:', 'mailto:',
            'tel:', 'javascript:', '#', '{', '$')))

def resolves(u):
    u = u.split('?')[0].split('#')[0]
    if not u: return True
    return os.path.exists(os.path.normpath(os.path.join(WEB, u)))

problems, checked = [], 0
pages = sorted(glob.glob(os.path.join(WEB, '*.html')))

for f in pages:
    s = open(f, encoding='utf-8', errors='replace').read()
    name = os.path.basename(f)
    # href="" / src="" attributes
    for m in re.finditer(r'(?:href|src)\s*=\s*"([^"]+)"', s):
        u = m.group(1)
        if external(u) or '${' in u: continue
        checked += 1
        if not resolves(u): problems.append(f'{name}: href/src -> {u}')
    # ES-module imports of a same-dir file:  from './x.js'  /  from 'x.js'
    for m in re.finditer(r"""from\s*['"](\.?/?[\w./-]+\.js)['"]""", s):
        u = m.group(1)
        if external(u): continue
        checked += 1
        if not resolves(u): problems.append(f'{name}: import -> {u}')

# manifests reference real files
def check_manifest(rel, keyfn, base):
    global checked
    p = os.path.join(WEB, rel)
    if not os.path.isfile(p): return
    try: data = json.load(open(p, encoding='utf-8'))
    except Exception as e: problems.append(f'{rel}: invalid JSON ({e})'); return
    for e in data:
        tgt = keyfn(e)
        if not tgt: continue
        checked += 1
        if not os.path.exists(os.path.join(WEB, base, tgt)):
            problems.append(f'{rel} -> {base}/{tgt}')

check_manifest(os.path.join('roadbooks', 'manifest.json'), lambda e: e.get('file'), '')
check_manifest(os.path.join('devices', 'manifest.json'), lambda e: f"device{e.get('id')}.jpg", 'devices')
check_manifest(os.path.join('tracks', 'manifest.json'), lambda e: f"{e.get('id')}/meta.json", 'tracks')

print(f'checked {checked} references across {len(pages)} pages + manifests')
if problems:
    print(f'FAIL: {len(problems)} broken reference(s):')
    for p in problems: print('  -', p)
    sys.exit(1)
print('PASS: every internal reference resolves.')
