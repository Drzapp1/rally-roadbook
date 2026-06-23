// Build web/roadbooks/manifest.json — the library reads this to list every map
// (name, group, distance, difficulty) so adding a roadbook needs no code change.
// Run from the repo root:  node tools/make_manifest.mjs
import { readdirSync, readFileSync, writeFileSync } from 'fs';
import { join } from 'path';

const DIR = 'web/roadbooks';
const TH = { jump:1, water:1, cliff:1, dune:.6, dunes:.6, rocks:.5, sand:.3, ditch:.5, hole:.6, wadi:.4, compression:.4, bumpy:.2, narrow:.4, collapse:.6 };

function difficulty(rb){
  const b = rb.boxes || []; const km = b.length ? b[b.length-1].distTotalKm : 0;
  if (km < 0.1) return '';
  let h = 0;
  b.forEach(x => { h += (x.dangerLevel||0) + (x.type==='hairpin'?2:0) + (x.pictograms||[]).reduce((s,p)=>s+(TH[p]||0),0); });
  const s = h / km;
  return s < 2 ? 'Easy' : s < 5 ? 'Moderate' : s < 9 ? 'Hard' : 'Expert';
}
function group(id){
  if (id.startsWith('rally_')) return 'Real rally stages';
  if (id.startsWith('drill_')) return 'Practice drills';
  if (id.startsWith('stage_')) return 'GPX stages';
  return 'Tracks';
}

const files = readdirSync(DIR).filter(f => f.startsWith('roadbook_') && f.endsWith('.json'));
const items = files.map(f => {
  const rb = JSON.parse(readFileSync(join(DIR, f), 'utf8'));
  const id = f.replace(/^roadbook_/, '').replace(/\.json$/, '');
  const b = rb.boxes || [];
  const km = rb.totalDistanceKm || (rb.meta && rb.meta.totalDistanceKm) || (b.length ? b[b.length-1].distTotalKm : 0);
  const name = rb.trackName || (rb.meta && rb.meta.trackName) || id;
  const turns = b.filter(x => x.type === 'turn' || x.type === 'hairpin').length;
  return { id, file: `roadbooks/${f}`, name, group: group(id), km: +km.toFixed(2), boxes: b.length, turns, difficulty: difficulty(rb) };
});

const ORDER = ['Real rally stages', 'Tracks', 'GPX stages', 'Practice drills'];
items.sort((a, b) => (ORDER.indexOf(a.group) - ORDER.indexOf(b.group)) || a.name.localeCompare(b.name));
writeFileSync(join(DIR, 'manifest.json'), JSON.stringify(items, null, 1));
console.log(`manifest: ${items.length} roadbooks -> ${join(DIR, 'manifest.json')}`);
