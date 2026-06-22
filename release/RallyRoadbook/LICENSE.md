# Licence

## This distributed package: GPL v2.0-or-later

This release **bundles roadbook sign artwork and the tulip arrowhead** rasterized
from the open-source **tulip** roadbook editor by *drid*
(<https://gitlab.com/drid/tulip>), which is licensed **GNU GPL v2.0-or-later**.

Because those GPL assets are included, **this package as a whole is distributed
under the GNU General Public License, version 2 or later**. A copy of the GPL is
available at <https://www.gnu.org/licenses/gpl-2.0.html>. Source for the plugin is
provided alongside this package to satisfy the GPL's source-availability term.

## Components & their upstream licences

| Component | Source | Licence |
|-----------|--------|---------|
| Roadbook signs + tulip arrow | tulip (drid) | GPL v2.0-or-later |
| Turn tulips (`rb_tulip_*`) | this project | GPL v2.0-or-later |
| Device material textures (carbon / brushed / knurled) | Wikimedia Commons (Acheolg, Twigg, Andrezadnik, McKechnie) | CC BY-SA 2.5 / 3.0 / 4.0 |
| A few abbreviation glyphs | "Rally Symbols" font (dafont, free) | free for use |
| Fonts (Roboto Mono) | Google Fonts | Apache-2.0 |
| JSON (nlohmann/json) | nlohmann | MIT |
| Plugin source, device chrome, web/CLI companions | this project | GPL v2.0-or-later |

> The device skins (`rb_device*.tga`) bake in **CC BY-SA** photo textures, so
> those skins are share-alike (see `Roadbook_data/NOTICE.md` for attribution).

## Note on the plugin's own code

The original source in this project (the C++ plugin, the device-chrome generator,
and the `web/` + `tools/` companions) is offered under **GPL v2.0-or-later** to
match the bundled assets. If you build a distribution that does **not** include the
GPL sign artwork (e.g. code-only, or with your own non-GPL signs), the original
code may be relicensed by the author under other terms.

See `Roadbook_data/NOTICE.md` for the full attribution list.
