// Fetch rally-relevant icons from game-icons.net (CC BY 3.0), recolor white,
// rasterize to 64x64 PNG. Verify PNGs, then convert to TGA separately.
import { Resvg } from '@resvg/resvg-js';
import { writeFileSync, mkdirSync } from 'fs';

const OUT = 'D:/BikesRoadbook/tools/iconpack/png';
mkdirSync(OUT, { recursive: true });

// concept -> ordered keyword candidates (exact basename preferred)
const wants = {
  caution: ['hazard-sign', 'danger-sign'],
  crest:   ['mountains', 'mountain-road', 'hills'],
  dip:     ['valley', 'mountain-pass'],
  bump:    ['hills', 'molehill', 'speed-bump'],
  hole:    ['hole', 'pit', 'spiked-pit'],
  rocks:   ['stone-pile', 'rock', 'boulder', 'rock-golem'],
  ruts:    ['tire-tracks', 'skid-mark', 'tread'],
  water:   ['water-drop', 'droplet', 'water-splash', 'waves'],
  jump:    ['high-jump', 'jump-across', 'leapfrog', 'jump'],
  narrow:  ['constriction', 'bottleneck', 'road'],
  fork:    ['split-arrows', 'tee-pipe', 'cross-road'],
  gate:    ['gate', 'portcullis', 'wooden-door', 'wooden-fence'],
  arrow:   ['plain-arrow', 'fast-forward-button', 'back-forth', 'pointing'],
};

const treeUrl = 'https://api.github.com/repos/game-icons/icons/git/trees/master?recursive=1';
const tree = await (await fetch(treeUrl, { headers: { 'User-Agent': 'cc' } })).json();
const svgs = tree.tree.filter(t => t.path.endsWith('.svg'));
const base = p => p.slice(p.lastIndexOf('/') + 1, -4);

function resolve(keywords) {
  for (const kw of keywords) {
    let hit = svgs.find(t => base(t.path) === kw);
    if (!hit) hit = svgs.find(t => base(t.path).includes(kw));
    if (hit) return hit.path;
  }
  return null;
}

function whiten(svg) {
  // strip a full-canvas background path, then force white fills
  svg = svg.replace(/<path[^>]*d="M0 0h512v512H0(?:z|V0)?"[^>]*\/>/gi, '');
  svg = svg.replace(/fill="#000"/gi, 'fill="#fff"');
  svg = svg.replace(/<svg([^>]*)>/i, '<svg$1><style>path,circle,rect,polygon,ellipse,g{fill:#ffffff;stroke:none}</style>');
  return svg;
}

const manifest = {};
for (const [code, kws] of Object.entries(wants)) {
  const path = resolve(kws);
  if (!path) { console.log(`MISS ${code}`); continue; }
  let svg = await (await fetch(`https://raw.githubusercontent.com/game-icons/icons/master/${path}`, { headers: { 'User-Agent': 'cc' } })).text();
  svg = whiten(svg);
  const r = new Resvg(svg, { fitTo: { mode: 'width', value: 64 }, background: 'rgba(0,0,0,0)' });
  const png = r.render().asPng();
  writeFileSync(`${OUT}/${code}.png`, png);
  const author = path.slice(0, path.indexOf('/'));
  manifest[code] = { path, author, name: base(path) };
  console.log(`OK   ${code} <- ${path}`);
}
writeFileSync('D:/BikesRoadbook/tools/iconpack/manifest.json', JSON.stringify(manifest, null, 2));
console.log('done', Object.keys(manifest).length, 'icons');
