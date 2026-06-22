// Download authentic FIA/Dakar roadbook glyphs from the open-source tulip
// project (gitlab.com/drid/tulip, GPLv2+) and rasterize them to PNG.
// Whitening + TGA conversion happens in make_tulip_tga.py.
import { Resvg } from '@resvg/resvg-js';
import { writeFileSync, mkdirSync, rmSync } from 'fs';

const BASE = 'https://gitlab.com/drid/tulip/-/raw/master/src/svg/glyphs/';
// code -> svg file name (without extension). ~90 authentic FIA/Dakar glyphs.
// stop/info/north come from the rally font in the python step.
const MAP = {
  // directions
  keepL: 'abbr-kpL', keepR: 'abbr-kpR', keepS: 'abbr-kpS',
  left: 'abbr-L', right: 'abbr-R', onL: 'abbr-onL', onR: 'abbr-onR',
  hairpin: 'abbr-HP', roundaboutL: 'roundabout-left', roundaboutR: 'roundabout-right',
  keepMain: 'keep-straight-on-main-track',
  // danger / hazards
  caution: 'danger-1', danger2: 'danger-2', danger3: 'danger-3',
  cutL: 'cut-danger-left', cutR: 'cut-danger-right',
  crestL: 'left-over-crest', crestR: 'right-over-crest',
  downhill: 'downhill', inclineL: 'lateral-inclination-left', inclineR: 'lateral-inclination-right',
  slowdown: 'slow-down', deadend: 'dead-end', lessVisible: 'less-visible', noentry: 'off-track-forbidden',
  // terrain / surface
  bump: 'bump', dip: 'dip', ditch: 'ditch', compression: 'compression', hole: 'hole', bumpy: 'bumpy',
  ruts: 'ruts', rough: 'rough', dune: 'dune', dunes: 'dunes', sandpit: 'sand-pit', camelgrass: 'camel-grass',
  rocky: 'rocky', rocks: 'rocks', chott: 'plain-chott', mountain: 'mountain', hill: 'hill1',
  cliff: 'cliff', collapse: 'collapse', narrow: 'narrow',
  // water
  water: 'ford', river: 'river-water', lake: 'lake', pond: 'pond', dryriver: 'dry-river',
  wadi: 'small-wadi', bigwadi: 'big-wadi', canal: 'aqueduct-canal',
  // man-made
  fence: 'fence', gate: 'fence-gate', bridge: 'above-bridge', pole: 'electric-pole', antenna: 'antenna',
  pipeline: 'pipeline', railroad: 'railroad', wall: 'low-wall', barbwire: 'barbed-wire-fence',
  cattleguard: 'cattle-guard-1', gatebar: 'gate-barrier', hvline: 'high-voltage-line', pump: 'pumpjack',
  mine: 'mine', cairn: 'cairn', wellpost: 'post', roadworks: 'roadworks',
  // scenery
  tree: 'palm-tree', bush: 'bush', village: 'buildings', house: 'house', church: 'church',
  cemetery: 'cemetery', monument: 'monument', ruins: 'ruins', castle: 'fort-castle_1', camp: 'camp', animals: 'animals',
  // service / control
  fuel: 'petrol-station', fuelzone: 'fuel-zone', checkpoint: 'control-checkpoint', mediapt: 'media',
  photo: 'photo', spectators: 'spectators', police: 'police', medical: 'medical-vehicle-point', helicopter: 'helicopter',
};

rmSync('png_tulip', { recursive: true, force: true });
mkdirSync('png_tulip', { recursive: true });
const ok = [], fail = [];
for (const [code, name] of Object.entries(MAP)) {
  try {
    const res = await fetch(BASE + name + '.svg');
    if (!res.ok) { fail.push(`${code}:${res.status}`); continue; }
    const svg = await res.text();
    const resvg = new Resvg(svg, { fitTo: { mode: 'width', value: 240 }, background: 'rgba(0,0,0,0)' });
    writeFileSync(`png_tulip/${code}.png`, resvg.render().asPng());
    ok.push(code);
  } catch (e) { fail.push(`${code}:${String(e).slice(0, 40)}`); }
}
console.log('OK', ok.length, ok.join(','));
if (fail.length) console.log('FAIL', fail.join(','));
