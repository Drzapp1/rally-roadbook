// roadbook.js — reference reader / renderer for the open Rally Roadbook format v1.
// Zero dependencies, ES module. Spec + schema: format.html, roadbook.schema.json.
//
//   import { renderRoadbook, injectStyles, validate } from 'roadbook.js';
//   injectStyles();
//   renderRoadbook(el, roadbookObject, { base: 'https://…/rally-roadbook/' });

export function validate(rb) {
  const issues = [], warns = [];
  if (rb === null || typeof rb !== 'object' || Array.isArray(rb)) { issues.push('top level must be a JSON object'); return { valid: false, issues, warns }; }
  if (rb.schemaVersion !== 1) warns.push('schemaVersion should be 1');
  const boxes = rb.boxes;
  if (!Array.isArray(boxes) || !boxes.length) { issues.push('no boxes'); return { valid: false, issues, warns }; }
  let prev = -1, prevType = null;
  boxes.forEach((b, i) => {
    ['distTotalKm', 'capDeg', 'type', 'tulip'].forEach(k => { if (!(k in b)) issues.push(`box ${i}: missing ${k}`); });
    const d = b.distTotalKm, t = b.type;
    if (typeof d === 'number') {
      if (Number.isNaN(d)) issues.push(`box ${i}: distTotalKm is NaN`);
      if (d < prev - 1e-6) issues.push(`box ${i}: distance goes backwards`);
      const edge = ['start', 'finish'].includes(t) || ['start', 'finish'].includes(prevType);
      if (!edge && prev >= 0 && (d - prev) * 1000 < 5) issues.push(`box ${i}: coincident with previous (<5 m)`);
      prev = d;
    }
    prevType = t;
    const c = b.capDeg;
    if (typeof c === 'number' && !(c >= 0 && c < 360)) issues.push(`box ${i}: cap out of range (${c})`);
    const pts = (b.tulip && b.tulip.points) || [];
    if (pts.length < 2) issues.push(`box ${i}: tulip < 2 points`);
    if (pts.some(p => !Array.isArray(p) || p.length < 2 || !(p[0] >= 0 && p[0] <= 1 && p[1] >= 0 && p[1] <= 1))) issues.push(`box ${i}: tulip point out of [0,1]`);
  });
  const km = boxes[boxes.length - 1].distTotalKm || 0;
  if (km > 0 && boxes.length / km > 25) warns.push(`high box density: ${(boxes.length / km).toFixed(1)}/km`);
  return { valid: !issues.length, issues, warns, boxes: boxes.length };
}

// Classify a box into one of the 14 turn-tulip codes (matches the in-game plugin).
export function tulipCode(b) {
  const t = b.type, R = b.turnDir === 'right';
  if (t === 'start') return 'start';
  if (t === 'finish') return 'finish';
  if (t === 'hairpin') return R ? 'hairpinR' : 'hairpinL';
  if (t === 'turn') {
    const p = (b.tulip && b.tulip.points) || [];
    if (p.length < 2) return R ? 'right' : 'left';
    const a = p[0], c = p[1], d = p[p.length - 2], e = p[p.length - 1];
    const v1 = [c[0] - a[0], c[1] - a[1]], v2 = [e[0] - d[0], e[1] - d[1]];
    const m = Math.acos(Math.max(-1, Math.min(1, (v1[0] * v2[0] + v1[1] * v2[1]) / ((Math.hypot(...v1) || 1) * (Math.hypot(...v2) || 1))))) * 180 / Math.PI;
    if (m < 32) return R ? 'slightR' : 'slightL';
    if (m >= 100) return R ? 'sharpR' : 'sharpL';
    return R ? 'right' : 'left';
  }
  return 'straight';
}

export function turnAbbrev(b) { const t = b.type; return t === 'start' ? 'S' : t === 'finish' ? 'F' : t === 'straight' ? 'kpS' : (b.turnDir === 'right' ? 'R' : 'L'); }
export function totalKm(rb) { const b = (rb && rb.boxes) || [], m = (rb && rb.meta) || {}; return rb.totalDistanceKm || m.totalDistanceKm || (b.length ? b[b.length - 1].distTotalKm : 0); }
const comma = v => (v || 0).toFixed(2).replace('.', ',');

// Render a roadbook into `el` as a grid of cells, using the hosted tulip + sign
// art under opts.base (default '' = same origin). Returns el.
export function renderRoadbook(el, rb, opts = {}) {
  const base = opts.base || '';
  el.classList.add('rb-grid'); el.innerHTML = '';
  (rb.boxes || []).forEach(b => {
    const part = b.type === 'start' ? '0,00' : (b.type === 'finish' ? 'END' : ((b.distPartialM || 0) + ' m'));
    const sg = (b.pictograms || []).slice(0, 4).map(c => `<img src="${base}signs/${c}.png" onerror="this.remove()" alt="${c}">`).join('');
    const dl = b.dangerLevel ? `<span class="rb-dl">${'!'.repeat(b.dangerLevel)}</span>` : '';
    const cell = document.createElement('div'); cell.className = 'rb-cell';
    cell.innerHTML = `<div class="rb-l"><span class="rb-i">${b.index ?? ''}</span><div class="rb-tot">${comma(b.distTotalKm)}</div><div class="rb-part">${part}</div></div>`
      + `<div class="rb-c"><img src="${base}tulips/${tulipCode(b)}.png" alt=""></div>`
      + `<div class="rb-r"><div class="rb-cap">${turnAbbrev(b)}</div><div class="rb-deg">${String(b.capDeg || 0).padStart(3, '0')}</div>${dl}<div class="rb-sg">${sg}</div></div>`;
    el.appendChild(cell);
  });
  return el;
}

// Inject minimal default styles once (skip if you supply your own .rb-* CSS).
export function injectStyles() {
  if (document.getElementById('rb-css')) return;
  const s = document.createElement('style'); s.id = 'rb-css';
  s.textContent = `.rb-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(220px,1fr));gap:8px;font-family:system-ui,sans-serif}
.rb-cell{display:flex;background:#14171e;border:1px solid #2a2e37;border-radius:9px;overflow:hidden;min-height:92px;color:#e6e8ec}
.rb-l{flex:0 0 32%;border-right:1px solid #2a2e37;padding:8px}.rb-i{font-size:10px;color:#8b919c}.rb-tot{font-size:22px;font-weight:700;font-family:ui-monospace,monospace}.rb-part{margin-top:6px;font-size:12px;color:#8b919c}
.rb-c{flex:0 0 40%;border-right:1px solid #2a2e37;display:flex;align-items:center;justify-content:center;padding:4px}.rb-c img{width:100%;max-height:78px;object-fit:contain}
.rb-r{flex:1;text-align:center;padding:8px 4px}.rb-cap{font-size:20px;font-weight:700}.rb-deg{font-size:11px;color:#8b919c}.rb-dl{color:#ff5a4d;font-weight:700}
.rb-sg{display:flex;gap:3px;justify-content:center;margin-top:4px}.rb-sg img{width:20px;height:20px;background:#f6f4ee;border-radius:4px}`;
  document.head.appendChild(s);
}
