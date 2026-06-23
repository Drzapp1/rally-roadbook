// Regenerate web/sitemap.xml from the actual web/*.html set (excludes 404).
// Run from the repo root (e.g. in the Pages workflow) so the sitemap can never
// go stale when pages are added or removed.
import { readdirSync, writeFileSync } from 'node:fs';

const BASE = 'https://drzapp1.github.io/rally-roadbook/';
const pages = readdirSync('web')
  .filter(f => f.endsWith('.html') && f !== '404.html')
  .sort();

// The root URL serves index.html (its canonical), so list root once and skip a
// separate index.html entry; every other page gets its own URL.
const entries = [{ loc: BASE, pri: '1.0' }];
for (const f of pages) {
  if (f === 'index.html') continue;
  entries.push({ loc: BASE + f, pri: f === 'home.html' ? '0.9' : '0.7' });
}

const body = entries
  .map(e => `  <url><loc>${e.loc}</loc><changefreq>weekly</changefreq><priority>${e.pri}</priority></url>`)
  .join('\n');
const xml = `<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
${body}
</urlset>
`;

writeFileSync('web/sitemap.xml', xml);
console.log(`sitemap.xml: ${entries.length} urls`);
