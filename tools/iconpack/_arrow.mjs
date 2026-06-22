import { Resvg } from '@resvg/resvg-js';
import { readFileSync, writeFileSync } from 'fs';
const r = new Resvg(readFileSync('arrowhead.svg','utf8'), { fitTo:{mode:'width',value:128}, background:'rgba(0,0,0,0)' });
writeFileSync('arrows/arrow_up.png', r.render().asPng());
