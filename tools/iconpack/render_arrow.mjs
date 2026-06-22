import { Resvg } from '@resvg/resvg-js';
import { readFileSync, writeFileSync } from 'fs';
const svg = readFileSync('arrows/distance-arrow.svg', 'utf8');
const r = new Resvg(svg, { fitTo: { mode: 'width', value: 200 }, background: 'rgba(0,0,0,0)' });
writeFileSync('arrows/distance-arrow.png', r.render().asPng());
console.log('rendered distance-arrow.png');
