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
| **Learn** | `home.html` | Landing hub + the project's structured-data (JSON-LD). |
| | `guide.html` | Roadbook 101 — how to read tulips, CAP, distances. |
| | `academy.html` · `compass.html` · `navigate.html` | Interactive drills (tulip, CAP, the *eclipse* navigate challenge). |
| | `progress.html` | Local progress / best scores (localStorage). |
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
