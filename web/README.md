# Rally Roadbook — web companion

The browser side of the [MX Bikes Rally Roadbook](../README.md) project: a static,
dependency-light site deployed to **GitHub Pages** at
**https://drzapp1.github.io/rally-roadbook/**. It lets anyone explore, learn,
build, and embed rally roadbooks without installing the plugin — and shares one
**open roadbook format** with the in-game plugin.

## Pages

| Area | Page | Purpose |
|---|---|---|
| **Explore** | `index.html` | Roadbook viewer + library (loads `roadbooks/manifest.json`). |
| | `track3d.html` | Real MX Bikes terrain in 3D (`.trh` heightmap + aerial). |
| | `map3d.html` | A roadbook's route rendered in 3D. |
| | `devices.html` | Gallery of the 23 rally-computer skins. |
| **Learn** | `home.html` | Landing hub — greets returning visitors with their live Academy + Daily progress; carries the JSON-LD. |
| | `guide.html` | Roadbook 101 — how to read tulips, CAP, distances. |
| | `course.html` | **Academy** — a sequenced 6-lesson course (+ a bonus lesson) → final exam → downloadable PNG certificate. |
| | `academy.html` · `compass.html` · `navigate.html` | Quick drills (tulip, CAP, the *eclipse* navigate challenge). |
| | `progress.html` | Local progress / best scores (localStorage). |
| **Play** | `daily.html` | A date-seeded daily stage (same for everyone each UTC day) + streak, stats and a shareable grid. |
| | `stage.html` | Practice a real library roadbook box-by-box — its actual recorded tulip geometry, distances, CAP and signs. |
| | `codriver.html` | Spoken **co-driver** — reads any stage aloud as natural pace notes (Web Speech API), with cross-box chaining + voice pick. |
| | `marathon.html` | A multi-stage **rally-raid marathon** — read each stage's roadbook and chase the overall GC across a simulated field. |
| **Create** | `builder.html` · `editor.html` | Route builder + roadbook editor. |
| | `generate.html` | Generate a roadbook from a drawn route. |
| **Reference** | `format.html` | The open format — spec + live validator. |
| | `embed.html` | Render/paste a roadbook + copyable embed snippet. |
| | `roadbook.js` | Dependency-free ES module: `validate` · `tulipCode` · `renderRoadbook` · `injectStyles`. |
| | `rally.html` | Installable PWA mobile reader (`manifest.webmanifest` + `sw.js`). |

`404.html`, `robots.txt`, `sitemap.xml`, `favicon.png`, `og-card.png` round it out.

## Open format

A roadbook is one JSON document — `{ schemaVersion, trackName, meta, boxes[], route[] }`
— validated by [`roadbook.schema.json`](roadbook.schema.json) and described on
[format.html](format.html). `roadbook.js` is the reference reader/renderer; the
plugin, the CLI tools, and this site all read the same shape.

## Generating stages from terrain

Most library stages are **auto-generated from terrain** — no recorded ride needed. They run
through the *same* C++ generator the plugin uses (`build/.../RoadbookTests.exe`, which turns a
ride trace into a roadbook), so generated stages match the in-game output exactly:

- **`tools/make_terrain_stage.py <track_id>`** drives a **terrain-following** rally line — a
  least-cost path biased toward valleys and gentle slopes — across a real MX Bikes heightmap
  (`web/tracks/<id>/height.bin`), samples the elevation along it, and tags each box with the
  signs the relief implies (crest / jump / dip / compression / descent / climb). Both
  *curvature → tulips* and *elevation → signs* come from the real landscape.
- **`tools/make_procedural_stage.py <name> [seed] [dunes|mountains|mixed]`** synthesizes a
  brand-new world (multi-octave noise heightmap + a hill-shaded aerial) as a 3D-viewable track,
  then drives a stage across it — endless fresh rally-raid.

The library (`roadbooks/manifest.json`) is **64 roadbooks**: 26 real-terrain stages, 10
procedural worlds, and 28 hand-built / GPX / drill stages — grouped in the viewer as *Terrain
stages*, *Procedural stages*, and so on.

## Build & deploy pipeline

Push to `main` touching `web/**` (or the workflow) triggers
[`.github/workflows/pages.yml`](../.github/workflows/pages.yml), which runs, in order:

1. `node tools/make_manifest.mjs` — rebuilds `roadbooks/manifest.json` from the actual `.json` files.
2. `node tools/make_sitemap.mjs` — rebuilds `sitemap.xml` from the actual `*.html` set (so it can't go stale).
3. `python tools/check_web_links.py` — **gate:** every internal `href`/`src`/import + manifest entry must resolve.
4. `python tools/check_a11y.py` — **gate:** every page needs `<html lang>`, `<title>`, viewport, a meta description, `alt` on every `<img>`, and any `ld+json` block must parse.
5. Upload `web/` as the Pages artifact and deploy.

A broken link, a missing description, or malformed JSON-LD **fails the deploy** —
nothing broken reaches production. Run both gates locally before pushing:

```sh
python tools/check_web_links.py && python tools/check_a11y.py
```

## Adding a page

1. Create `web/<name>.html` with the standard head: `<html lang="en">`, a `<title>`,
   a viewport meta, a `<meta name="description">`, a `<link rel="canonical">`, and the
   favicon link. Give every `<img>` an `alt`. (Copy an existing page's head.)
2. Add it to the relevant `nav` and, if it's a top-level destination, a card on `home.html`.
3. Run the two gates above. The sitemap picks it up automatically on the next deploy.

> Deploys are made from a throwaway `git worktree` detached on `origin/main`, so the
> live site stays independent of any in-progress feature branch.
