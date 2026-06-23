# Rally Roadbook — the 20× Roadmap

> From a single MX Bikes plugin to **the definitive cross-sim rally-raid
> navigation platform** — authentic roadbook navigation for every sim, every
> rider, every screen: real-world content, a mobile roadbook device, a cloud
> community, an AI co-driver, and organized online rallies.

---

## North Star

**Make roadbook navigation a first-class sim discipline.** Today rally-raid
navigation (tulips, CAP, ICO, hidden waypoints) exists nowhere in sim racing.
We make it exist *everywhere* — and become the place people go to ride it,
build it, learn it, and compete at it.

---

## H0 — where we are today (the baseline)

A remarkably complete solo + AI effort, already spanning four layers:

- **In-game plugin** (`.dlo`): roadbook reader + ICO trip meter, live map-match,
  compass/CAP, off-route alert, minimap, elevation profile, training modes,
  challenge scoring, **23 photo-real rally-computer skins**, audio + spoken
  co-driver, auto-roadbook from a recorded ride, multi-ride junction union.
- **Web ecosystem**: roadbook viewer + library, **3D real-terrain viewer for ~26
  real MXB tracks** (decoded `.trh` heightmaps + aerials), route builder, device
  gallery, sharing hub — live on GitHub Pages.
- **Tools**: tulip/sign/device asset pipelines, GPX import, procedural scenarios,
  printable SVG/PDF sheets, a CLI, an offline test rig.
- **Content**: real rally-flavoured stages + a GPX rally library.

~62 shipped epics. One game. One machine. Offline. **This is 1×.**

---

## The 20× thesis

20× is **not** 20× more of the same — it's 20× the *surface area*:

| Dimension | Today (1×) | 20× |
|---|---|---|
| Sims supported | 1 — MX Bikes | 6+ — AC, rFactor 2, BeamNG, EA WRC, RaceRoom, AMS2 |
| Playable terrain | ~26 real MXB tracks | **thousands** — auto-extract + real-world import |
| Roadbook source | your rides + GPX | + AI-from-satellite, a community marketplace, real rallies |
| Co-driver | baked voice clips | **LLM AI co-driver** + your-voice recognition |
| Where you read it | in-game overlay | + **phone / tablet / watch** second-screen device |
| Community | static site + files | cloud platform: accounts, marketplace, live events |
| Competition | personal best + ghost | organized **online championships** + broadcast kit |
| Navigation depth | reader + trip + match | full rally-raid: hidden WPs, CAP discipline, penalties, multi-day |
| Authoring | CLI + in-game gen | full editors (in-game / web / mobile), procedural, AI |
| Learning | a tutorial doc | interactive **academy** + certification |

---

## Pillars (the 8 charters)

1. **Navigation Engine** — the most authentic rally-raid navigation in any sim.
2. **Content & Terrain** — infinite stages: community tracks, real-world import, procedural, real rallies.
3. **Cloud & Community** — accounts, library, marketplace, live events, social.
4. **Mobile & Second-Screen** — your phone/tablet *is* the rally computer.
5. **AI & Procedural** — generate, coach, co-drive, and tune with AI.
6. **Multi-Sim & Extensibility** — a sim-agnostic core + adapters + open format.
7. **Competition & Esports** — organized online rally-raid with broadcast.
8. **Training & Academy** — teach the world to read a roadbook.

---

## Horizons

### H1 — DEEPEN (make the MXB experience world-class) · ~3–4×
*Theme: turn the reader into a real rally-raid simulator.*
- **Nav Engine**: rally-raid **stage structure** (liaison + special, start/finish
  controls, time cards); **hidden/eclipse waypoints** (WPM masked, WPE, WPS, WPC)
  that unlock the GPS heading only within radius — the actual navigation
  challenge; **CAP heading-hold** that locks/unlocks per waypoint; **speed/control
  zones (DZ/FZ)** with enforced limits; a **penalty + official-results** system;
  **fog-of-war** mode (no route line, pure roadbook + ICO).
- **Authoring**: full **in-game WYSIWYG roadbook editor**; robust auto-roadbook
  with branch detection (fork/crossroads → the fork tulips already in the catalog).
- **Content**: auto-extract **3D terrain for *every* community MXB track** (batch
  the `.trh`/aerial pipeline); surface/biome classification.
- **Polish**: device material realism v2; genuinely distinct modern-tablet vs
  holder *shapes*; VR + physical-device-on-handlebars support.

### H2 — CONNECT (platform + mobile + content at scale) · ~8–10×
*Theme: stop shipping files; start running a service.*
- **Cloud**: accounts + cloud sync; an **online roadbook library** (search, tags,
  ratings, comments, versioning); a public **API**.
- **Mobile**: a companion **app that IS the roadbook device** — scrolls with your
  in-game position over LAN telemetry; a tablet "rally computer" on your rig;
  offline reader; a watch app (CAP + distance on your wrist).
- **Content**: **real-world location → stage** (any lat/long + radius → DEM +
  satellite → playable); GPX/KML import at scale; a **procedural rally-raid
  generator** (infinite, tunable stages).
- **Community**: ghost + replay sharing, leaderboards, teams, profiles.

### H3 — EXPAND (multi-sim + AI + marketplace + live rallies) · ~15×
*Theme: leave the single game behind.*
- **Multi-Sim**: extract a **sim-agnostic roadbook core**; ship adapters for
  **Assetto Corsa, rFactor 2, BeamNG** (then EA WRC, RaceRoom, AMS2); publish an
  **open roadbook format** + SDK.
- **AI**: **AI roadbook generation from satellite imagery** (CV → tulips + signs);
  an **LLM AI co-driver** (dynamic, context-aware); an **AI navigation coach**
  that analyzes your runs and drills weaknesses; voice recognition for calling
  your own notes.
- **Cloud**: a **roadbook marketplace** (free + premium + creator payouts);
  **live event hosting** (rally calendar, entry lists, scrutineering, results).

### H4 — MOONSHOT (the platform) · ~20×+
*Theme: the category-defining rally-raid ecosystem.*
- **Esports**: organized **online rally-raid championships** (divisions, points,
  a world tour); live timing + **broadcast/stream kit** (spectator nav tracking,
  picture-in-picture roadbook); officiating + anti-cheat.
- **Academy**: a structured **navigation curriculum** + interactive drills +
  **certification tiers**; a "real-rally prep" mode used by *actual* rally-raid
  competitors to train.
- **Reach**: real-rally **content partnerships** (licensed stages); **hardware**
  (DIY trip-meter / roadbook-holder reading the telemetry); a standalone
  engine build (Unity/Unreal) beyond plugins.

---

## The 20× feature catalog (the backlog, by pillar)

**1 · Navigation Engine** — stage structure (liaison/special, time controls) ·
hidden eclipse waypoints (WPM/WPE/WPS/WPC) · CAP heading-hold discipline ·
speed/control zones + penalties · official results + road-position · fog-of-war
nav · multi-day career (bivouac, standings, fatigue) · fuel + mechanical
strategy · dynamic weather / time-of-day · sandstorm low-visibility events ·
break-trail vs follow-tracks · spectator/director nav view · pace-note ⨉ tulip
hybrid · VR cockpit + handlebar device.

**2 · Content & Terrain** — batch 3D extraction for all community tracks ·
real-world location → stage (DEM + satellite) · GPX/KML import at scale ·
procedural rally-raid generator · surface/biome classification from imagery ·
heightmap + surface-mask authoring · stage navigability QA · seasonal/event
packs · digitized real roadbooks (legal/community).

**3 · Cloud & Community** — accounts + cloud sync · online library (search,
ratings, comments, versions) · marketplace + creator payouts · live event hosting
(calendar, entries, scrutineering, results) · leaderboards (stage/rally/global) ·
ghost + replay sharing + co-watch · teams, following, social feed · moderation +
anti-cheat · public API + webhooks · creator analytics.

**4 · Mobile & Second-Screen** — companion app as a live roadbook device (LAN
telemetry scroll) · tablet rally-computer mirror · offline reader + library · AR
roadbook overlay · phone-as-ICO trip meter · watch app (CAP + distance) ·
Bluetooth device pairing.

**5 · AI & Procedural** — AI roadbook from satellite (CV → tulips/signs) · LLM AI
co-driver · AI navigation coach + drills · adaptive difficulty · AI stage designer
· voice recognition (call/confirm your own notes) · AI scrutineer (validate clean
runs).

**6 · Multi-Sim & Extensibility** — sim-agnostic core · adapters: AC, rF2,
BeamNG, EA WRC, RaceRoom, AMS2, iRacing (API permitting) · open roadbook format +
validators · SDK + plugin templates · cross-sim profile + library · standalone
engine integration.

**7 · Competition & Esports** — online championships (divisions, points, world
tour) · live timing + broadcast overlays · caster stream kit · officiating +
anti-cheat · community-run events platform · sponsorships / prize pools.

**8 · Training & Academy** — navigation curriculum (beginner→expert) ·
interactive drills (read-the-tulip, CAP discipline, distance estimation) ·
certification / licensing tiers · real-rally prep mode · web lessons + tutorials ·
mentor matching.

---

## Architecture evolution

| Layer | Today | Where it goes |
|---|---|---|
| Engine | C++ plugin tied to MXB API | **sim-agnostic core** (nav, tulips, model) + thin per-sim adapters |
| Format | project JSON schema | **open, versioned roadbook spec** + validators + SDK |
| Content | local PIL/Node asset scripts | a **content pipeline** (cloud terrain extraction, CV, procedural) |
| Web | static GitHub Pages | a **cloud app** (accounts, DB, API, realtime, CDN) |
| Devices | baked TGA skins | + **mobile/native second-screen** apps over telemetry |
| Co-driver | baked WAV clips | **streaming LLM + TTS** co-driver service |
| Distribution | a zip | in-sim mod stores + app stores + a content marketplace |

---

## Sequencing & dependencies (what unlocks what)

- The **open roadbook format** (H1→H2) is the keystone — it unblocks multi-sim,
  marketplace, mobile, and AI generation. Lock it early.
- **Cloud accounts/library** (H2) precede marketplace, live events, and social.
- The **sim-agnostic core** (H3) is a refactor of today's plugin internals; do it
  once the MXB experience is feature-complete (end of H1) so requirements are known.
- **AI-from-satellite** (H3) reuses the H1 terrain/aerial pipeline + classification.
- **Mobile second-screen** (H2) needs only a small telemetry-streaming shim in the
  plugin — high impact, low cost; pull it forward.

---

## Risks & how we de-risk

- **Per-sim API limits** (closed sims, no overlay) → target moddable sims first
  (AC, rF2, BeamNG); ship the open format so the community fills gaps.
- **Scope vs solo+AI capacity** → ruthless horizon gating; every horizon ships a
  usable product on its own.
- **Licensing** (real rally content, CC-SA textures, GPL signs) → keep the clean
  attribution discipline already in `NOTICE`/`LICENSE`; prefer CC0 + original art
  for anything commercial.
- **Cloud cost/ops** → start serverless + static-first; monetize via marketplace
  before standing up heavy infra.
- **Anti-cheat / fairness** (esports) → server-validated telemetry + replays from day one of competition.

---

## Success metrics (per horizon)

- **H1**: a full rally-raid stage is playable end-to-end with hidden waypoints +
  penalties; 100% of community MXB tracks have 3D terrain.
- **H2**: N accounts, a searchable cloud library, a phone scrolling a live
  roadbook beside your rig.
- **H3**: roadbook navigation running in **3 sims**; first creator paid; first
  hosted online rally.
- **H4**: a recurring online championship with a broadcast; an academy graduate;
  a real rally-raid rider training on it.

---

## First 10 moves (concrete, near-term)

1. Detect forks/crossroads in the generator → use the `forkL/forkR/cross` tulips already in the catalog.
2. Hidden-waypoint (eclipse) navigation prototype: GPS CAP unlocks only within radius.
3. Speed/control zones + a penalty tally → an official-style results screen.
4. In-game WYSIWYG roadbook editor (place/edit boxes live).
5. Batch the 3D-terrain extractor over the full community track folder.
6. Lock the **open roadbook format v1** (JSON schema + validator + docs).
7. Telemetry-streaming shim + a minimal **mobile roadbook PWA** (scrolls live).
8. Extract the nav/tulip/model **core** into a sim-agnostic library.
9. First non-MXB adapter spike (Assetto Corsa or BeamNG).
10. AI co-driver spike: stream box context → an LLM → TTS pace notes.

---

## Build log — waves shipped

- **Wave 1 · Mobile roadbook PWA** *(H2 · Mobile)* — installable, offline
  `rally.html`: a touch roadbook reader with the imported tulips + signs, a
  library picker and a service worker (offline app shell + runtime cache). Live
  as **📱 Mobile** in the site nav. *Next:* live position scroll via a plugin
  telemetry shim.
- **Wave 2 · Academy “Read the Tulip” drill** *(H4 · Academy)* — `academy.html`:
  an interactive quiz to learn the roadbook tulips at a glance — two modes
  (name-the-tulip / spot-the-tulip), live score + streak + personal best. Live as
  **Academy** in the nav.
- **Wave 3 · Open roadbook format v1** *(H2 keystone · Multi-Sim)* — published a
  draft-07 **JSON Schema** (`web/roadbook.schema.json`) + `format.html`: a field
  reference and an **interactive validator** (mirrors `rbtool validate`) so any
  tool, sim or app can read/write the same roadbook. Live as **Format** in the nav.
- **Wave 4 · Eclipse-navigation demo** *(H1 · Nav Engine)* — `navigate.html`: an
  interactive canvas demo of the real rally-raid challenge — **no route line**,
  steer by the roadbook + trip meter, with the CAP bearing arrow unlocking only
  within range (ERTF/Unik-style). Procedural stage + derived roadbook + on-route
  scoring. Live as **Navigate**.
- **Wave 5 · Landing hub** *(UX)* — `home.html`: a polished front door that groups
  every tool into **Explore / Learn & Play / Create / Devices & Reference** cards
  (hero + live stats), and a **⌂ Home** link added across the nav — taming the
  growing nav sprawl as the site passes 10 pages.

---

*Living document. Horizons gate on shipping, not dates: every horizon is a
usable product, and each one is ~2–3× the last.*
