# Rally Roadbook — website

The `web/` folder is a self-contained static site:

- **Explore** — browse every roadbook in your library (auto-discovered), open any
  in the viewer, see distance + difficulty, page through the full book, read the
  94-sign legend, switch themes.
- **Build a route** (`builder.html`) — draw a path on the canvas and it generates a
  rally roadbook live: turns classified into stock tulips, distances, CAP headings
  and danger marks computed automatically. Download the `.json` (import in-game via
  `Roadbook\import\`) or open it straight in the viewer.

No build step, no framework, no server code — just HTML/JS/JSON.

## Preview locally

Any static server works (the library uses `fetch`, which `file://` blocks):

```sh
cd web
python -m http.server 8000     # then open http://localhost:8000
```

(`web/launch.bat` does this on Windows.)

## Publish to GitHub Pages (free)

A workflow (`.github/workflows/pages.yml`) deploys `web/` on every push to `main`.

1. Create an empty repo on github.com, e.g. **rally-roadbook** (no README).
2. Push this project:
   ```sh
   git remote add origin https://github.com/<your-user>/rally-roadbook.git
   git push -u origin main
   ```
3. On GitHub: **Settings → Pages → Build and deployment → Source: GitHub Actions**.
4. The workflow runs (regenerates the map manifest, uploads `web/`) and the site goes
   live at **https://&lt;your-user&gt;.github.io/rally-roadbook/** — auto-redeploys on
   every push that touches `web/`.

## Adding more maps

Drop a `roadbook_<name>.json` into `web/roadbooks/`. The deploy regenerates the
library manifest automatically; to refresh it locally run:

```sh
node tools/make_manifest.mjs
```

The map appears in the library under a group inferred from its name prefix
(`rally_…` = Real rally stages, `stage_…` = GPX stages, `drill_…` = Practice drills,
anything else = Tracks).

## Custom domain (later)

Once you buy a domain, add a file `web/CNAME` containing just the domain
(e.g. `roadbooks.example.com`), point a DNS `CNAME` record at
`<your-user>.github.io`, and set the domain under Settings → Pages. The workflow
ships `web/CNAME` with the site, so it sticks across deploys.
