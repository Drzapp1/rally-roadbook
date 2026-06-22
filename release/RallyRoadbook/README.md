# MX Bikes — Rally Roadbook

A native MX Bikes plugin that teaches **rally-raid roadbook navigation**. Ride a
track once; the plugin records your line, auto-generates a real-style roadbook
(tulip diagrams + distances + compass headings), and renders it in-game as an
**ERTF/F2R-style rally computer** so you can learn to navigate by it — tulips,
CAP headings, trip meter, danger signs, the lot.

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

The roadbook is framed as a real **rally computer** with **23 selectable models**
— Carbon, Tablet, Classic ICO, Stealth, Desert/Dakar, Night, Retro CRT, Enduro,
Titanium, Forest, Crimson, Ocean, Sandstone, Mono, Gold, Ranger, Ice, Copper, Lime, Navy, RaidNav, Tripmaster, RallyeF2 — each with its
own finish and LCD colour, and a **power-on boot animation** on session start.

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
- **Spoken co-driver** (F4 → *Voice co-driver*): full **pace-note sentences**
  assembled from a clip bank — e.g. *"danger three, hairpin left, two hundred,
  into crest"* — called ~150 m out (toggle single-word vs full sentences).
- **Ghost / personal best**: the replay overlays your **personal-best run** (blue,
  saved per track and reloaded across sessions) for a side-by-side line + score +
  **per-section split deltas** (green where you're ahead of the ghost, amber behind).
- **Time-trial & run history**: each finished run is timed (with **section
  splits** per box in the replay) and saved to **F4 → Run history** (best + recent).
- **Elevation profile** (F4 → Modules → *Elevation*): a route altitude graph with
  a you-are-here marker + total ascent/descent.

## Signs

The **94 roadbook signs are the authentic FIA/Dakar lexicon**, grouped into
**Direction · Danger · Terrain · Water · Man-made · Scenery · Service**. They're a
library you place per box from a **categorized picker**, like a real roadbook
author: **F4 → Edit / review book → pick a Box → toggle signs → Save**.
Auto-placed on generation: **danger** `!`/`!!`/`!!!` on sharp corners, **bump /
dip** from the elevation profile, **jump** on a crest hit fast enough to go
airborne, **water** where you slow through a dip,
**compression** where you hit one fast, **downhill** on steep descending turns,
**bumpy** on rough sections, and a faint **junction crossing** stub where your
route crosses itself.

**Multi-ride union:** ride a track a few different ways and
`tools/rbtool.py union ride1.csv ride2.csv … out.json` overlays the **un-taken
branches** at each junction as faint spokes — the roads you *didn't* take — like a
real roadbook that shows every fork, not just your line.

## Sharing

- **Export** the current roadbook: F4 → Roadbooks → *Export current → Roadbook\shared*.
- **Import**: drop a `.json` in `Roadbook\import\`, then F4 → Roadbooks → import.
- **Real routes**: `tools/gpx_to_roadbook.py <file.gpx>` converts a real-world
  GPX track into a roadbook you can study in-game.
- **Examples**: ready-made roadbooks ship in `example-roadbooks\` — copy them to
  `Roadbook\import\` and import.

## Companion apps (`web/` + `tools/`)

Beyond the in-game plugin, the project ships desktop/web companions:

- **Web viewer** (`web/index.html`) — browse a roadbook library, page through the
  full book rendered like the in-game cases, a **stage-shape overview** + elevation
  profile, a community **sharing hub**, and a **Legend** of all 94 signs with their
  meanings. Run `web/launch.bat` (or any static server).
- **Roadbook editor** (`web/editor.html`) — create/edit roadbooks: drag tulip
  points, set distances/headings/danger, place signs from the categorized picker,
  save plugin-compatible JSON.
- **CLI** (`tools/rbtool.py`) — `gen` / `gpx` / `validate` / `stats` / **`sheet`**
  (printable SVG) / **`book`** (A4-paginated HTML → print to PDF) / `batch` / `union` / `diff`.
- **Practice scenarios** (`tools/make_scenarios.py`) — generated training drills
  (junctions, hairpins, crest/dip, slalom) under `content/scenarios/`.

## Styles

**Paper** (authentic white), **Dakar** (amber), **Carbon** (neon), **Blueprint**
— F4 → *Style*. **Notation** toggles between international (R / L / kpS) and
authentic **French** (D / G / TD) abbreviations.

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
  icons/                 94 lexicon signs + arrow + 23 device skins (see NOTICE.md)
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

**v1.0** — feature-complete for public release: 16-model rally computer with boot
animation, 94-sign authentic lexicon + categorized picker, full spoken pace-note
co-driver, elevation profile, ghost comparison, time-trial + section splits + run
history, and the web viewer / editor / CLI companions. See [CHANGELOG.md](CHANGELOG.md).
