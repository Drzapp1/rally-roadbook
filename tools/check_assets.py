#!/usr/bin/env python3
"""Asset-completeness check for the Rally Roadbook plugin + companions.

Codifies the manual audits that found real gaps (sign TGAs/PNGs, voice clips,
device sprites): every pictogram code must have an in-game TGA *and* a web PNG;
every clip the co-driver composer can speak must exist in the voice bank; every
device-model sprite index must have a TGA; the fonts must be present.

Run from the repo root:  python tools/check_assets.py
Exits non-zero (and lists every gap) if anything referenced is missing, so it can
gate a build.  Re-run it whenever signs / clips / device models change.
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def P(*a): return os.path.join(ROOT, *a)

problems = []
def need(path, what):
    if not os.path.isfile(path):
        problems.append(f"{what}: missing {os.path.relpath(path, ROOT)}")
        return False
    return True

# ---- pictogram codes (parsed from the plugin header, single source of truth) ----
def sign_codes():
    txt = open(P('src', 'plugin', 'model', 'roadbook.h'), encoding='utf-8', errors='replace').read()
    m = re.search(r'kAll\[\]\s*=\s*\{(.*?)\}', txt, re.S)
    if not m:
        problems.append("signs: could not parse kAll[] from roadbook.h")
        return []
    body = re.sub(r'//[^\n]*', '', m.group(1))            # strip line comments
    return re.findall(r'"([^"]+)"', body)

# ---- the distinct clips the composer (dllmain.cpp) can emit ----
# numbers 0..19 / tens / hundred / thousand, danger+direction words, signClip features.
COMPOSER_CLIPS = set("""
 zero one two three four five six seven eight nine ten eleven twelve thirteen
 fourteen fifteen sixteen seventeen eighteen nineteen twenty thirty forty fifty
 sixty seventy eighty ninety hundred thousand
 caution danger hairpin right left finish keep_straight into
 crest jump dip water bumpy ruts compression downhill sand hole ditch rocks bridge
 gate narrows tree village cliff fence
""".split())

def voice_dir():
    for d in (P('data','audio','voice'),
              P('release','RallyRoadbook','Roadbook_data','audio','voice')):
        if os.path.isdir(d): return d
    return None

def main():
    codes = sign_codes()
    print(f"pictogram codes in kAll: {len(codes)}")
    for c in codes:
        need(P('data', 'icons', f'rb_{c}.tga'), 'sign TGA (in-game)')
        need(P('web', 'signs', f'{c}.png'),     'sign PNG (web)')

    # device models — count the dicts in make_device.py, require a TGA per index
    md = open(P('tools','iconpack','make_device.py'), encoding='utf-8', errors='replace').read()
    n_models = len(re.findall(r"dict\(name=", md))
    print(f"device models declared: {n_models}")
    for i in range(n_models):
        need(P('data', 'icons', f'rb_device{i}.tga'), 'device sprite')

    # turn tulips — parsed from the renderer (kTulipCodes = single source of truth)
    rt = open(P('src','plugin','render','roadbook_renderer.cpp'), encoding='utf-8', errors='replace').read()
    tm = re.search(r'kTulipCodes\[kTulipCount\]\s*=\s*\{(.*?)\}', rt, re.S)
    tcodes = re.findall(r'"([^"]+)"', tm.group(1)) if tm else []
    if not tcodes: problems.append("tulips: could not parse kTulipCodes from roadbook_renderer.cpp")
    print(f"turn tulips declared: {len(tcodes)}")
    for c in tcodes:
        need(P('data', 'icons', f'rb_tulip_{c}.tga'), 'tulip TGA (in-game)')
        need(P('web', 'tulips', f'{c}.png'),          'tulip PNG (web)')

    # voice clips
    vd = voice_dir()
    if not vd:
        problems.append("voice: no voice-bank directory found")
    else:
        print(f"voice bank: {os.path.relpath(vd, ROOT)} ({len(COMPOSER_CLIPS)} composer clips)")
        for c in sorted(COMPOSER_CLIPS):
            need(os.path.join(vd, f'{c}.wav'), 'voice clip')

    # fonts
    for fnt in ('RobotoMono-Regular.fnt', 'RobotoMono-Bold.fnt'):
        need(P('data', 'fonts', fnt), 'font')

    print()
    if problems:
        print(f"FAIL: {len(problems)} missing asset(s):")
        for p in problems: print("  -", p)
        return 1
    print(f"PASS: all referenced assets present "
          f"({len(codes)} signs x2, {len(tcodes)} tulips x2, {n_models} devices, {len(COMPOSER_CLIPS)} clips, 2 fonts).")
    return 0

if __name__ == '__main__':
    sys.exit(main())
