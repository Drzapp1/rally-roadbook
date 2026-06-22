import { Resvg } from '@resvg/resvg-js';
import { readFileSync, writeFileSync } from 'fs';
const r = new Resvg(readFileSync(process.argv[2],'utf8'), { fitTo:{mode:'width', value:760} });
writeFileSync('sheet_preview.png', r.render().asPng());
console.log('rendered sheet_preview.png');
