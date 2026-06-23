#!/usr/bin/env python3
"""Self-contained gate: no page may LOAD a resource from an external origin —
no external <script src>, <link rel=stylesheet|preload|modulepreload>, import-map
entry, CSS @import/url(), or ES-module `import ... from 'https://…'`.

External URLs in anchors, canonical/OG/Twitter meta, and JSON-LD identifiers are
fine — those don't fetch a resource. Keeping the site self-contained means it
works offline (PWA), behind CDN-blocking networks, and leaks nothing to third
parties.

Run from repo root:  python tools/check_selfcontained.py
"""
import re, glob, os, sys

WEB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'web')
problems = []
files = sorted(glob.glob(os.path.join(WEB, '*.html'))) + sorted(glob.glob(os.path.join(WEB, '*.js')))

for f in files:
    s = open(f, encoding='utf-8', errors='replace').read()
    n = os.path.basename(f)
    # external <script src=...>
    if re.search(r'<script\b[^>]*\bsrc\s*=\s*["\']https?://', s, re.I):
        problems.append(f'{n}: external <script src>')
    # external <link> that actually loads (stylesheet / preload / modulepreload / prefetch)
    for tag in re.findall(r'<link\b[^>]*>', s, re.I):
        if re.search(r'href\s*=\s*["\']https?://', tag, re.I) and \
           re.search(r'rel\s*=\s*["\'](?:stylesheet|preload|modulepreload|prefetch)["\']', tag, re.I):
            problems.append(f'{n}: external <link> -> {tag[:70]}')
    # import-map entries pointing off-site
    for im in re.findall(r'<script\b[^>]*type\s*=\s*["\']importmap["\'][^>]*>(.*?)</script>', s, re.S | re.I):
        if re.search(r'https?://', im):
            problems.append(f'{n}: external import-map entry')
    # CSS @import / url() to an external origin
    if re.search(r'@import\s+["\']?https?://', s, re.I):
        problems.append(f'{n}: external CSS @import')
    for m in re.finditer(r'url\(\s*["\']?(https?://[^)"\']+)', s, re.I):
        problems.append(f'{n}: external CSS url() -> {m.group(1)[:50]}')
    # ES-module import from an absolute URL
    if re.search(r'\bfrom\s*["\']https?://', s):
        problems.append(f'{n}: external module import')

print(f'self-contained: scanned {len(files)} files')
if problems:
    print(f'FAIL: {len(problems)} external runtime dependency(ies):')
    for p in problems:
        print('  -', p)
    sys.exit(1)
print('PASS: no external runtime dependencies — fully self-contained.')
