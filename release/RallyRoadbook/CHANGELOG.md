# Changelog

## v1.0.0 — public release

The first public-ready release. Everything from the staged build, plus a large
feature + companion-app pass.

### Rally computer
- **23 device models** (was 8): Carbon, Tablet, Classic ICO, Stealth, Desert,
  Night, Retro, Enduro, Titanium, Forest, Crimson, Ocean, Sandstone, Mono, Gold,
  Ranger, Ice, Copper, Lime, Navy, RaidNav, Tripmaster, RallyeF2 — each its own finish + default LCD colour.
- **Faithful real-computer replicas** among them: RaidNav + Tripmaster carry the
  knurled **wind-knobs + mount tab** of a roadbook holder; RallyeF2 the modern
  red-trim look.
- **Power-on boot animation** (segment self-test + sweep + splash) on session /
  roadbook load.
- Glass sheen + per-model LCD colour, with a manual LCD colour override.

### Signs
- **Full 94-sign FIA/Dakar lexicon** (was 38), grouped into Direction · Danger ·
  Terrain · Water · Man-made · Scenery · Service.
- **Categorized sign picker** in the in-game editor.
- **Jump detection + sign**: a crest taken sharp + fast enough to go airborne
  (`v²·curvature > g`) is auto-tagged a **jump** — a ramp + ballistic-arc glyph the
  co-driver calls (*"into jump"*); gentle or slow crests stay a "bump" (speed-gated,
  unit-tested both ways).

### Co-driver audio
- **Full spoken pace-note sentences** assembled from a 76-clip voice bank, e.g.
  *"danger three, hairpin left, two hundred, into crest"*, called ~150 m out.
  Toggle between single-word and full-sentence modes.
- The co-driver now also calls **ruts**, **sand** (dunes / sand pits) and **rocky**
  terrain — the bank already had those words. Every clip the composer can speak is
  verified present in the bank (57 distinct clips, 0 missing).

### Navigation & training
- **Speed-scaled look-ahead**: the active box advances and the co-driver calls fire
  earlier at higher speed (consistent ~6 s warning for a spoken pace note), so
  instructions arrive with adequate reaction time at any pace.
- Dash shows **stage progress** (box X/N + a distance-through-stage bar); the
  minimap "you are here" marker is now a **heading arrow** (shows which way you face).
- **Elevation profile** module (route altitude graph + you-are-here + ascent/descent).
- Generator now inserts **crest / dip boxes** for prominent elevation features
  (density-capped so dune fields don't over-fill the book).
- More calibrated auto-signs: **compression** (a dip hit fast vs. slow=water) and
  **downhill** (steep descending turns) — both spam-tested across recorded rides.
- **Ghost / personal-best** overlay in the replay — saved per track and reloaded
  across sessions (your best run, not just the last), with per-box split deltas.
- **Multi-ride junction union** (`rbtool union`): overlays un-taken branch spokes
  at junctions from several rides of the same track.
- **Time-trial + section splits + run history**: every finished run is timed
  (per-box splits in the replay) and saved to `Run history` (best + recent).

### Authenticity
- **French roadbook notation** toggle (DEP / TD / D / G / ARR) alongside the
  international set (S / kpS / R / L / F) — F4 → *Notation*.

### Content
- **Real rally-stage roadbooks** — six recognizable stages (Empty Quarter · Merzouga ·
  Erzbergrodeo · Baja San Felipe · Finke · Hellas/Crete) built through the generator
  and terrain-flavoured, in the web library's *Real rally stages* group. Faithful
  representations for study, not the official copyrighted roadbooks.

### Companion apps
- **Web viewer** (`web/index.html`): library, full-book pager, elevation, a **sharing
  hub** that browses + downloads the bundled roadbooks (real, not a mockup), and a
  **Legend** of all 94 signs with their meanings (learning aid).
- Each box in the web viewer now shows the **co-driver pace-note** it would call
  (e.g. *“danger three, hairpin left, 180, into steep descent”*) — a reading aid that
  mirrors the in-game voice composer (same danger→direction→distance→feature order).
- **Roadbook editor** (`web/editor.html`): name the roadbook, drag tulip points,
  edit fields, place signs, save plugin-compatible JSON.
- **Theme switcher** in the web viewer — Midnight / Paper / Dakar / Blueprint
  (saved per browser); the roadbook cases themselves stay authentic paper.
- **CLI** (`tools/rbtool.py`): gen / gpx / validate / stats / **sheet** (printable
  SVG, `--french`) / **book** (A4-paginated HTML + **stage-overview cover map** +
  **signs-used legend**, print to PDF) / batch / union / **diff**.
  `validate` lints schema, monotonic distance,
  cap range, tulip bounds, coincident boxes, NaN and box density — all 21 bundled
  roadbooks pass clean.
- **Practice scenarios** (`tools/make_scenarios.py`): generated training drills.

### Build / quality
- Distribution now ships the **optimized, self-contained Release build**
  (`/MT` static CRT, ~710 KB, **0 compiler warnings**) — no VC++ redist needed.
  `tools/build_release.ps1` builds it automatically.
- **Asset-completeness gate** (`tools/check_assets.py`) runs in the release build:
  every pictogram (in-game TGA **and** web PNG), every co-driver voice clip, every
  device sprite and both fonts must exist or the build aborts — so a blank sign or
  silently-dropped callout can't ship. Parses the sign list straight from the
  plugin header, so it stays in sync.

### Fixes
- **Stock tulip diagrams** — each box now renders a clean catalogue tulip
  **classified from its turn** (straight · slight · 90° · sharp · hairpin L/R ·
  start · finish) with a crisp arrowhead + chequered finish, instead of tracing
  the **raw recorded path** with a runtime-rotated arrow sprite (which rendered
  erratically). Shared by the in-game renderer, the web viewer and the printable
  book; classification is at draw-time so existing roadbooks pick it up with no
  regeneration.
- **Content gate** in the release build: every bundled roadbook must pass
  `rbtool validate` (schema, monotonic distance, CAP range, coincident boxes, NaN)
  or the build aborts. It caught a duplicate/coincident box in `union_demo`
  (now 72 boxes, clean) — complements the asset-completeness gate.
- Completed the **web sign set** — `stop`, `info` and `north` were missing their
  `web/signs/*.png`, so 3 signs showed as broken images in the Legend + editor
  (all 94 codes now have a matching PNG, derived from the in-game art).
- Audited the in-game **sprite-index plumbing** (pictograms `i+1`, arrow `kCount+1`,
  devices `kCount+2+model`) against the registration order — all consistent.
- Fixed device models 10+ registering a **single-char sprite filename** (`rb_device:`
  instead of `rb_device10`) — the 11th–16th finishes loaded a blank/wrong body
  in-game; all 20 now register their correct `rb_deviceNN.tga`.
- **Difficulty rating** (Easy → Expert from hazard density) in `rbtool stats`, the
  web viewer header, and on each **library card** (distance + difficulty at a glance) —
  the rating also weighs **terrain hazards** (jumps, dunes, water, rocks) so a
  dune/desert stage isn't mislabelled Easy. The web viewer + editor now read the plugin's **meta-nested**
  `trackName`/`totalDistanceKm` (falling back to the last box), so a roadbook
  exported straight from the game shows its name + distance correctly.
- New track with **no roadbook** now shows a slim guidance banner (ride → F4 → Save)
  instead of a blank overlay.
- Generator **merges coincident / too-close boxes** (overlapping turn + elevation
  + crossing waypoints) so the book isn't cluttered with duplicates — verified on
  recorded rides (e.g. a dune stage dropped 16 duplicate boxes).
- ESC / pause never regenerates the active roadbook.
- Junction-crossing detection tightened (no false positives on winding desert).
- Surface roughness via detrended elevation RMS.
