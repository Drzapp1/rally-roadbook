# MX Bikes — Rally Roadbook

A native MX Bikes plugin that teaches **rally-raid roadbook navigation**. Ride a
track once; the plugin records your line, auto-generates a real-style roadbook
(tulip diagrams + distances + compass headings), and renders it in-game as an
**ERTF/F2R-style rally computer** so you can learn to navigate by it — tulips,
CAP headings, trip meter, danger signs, the lot.

## 🌐 Live web companion

A full **browser companion** is live — no install needed — to explore, learn, build and try roadbook navigation:

### ▶ https://drzapp1.github.io/rally-roadbook/

| Explore | Learn & play | Create | Reference |
|---|---|---|---|
| roadbook viewer · 3D real terrain · route 3D · **64-stage library** | Academy course · daily challenge · stage practice · **spoken co-driver** · **rally-raid marathon** · tulip + CAP drills | route builder · roadbook editor · **auto-generate stages from real & procedural terrain** | photo-real device skins · installable mobile reader · open JSON format + validator + embed module |

The plugin and the site share one **open roadbook format** ([schema](https://drzapp1.github.io/rally-roadbook/roadbook.schema.json) · [validator](https://drzapp1.github.io/rally-roadbook/format.html) · [embed module](https://drzapp1.github.io/rally-roadbook/roadbook.js)). See the [20× roadmap](ROADMAP.md).

## How it works

```
Ride  ──►  Recorder (ride_*.csv)  ──►  Tulip generator  ──►  roadbook_<track>.json
                                                                     │
Re-ride the track  ◄──  In-game renderer + map-matching  ◄──────────┘
```

- **Recorder** logs telemetry (position, heading, speed) at 100 Hz to
  `Documents\PiBoSo\MX Bikes\Roadbook\rides\`.
- **Generator** resamples + smooths the line, detects waypoints (turns,
  hairpins, forced straights), and builds tulip diagrams + a route polyline.
- **Map-matching** snaps your live position onto the recorded route every frame,
  so your **trip distance is always aligned** and the active box reflects where
  you actually are.

## Install

Copy into your MX Bikes install (`<MXB>\plugins\`):
```
Roadbook.dlo
Roadbook_data\        (fonts, icons, NOTICE)
```
Steam path: `C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\`.
A `.dlo` auto-loads; rename to `.dlo.off` to disable.

## Quick start

1. Load a track and ride the route.
2. **F4 → Save roadbook now (from ride)** (pausing/leaving also auto-generates).
3. Re-ride — the rally computer auto-advances through the cases by distance.
4. **F4 → Restart recording** to begin a fresh recording from where you are.

## The rally computer (headline feature)

The roadbook is framed as a real **rally computer** with **8 selectable models**
— Carbon, Tablet, Classic ICO, Stealth, Desert/Dakar, Night, Retro CRT, Enduro —
each with its own finish and LCD colour.

- **Twin LCDs** — `TOTAL km` + live `SPEED km/h` (big), with `NEXT` / `HDG` /
  next-`CAP` below and a blinking `REC` light while recording.
- **The roadbook screen** — scrolling FIA/Dakar **cases**: bold total + partial
  distances, the tulip with the route highlighted in cyan + an official
  arrowhead, the turn abbreviation (R / L / kpS…), CAP, danger marks, and signs.
- **Function buttons + brand chrome.**

**F4 → Rally computer (model / view)** sets **Model**, **LCD colour** (default /
amber / green / red / blue / white), **View**, **Size**, **Opacity** and the
frame on/off. Per-module layout is also under **F4 → Modules**.

### Views
- **Chase** — full size at the right edge (chase-cam).
- **Bottom-centre** — centred, anchored to the bottom edge.
- **Onboard** — smaller, low-centre near the bars (onboard view).

> **Bike-mounted HUD note:** the MX Bikes plugin API only draws a **2-D screen
> overlay** — it can't paint onto the 3-D bike. *Onboard* is the achievable
> approximation; a true mounted screen needs a custom bike-model mod.

## Training, scoring & replay

- **Challenge mode** hides the route and scores your run. **F4 → Ride stats**
  shows an **overall score /100 + grade (A–F)** from on-route % and off-route hits.
- **Training toggles** (F4): **masked** (only the current box shown), **no-CAP**
  (navigate by tulip + distance), **distance-only** (hide tulips, train the trip).
- **Run replay** (F4 → *Run replay*) — a top-down **map** of your line vs the
  route (green = on-route, red = off) with boxes marked, plus a **box timeline**
  (hit / missed). F4 closes it.
- **Warning tones** (F4 → *Audio*): chime on box approach, escalated tone for a
  danger box, alert when off-route.

## Signs

The **38 roadbook signs are the authentic FIA/Dakar lexicon** (terrain, track,
danger, scenery, directions). They're a library you place per box, like a real
roadbook author: **F4 → Edit / review book → pick a Box → toggle signs → Save**.
Auto-placed on generation: **danger** `!`/`!!`/`!!!` on sharp corners, **bump /
dip** from the elevation profile, **water** where you slow through a dip, and a
faint **junction crossing** stub where your route crosses itself.

## Sharing

- **Export** the current roadbook: F4 → Roadbooks → *Export current → Roadbook\shared*.
- **Import**: drop a `.json` in `Roadbook\import\`, then F4 → Roadbooks → import.
- **Real routes**: `tools/gpx_to_roadbook.py <file.gpx>` converts a real-world
  GPX track into a roadbook you can study in-game.
- **Examples**: ready-made roadbooks ship in `example-roadbooks\` — copy them to
  `Roadbook\import\` and import.

## Styles

**Paper** (authentic white), **Dakar** (amber), **Carbon** (neon), **Blueprint**
— F4 → *Style*.

## Controls (hotkeys)

| Key | Action |
|-----|--------|
| **F4**  | Open/close the menu |
| **↑ ↓ · ← → · Enter** | In the menu: move selection · change the setting · activate |
| **F8**  | Toggle the whole overlay |
| **F10** | Toggle the minimap |
| **F12** | Toggle the off-route alert |
| **F5**  | Reload `roadbook_<track>.json` |
| **F6 / F7** | Review: scroll boxes back / forward |
| **F9**  | Back to auto-follow + reset trip & run |

**ESC / pause never regenerates** the roadbook — it won't overwrite the book
you're navigating. Save explicitly with **F4 → Save roadbook now**.

## Menu (F4)

Style · Rally computer frame · View (Chase/Onboard) · Settings (scale, opacity,
generator detail, off-route tolerance, advance look-ahead) · Modules
(toggle/size/position/opacity each) · Roadbooks (load / import / generate from a
ride) · Ride stats · Edit / review book · Save roadbook now · Restart recording ·
Challenge mode (hides the route, scores % on-line).

## On-screen aids (beyond the computer)

- **Compass tape** (top): live heading + a marker for where the route goes next.
- **NEXT preview** (bottom): the upcoming tulip large + a big distance countdown.
- **Off-route** alert + an arrow back to the line.
- **Minimap** (F10): the recorded route + waypoints + your position.

## Build

Visual Studio C++ (this machine: VS18 toolset **v145**; change
`<PlatformToolset>` in `src/plugin/Roadbook.vcxproj` for other VS versions).

```
msbuild Roadbook.sln /t:Roadbook /p:Configuration=Debug /p:Platform=x64
```

Post-build copies `Roadbook.dlo` + `data\` into `plugins\`. Close the game first
— it locks the `.dlo`.

Offline generator test (no game): `build\Debug\RoadbookTests.exe [ride.csv] [out.svg]`.

Asset tooling: `tools/iconpack/` (Node `@resvg/resvg-js` for SVG→PNG, Python
Pillow for the TGA sprites + the device chrome in `make_device.py`).

## Layout

```
src/plugin/
  dllmain.cpp            exported PiBoSo API + nav state + Draw
  api/                   mxb_api.h (PiBoSo contract) + size validation
  record/                ride recorder
  generate/              geometry + tulip generator (offline-tested)
  model/                 roadbook structs + settings + JSON
  render/                draw cache, primitives, roadbook + device renderer
  input/                 GetAsyncKeyState hotkeys
data/
  fonts/                 Roboto Mono (Apache-2.0)
  icons/                 38 lexicon signs + arrow + device chrome (see NOTICE.md)
tools/iconpack/          asset generators
```

## Credits & licence

See [data/NOTICE.md](data/NOTICE.md). The roadbook **signs** and tulip **arrow**
are rasterized from the open-source **tulip** roadbook editor by *drid*
(https://gitlab.com/drid/tulip), **GPL v2+** — a bundled build carries that
obligation. A few glyphs come from the free **Rally Symbols** font (dafont). The
**device chrome** is original; fonts are Roboto Mono (Apache-2.0); JSON is
nlohmann (MIT).

## Status

**Stage 1 complete** + rally-computer presentation, authentic lexicon signs, and
onboard preset. Next: Stage 2 — richer hide-the-route challenge, auto-placed
terrain signs from elevation, and long point-to-point enduro support.
