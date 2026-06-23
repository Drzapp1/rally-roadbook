#!/usr/bin/env python3
"""Accessibility / SEO gate for web/: every page must declare <html lang>, a
<title>, a viewport meta, and give every <img> an alt attribute. These are
concrete, unambiguous failures — exits non-zero listing each.

Run from repo root:  python tools/check_a11y.py
"""
import glob, re, os, sys, json

WEB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'web')
problems = []
pages = sorted(glob.glob(os.path.join(WEB, '*.html')))

for f in pages:
    s = open(f, encoding='utf-8', errors='replace').read()
    n = os.path.basename(f)
    if not re.search(r'<html[^>]*\blang=', s): problems.append(f'{n}: <html> missing lang')
    if '<title>' not in s: problems.append(f'{n}: missing <title>')
    if 'name="viewport"' not in s: problems.append(f'{n}: missing viewport meta')
    if not re.search(r'<meta\s+name="description"', s): problems.append(f'{n}: missing meta description')
    for t in re.findall(r'<img\b[^>]*>', s):
        if 'alt=' not in t: problems.append(f'{n}: <img> without alt -> {t[:64]}')
    for block in re.findall(r'<script type="application/ld\+json">(.*?)</script>', s, re.S):
        try: json.loads(block)
        except Exception as ex: problems.append(f'{n}: invalid JSON-LD ({ex})')

print(f'a11y: checked {len(pages)} pages')
if problems:
    print(f'FAIL: {len(problems)} issue(s):')
    for p in problems: print('  -', p)
    sys.exit(1)
print('PASS: lang + title + viewport + description + img alt + JSON-LD all valid.')
