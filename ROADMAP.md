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
- **Wave 6 · CAP-heading drill** *(H4 · Academy)* — `compass.html`: an interactive
  SVG-compass drill to read CAP headings fast (0°=N, clockwise) — two modes
  (read-the-needle / pick-the-bearing), score + streak + best. Carded under
  **Learn** on the hub.
- **Wave 7 · Roadbook 101 guide** *(H4 · Academy)* — `guide.html`: an illustrated
  primer on reading a roadbook (box anatomy, the 14 tulips, CAP, ICO, pictograms +
  danger, the 3-step ride loop), wired straight to the drills. Completes the
  concepts → drills → challenge learn path. Carded under **Learn**.
- **Wave 8 · Navigate polish** *(H1 · Nav Engine)* — added a **breadcrumb trail**,
  a per-difficulty **best-time** (localStorage), and an **Easy / Medium / Hard**
  selector (waypoint count + unlock range + speed) to the eclipse demo.
- **Wave 9 · Stage generator** *(H2 · Content)* — `generate.html`: procedurally
  spin up endless **schema-valid** practice roadbooks (turns, distances, CAP,
  signs, danger), rendered as a roadbook sheet, with one-click JSON download.
  Carded under **Create**.
- **Wave 10 · Progress dashboard** *(H4 · Academy)* — `progress.html`: your personal
  bests across the drills (tulip + CAP streaks, Navigate times per difficulty) as
  stat cards + 8 earnable **badges**, all from localStorage. Completes the gamified
  learn loop. Carded under **Learn**.
- **Wave 11 · Social share-cards** *(Reach)* — a 1200×630 `og-card.png` + Open
  Graph / Twitter meta on **every page**, so the site previews professionally
  (title, tagline, tulip strip) when shared to Discord / X / forums.
- **Wave 12 · Browser & SEO polish** *(Reach)* — a crisp **favicon** + apple-touch
  icon on every page, a **sitemap.xml**, **robots.txt**, and a branded **404** page
  ("Off the route"). Rounds out the site's discoverability + presentation.
- **Wave 13 · Reference module + embed** *(H3 keystone · Multi-Sim)* — `roadbook.js`:
  a dependency-free ES module that validates + renders the open format (the same
  renderer the site uses). `embed.html`: pick/paste a roadbook → live render + a
  **copyable embed snippet** so any site can drop in a roadbook via the hosted
  module + art. The format's real interop step.
- **Wave 14 · README front page** *(Reach)* — a **🌐 Live web companion** section in
  the repo README pointing visitors to the deployed site + the tool map + the open
  format, so the GitHub front page leads to the live experience.
- **Wave 16 · CI link-gate** *(Quality)* — wired `check_web_links.py` into the Pages
  deploy workflow (after manifest generation, before upload), so a broken internal
  link/asset now **fails the deploy** instead of shipping. QA is permanent.
- **Wave 17 · Accessibility** *(Quality)* — a keyboard `:focus-visible` ring and a
  `prefers-reduced-motion` guard on every page, plus `check_a11y.py` (lang + title +
  viewport + img-alt) wired into the deploy gate beside the link check.
- **Wave 18 · SEO & perf completeness** *(Reach)* — a `rel="canonical"` on every
  page, meta descriptions filled in where missing, and `loading="lazy"`/`decoding="async"`
  on remaining images. The page-basics gate now also requires a meta description.
- **Wave 19 · Auto sitemap** *(Reach)* — `make_sitemap.mjs` regenerates `sitemap.xml`
  from the actual `web/*.html` set and is wired into the Pages workflow, so the
  sitemap can never go stale again (it was missing `embed.html`).
- **Wave 20 · Structured data** *(Reach)* — `WebSite` + `SoftwareApplication`
  JSON-LD on the hub so search engines get rich, machine-readable metadata about
  the plugin (free, Windows, help link, author). The gate now also validates that
  every `ld+json` block parses.
- **Wave 21 · Consolidation & handoff** *(Docs)* — `web/README.md`
  documenting the site map, the open format, the CI pipeline (manifest → sitemap
  → link + a11y/JSON-LD gates), and how to add a page. Closes the 21-wave web
  build-out; the big roadmap pillars (multi-sim, AI, cloud) are next and need a
  focused session.
- **Wave 22 · Academy pillar: course + Lesson 1** *(Academy)* — `course.html`:
  a structured, progress-tracked curriculum (6 lessons, each locked until the prior
  passes). Lesson 1 *Reading tulips* is fully playable — teach → endless practice →
  a 5-question mastery check that unlocks the next. Lessons 2–6 scaffolded; the loop
  now builds the ROADMAP Academy pillar one lesson per wave.
- **Wave 23 · Academy Lesson 2 + lesson engine** *(Academy)* — generalized the
  course to a type-driven question factory (`genQ`), then authored Lesson 2 *Distances
  & the ICO*: teach total-vs-partial, then distance-arithmetic practice + check
  (next TOTAL = this TOTAL + PARTIAL). Factory unit-tested over 8000 questions
  (always 4 unique options incl. the answer).
- **Wave 24 · Academy Lesson 3 (CAP & compass)** *(Academy)* — a `cap` widget
  with an inline compass-rose SVG: name the cardinal for a bearing, and turn
  left/right onto a CAP (always the shorter way). Answers verified against an
  independent shortest-turn calc over 8000 questions. 3 of 6 lessons live.
- **Wave 25 · Academy Lesson 4 (Danger & signs)** *(Academy)* — a `signs` widget
  over a curated 26-sign pool (danger ! !! !!!, terrain, hazards, water, services)
  using the FIA art; identify each sign’s meaning. All 26 codes verified present;
  options logic-tested. 4 of 6 lessons live.
- **Wave 26 · Academy Lesson 5 (Putting it together)** *(Academy)* — a `follow`
  widget that renders a full roadbook box (TOTAL/PARTIAL + tulip + CAP + sign) and
  quizzes one aspect at random, synthesizing lessons 1–4. Logic-tested over 12000
  questions. 5 of 6 lessons live.
- **Wave 27 · Academy Lesson 6 (Blind nav) + course complete** *(Academy)* — a
  `blind` widget: the full box shows briefly then hides (setTimeout, reduced-motion-
  safe; options locked until it hides) so you answer from memory. All 6 lessons
  now live → the completion certificate unlocks. The Academy pillar’s first full
  curriculum is shipped.
- **Wave 28 · Academy Final Exam** *(Academy)* — once all 6 lessons pass, a
  12-question mixed exam (a random skill each question) gates the certificate: 10/12
  to earn it. Reuses the five tested widget types; 24000 generated exam questions
  verified valid.
- **Wave 29 · Academy browser verification** *(Quality)* — drove a real browser
  over the live course: all 6 widget types generate valid questions, lesson
  practice/feedback works, the blind-peek auto-hide times correctly (locked → hidden
  → unlocked), and the exam gate generates 12, passes, and unlocks the certificate.
  Zero console errors, no bugs.
- **Wave 30 · Daily Challenge** *(Procedural + Community)* — `daily.html`: a
  date-seeded (UTC) procedural stage everyone gets the same day — read 10 roadbook
  boxes (turn/CAP/sign), score, and share a Wordle-style grid. Deterministic
  generator unit-tested + browser-verified (play → result → share → once-per-day lock).
- **Wave 31 · Daily streak & stats** *(Community)* — the Daily now tracks a
  per-day history and shows played / average / best / current streak plus a 7-day
  bar, on the intro and result. Browser-verified (streak stops correctly at gaps).
- **Wave 32 · Academy certificate download** *(Academy)* — the completion
  certificate now renders to a PNG via canvas (name + date + seal) with Download
  and Copy-share-text buttons. Browser-verified: drawCert produces a valid ~79 KB
  PNG with no errors. Closes the Academy reward loop.
- **Wave 33 · Personalized home hub** *(Community)* — the home page now reads
  local progress and shows a returning visitor their Academy completion and Daily
  streak with contextual CTAs; stays hidden for first-time visitors. Browser-
  verified both states.
- **Wave 34 · Academy bonus lesson: Junctions & forks** *(Academy)* — a 7th
  bonus lesson teaching which branch to take at a fork/crossroads (new ‘fork’
  widget on the fork/cross tulips). Flagged bonus, so it does not gate the
  certificate. Browser-verified: 6 core still unlock the exam; the bonus plays.
- **Wave 35 · Practice a real stage** *(Academy + Content)* — `stage.html`: pick a
  library roadbook and ride its **actual** boxes — the real recorded tulip geometry,
  distances, CAP and signs — calling each turn for a score. Imports `validate` from
  `roadbook.js`. Browser-testing caught that flat GPX stages have no classified
  turns, so `make_manifest.mjs` now emits a per-stage `turns` count and the picker
  only offers stages with ≥4 turns (19 of 28). Verified end to end.
- **Wave 36 · Docs checkpoint** *(Docs)* — refreshed web/README.md with the
  Academy course, Daily challenge, Stage practice and the personalized hub (a new
  ‘Play’ area), and updated the project memory with the platform’s growth, the
  browser-verify workflow, and the python surrogate/truncation gotcha.
- **Wave 37 · Stage-practice best + share + hub** *(Community)* — Stage practice
  now saves a per-stage best, shows it on the result, and offers a shareable
  grid; the home hub gains a "N stages practised" pill. Browser-verified end to end.
- **Wave 38 · Keyboard answering** *(Accessibility)* — the Daily and Stage
  quizzes now accept keys 1–4 to pick an option (Enter for next), with an on-screen
  hint. Browser-verified (key press marks the right option).
- **Wave 39 · Keyboard answering on the course** *(Accessibility)* — the Academy
  lessons + final exam now accept keys 1–4 too, with a handler scoped to the active
  quiz area (exam, else check-over-practice). Keyboard answering now spans all
  three quiz surfaces. Browser-verified (exam + lesson).
- **Wave 40 · Academy bonus 2: Reading the terrain** *(Academy)* — a 2nd bonus
  lesson on anticipating crests/dips/jumps/surface from the signs (new ‘terrain’
  widget over a 12-sign pool). Excluded from the certificate like the forks bonus;
  terrainQ logic-tested over 6000 questions.
- **Wave 41 · Printable cheat-sheet** *(Reference)* — `cheatsheet.html`: a one-page
  reference of the 14 tulips, danger marks, CAP, and ~36 common signs with
  meanings, with a print stylesheet + Print/Save-PDF button. Eager-loaded so it
  prints complete. Browser-verified (51 symbols render).
- **Wave 42 · Onboarding path** *(UX)* — the home hub now greets first-time
  visitors with a "New here? Start with the Guide → Cheat-sheet → Academy" path
  (returning visitors still get their progress strip). Browser-verified both states.
- **Wave 43 · 3D page preload** *(Performance)* — track3d/map3d load Three.js
  from the jsdelivr CDN via an import map; added `preconnect` + `modulepreload` so
  the connection and main-module download start at parse time, not when the module
  graph resolves. (The Wave-18 audit missed this CDN — it lives in importmap JSON.)
- **Wave 44 · Vendor Three.js — self-contained 3D** *(Reliability)* — track3d/map3d
  no longer depend on the jsdelivr CDN: three.module.js + OrbitControls + FlyControls
  are vendored under `web/vendor/three/` and the import maps point at local paths.
  The 3D viewers now work offline and behind CDN-blocking networks. Browser-verified
  (1.2 MB module loads from /vendor, canvas inits, zero jsdelivr requests).
- **Wave 45 · Self-contained gate** *(Reliability)* — audited every page for
  external runtime deps (scripts/styles/fonts/importmaps/module imports): zero remain
  after vendoring Three.js. Added `check_selfcontained.py` as a 3rd CI gate
  (negative-tested) so an external dependency can never slip in again.
- **Wave 47 · PWA offline precache** *(Mobile)* — the installable reader now
  precaches the library manifest + all 14 tulips (cache bumped to rally-v2) on top
  of the shell, so it lists the library and renders roadbook turns offline from
  first install. Browser-verified: SW installs, 19 assets cached, old cache cleaned.
- **Wave 48 · Roadbook-from-terrain generator** *(AI/Procedural)* — new
  `make_terrain_stage.py` drives an open-terrain rally line (straights + corners)
  across a real heightmap, samples elevation along it, and feeds the trace through
  the same C++ generator the plugin uses → a validated roadbook with tulip turns
  + crest/dip/jump/downhill signs from the real elevation. No recorded ride needed.
  Shipped 8 real-terrain stages (26–51 turns each) + a "Terrain stages" library group.
  The shared engine for procedural rally-raid (#2) and the real-track library (#4).
- **Wave 49 · Procedural rally-raid** *(AI/Procedural)* — `make_procedural_stage.py`
  synthesizes terrain (multi-octave value-noise heightmap + a shaded aerial) as a
  brand-new viewable track, then drives a stage across it with the Wave-48 engine →
  a 3D-viewable track + a validated roadbook, fully synthetic. Styles: dunes /
  mountains / mixed. Shipped 3 stages; browser-verified one renders in track3d.
- **Wave 50 · Real-track library** *(Content)* — batch-generated terrain-stage
  roadbooks for all 26 real MXB track heightmaps in `web/tracks/` (18 new this wave),
  each a valid open-terrain rally stage over real elevation. The library is now
  genuinely real tracks (the "Terrain stages" group). Delivers feature #4.
- **Wave 51 · LLM co-driver** *(AI)* — `codriver.html` reads any stage aloud as
  natural spoken pace notes ("in 320, caution, hard right, over crest"). A phrasing
  grammar composes distance + danger + turn severity (from the tulip) + terrain signs
  from each box; the Web Speech API voices them in sequence with a synced tulip/CAP
  display, pace control and play/stop. Delivers feature #3 — completes the four
  autonomous features (roadbook-from-track, procedural, real-track library, co-driver).

---

*Living document. Horizons gate on shipping, not dates: every horizon is a
usable product, and each one is ~2–3× the last.*
