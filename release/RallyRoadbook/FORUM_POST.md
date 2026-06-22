# [PLUGIN] Rally Roadbook — learn roadbook navigation in MX Bikes

**A native plugin that turns any track into a rally-raid roadbook trainer.** Ride a
track once; it records your line, auto-generates a real-style roadbook (tulip
diagrams + distances + compass headings + danger/terrain signs), and renders it
in-game as an **ERTF/F2R-style rally computer** so you can learn to navigate by it
— no GPS line, just tulips and the trip meter, like the real thing.

---

## Highlights

🧭 **Rally computer, 23 models** — Carbon, Classic ICO, Desert/Dakar, Stealth,
Night, Titanium, Gold… each with its own finish + LCD colour, a **power-on boot
animation**, twin LCDs (TOTAL + SPEED), and the scrolling roadbook screen.

🛣️ **Authentic roadbook** — 94-sign FIA/Dakar lexicon (rasterized from the
open-source *tulip* editor), cyan-highlighted tulips with the official arrowhead,
CAP headings, partial/total distances, danger marks.

🎙️ **Spoken co-driver** — full **pace-note sentences** assembled from a voice
bank: *"danger three, hairpin left, two hundred, into crest"*, called ahead of the
corner.

🎯 **Training & scoring** — hide-the-route challenge, masked / no-CAP /
distance-only modes, score /100 + grade, **time-trial with section splits**, a
**ghost** of your previous run, and a **run history**.

🗺️ **Aids** — compass tape, big NEXT preview, off-route alert, minimap, elevation
profile.

🖥️ **Companion apps** — a **web viewer** (browse/print roadbooks), a **roadbook
editor** (drag tulips, place signs, save), and a **CLI** (generate / validate /
GPX import / printable SVG sheets).

---

## Install

1. Download the zip, copy `Roadbook.dlo` + `Roadbook_data\` into
   `…\steamapps\common\MX Bikes\plugins\`.
2. Launch, ride a track, then **F4 → Save roadbook now**.
3. Re-ride — the rally computer auto-advances. Press **F4** for all settings.

Full guide in `README.md`. Hotkeys: **F4** menu · **F8** overlay · **F10** minimap ·
**F9** reset trip.

## Notes

- The plugin API only draws a 2-D overlay, so the computer is a screen overlay
  (with chase / bottom-centre / onboard placements) — not painted onto the 3-D
  bike.
- **Licence:** the bundled signs come from the open-source *tulip* project
  (GPLv2+), so the package is distributed under **GPLv2+**. Full credits in
  `Roadbook_data/NOTICE.md`.

*Feedback + roadbooks welcome — share your generated books and I'll add good ones
to the library.*

<!-- screenshots: device models sheet, an in-game case strip, the web editor,
     a printable roadbook sheet -->
