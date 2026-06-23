// Rally Roadbook PWA service worker — offline app shell + runtime caching so a
// roadbook stays readable on the bars without signal once it has been opened.
const C = 'rally-v2';
const TULIPS = ['straight','slightL','slightR','left','right','sharpL','sharpR','hairpinL','hairpinR','forkL','forkR','cross','start','finish'].map(c => 'tulips/' + c + '.png');
const SHELL = ['rally.html', 'manifest.webmanifest', 'icons/rally-192.png', 'icons/rally-512.png', 'roadbooks/manifest.json', ...TULIPS];
self.addEventListener('install', e => {
  e.waitUntil(caches.open(C).then(c => c.addAll(SHELL)).then(() => self.skipWaiting()));
});
self.addEventListener('activate', e => {
  e.waitUntil(caches.keys().then(ks => Promise.all(ks.filter(k => k !== C).map(k => caches.delete(k)))).then(() => self.clients.claim()));
});
self.addEventListener('fetch', e => {
  const r = e.request;
  if (r.method !== 'GET' || new URL(r.url).origin !== location.origin) return;
  e.respondWith(
    caches.match(r).then(hit => hit || fetch(r).then(res => {
      const cp = res.clone(); caches.open(C).then(c => c.put(r, cp)); return res;
    }).catch(() => hit))
  );
});
