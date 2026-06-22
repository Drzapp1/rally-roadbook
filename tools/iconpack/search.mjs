const tree = await (await fetch('https://api.github.com/repos/game-icons/icons/git/trees/master?recursive=1', { headers: { 'User-Agent': 'cc' } })).json();
const names = tree.tree.filter(t => t.path.endsWith('.svg')).map(t => t.path.slice(t.path.lastIndexOf('/') + 1, -4));
for (const kw of ['bump', 'mound', 'mole', 'hill', 'hump', 'tire', 'tyre', 'wheel', 'track', 'rut', 'skid', 'tread', 'fork', 'junction', 'tee', 'split', 'cross-road', 'roads', 'dip', 'ditch', 'sand', 'dune', 'rock', 'stone']) {
  const m = names.filter(n => n.includes(kw)).slice(0, 12);
  if (m.length) console.log(kw.padEnd(10), '->', m.join(', '));
}
