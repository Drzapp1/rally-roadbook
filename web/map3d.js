import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

let scene, camera, renderer, controls, group, ok=false;
let RB=null, EXAG=14, dropsOn=true;
const host = () => document.getElementById('view');

function init(){
  const h = host();
  try { renderer = new THREE.WebGLRenderer({ antialias:true }); }
  catch(e){ document.getElementById('stat').textContent = 'WebGL is not available in this browser.'; return; }
  ok = true;
  scene = new THREE.Scene(); scene.background = new THREE.Color(0x0d0f14); scene.fog = new THREE.Fog(0x0d0f14, 700, 1900);
  camera = new THREE.PerspectiveCamera(55, h.clientWidth/h.clientHeight, 0.1, 8000);
  camera.position.set(240, 180, 240);
  renderer.setPixelRatio(Math.min(2, window.devicePixelRatio||1));
  renderer.setSize(h.clientWidth, h.clientHeight); h.appendChild(renderer.domElement);
  new ResizeObserver(onResize).observe(h);
  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true; controls.maxPolarAngle = Math.PI*0.49; controls.minDistance = 20; controls.maxDistance = 1600;
  scene.add(new THREE.HemisphereLight(0xc8d8ff, 0x141820, 1.15));
  const d = new THREE.DirectionalLight(0xffffff, 1.0); d.position.set(150, 280, 90); scene.add(d);
  group = new THREE.Group(); scene.add(group);
  window.addEventListener('resize', onResize);
  animate();
}
function onResize(){ const h=host(); if(!h||!ok)return; camera.aspect=h.clientWidth/h.clientHeight; camera.updateProjectionMatrix(); renderer.setSize(h.clientWidth,h.clientHeight); }
function animate(){ if(!ok)return; requestAnimationFrame(animate); controls.update(); renderer.render(scene,camera); }
function clear(){ for(let i=group.children.length-1;i>=0;i--){ const o=group.children[i]; o.geometry&&o.geometry.dispose(); if(o.material){ Array.isArray(o.material)?o.material.forEach(m=>m.dispose()):o.material.dispose(); } group.remove(o); } }
function ramp(t){ const c=new THREE.Color(); c.setHSL(0.55-0.46*Math.max(0,Math.min(1,t)), 0.62, 0.52); return c; }

function build(){
  if(!ok) return; clear(); if(!RB) return;
  const R = RB.route||[];
  const stat = document.getElementById('stat');
  if(R.length<2){ stat.textContent='This map has no route polyline to render in 3D.'; return; }
  let minx=1e9,maxx=-1e9,minz=1e9,maxz=-1e9,miny=1e9,maxy=-1e9;
  // route point = [x, z, distM, elevation]
  R.forEach(p=>{ minx=Math.min(minx,p[0]);maxx=Math.max(maxx,p[0]);minz=Math.min(minz,p[1]);maxz=Math.max(maxz,p[1]);miny=Math.min(miny,p[3]);maxy=Math.max(maxy,p[3]); });
  const cx=(minx+maxx)/2, cz=(minz+maxz)/2, ext=Math.max(maxx-minx, maxz-minz)||1;
  const hS = 320/ext, vS = hS*EXAG, relief = maxy-miny;
  const yspan = Math.max(0.001, relief*vS);
  const v3 = p => new THREE.Vector3((p[0]-cx)*hS, (p[3]-miny)*vS, (p[1]-cz)*hS);
  const pts = R.map(v3);

  const curve = new THREE.CatmullRomCurve3(pts, false, 'catmullrom', 0.15);
  const tube = new THREE.TubeGeometry(curve, Math.min(3000, pts.length*2), 1.7, 8, false);
  const pos = tube.attributes.position, col = [];
  for(let i=0;i<pos.count;i++){ const c=ramp(pos.getY(i)/yspan); col.push(c.r,c.g,c.b); }
  tube.setAttribute('color', new THREE.Float32BufferAttribute(col,3));
  group.add(new THREE.Mesh(tube, new THREE.MeshStandardMaterial({ vertexColors:true, roughness:.6, metalness:.05 })));

  // ground-projected shadow of the route
  group.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints(pts.map(p=>new THREE.Vector3(p.x,0.15,p.z))),
                           new THREE.LineBasicMaterial({ color:0x3a414e })));
  // ground grid
  group.add(new THREE.GridHelper(680, 68, 0x242b39, 0x161b25));

  // waypoint markers placed by distance along the route
  const cum = R.map(p=>p[2]); const totalM = cum[cum.length-1]||1;
  const atDist = dM => { for(let i=1;i<R.length;i++){ if(cum[i]>=dM) return pts[i]; } return pts[pts.length-1]; };
  (RB.boxes||[]).forEach(b=>{
    const p = atDist((b.distTotalKm||0)*1000); if(!p) return;
    let color=0x9fb0c4, r=1.5;
    if(b.type==='start'){ color=0x1f9d4e; r=3.2; }
    else if(b.type==='finish'){ color=0xcd2620; r=3.2; }
    else if(b.dangerLevel>=2){ color=0xcd2620; r=2.4; }
    else if(b.dangerLevel){ color=0xe0902a; r=2.1; }
    else if(b.type==='hairpin'){ color=0xffb028; r=2.2; }
    else if(b.type==='finish'){ color=0xcd2620; }
    const m = new THREE.Mesh(new THREE.SphereGeometry(r,14,14),
              new THREE.MeshStandardMaterial({ color, emissive:color, emissiveIntensity:.3, roughness:.4 }));
    m.position.copy(p); group.add(m);
    if(dropsOn){ group.add(new THREE.Line(new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(p.x,0,p.z), p]),
                 new THREE.LineBasicMaterial({ color, transparent:true, opacity:.35 }))); }
  });

  controls.target.set(0, yspan*0.5, 0);
  camera.position.set(260, 160+yspan*0.5, 260); controls.update();
  const nm = RB.trackName || (RB.meta&&RB.meta.trackName) || 'route';
  stat.innerHTML = `<b>${nm}</b> — ${(totalM/1000).toFixed(2)} km · ▲ ${Math.round(relief)} m relief`;
}

function load(file){ document.getElementById('stat').textContent='Loading…';
  fetch(file).then(r=>r.json()).then(rb=>{ RB=rb; build(); }).catch(()=>{ document.getElementById('stat').textContent='Could not load that map.'; }); }

// picker from the manifest
fetch('roadbooks/manifest.json').then(r=>r.json()).then(m=>{
  const sel=document.getElementById('pick'); const groups={};
  m.forEach(it=>{ (groups[it.group]=groups[it.group]||[]).push(it); });
  Object.keys(groups).forEach(g=>{ const og=document.createElement('optgroup'); og.label=g;
    groups[g].forEach(it=>{ const o=document.createElement('option'); o.value=it.file; o.textContent=it.name+' · '+it.km+' km'; og.appendChild(o); });
    sel.appendChild(og); });
  sel.onchange=()=>load(sel.value);
  const first = m.find(it=>it.id.startsWith('rally_')) || m[0];
  if(first){ sel.value=first.file; load(first.file); }
}).catch(()=>{ document.getElementById('stat').textContent='Run from a web server to load maps (file:// blocks fetch).'; });

document.getElementById('exag').oninput=e=>{ EXAG=+e.target.value; document.getElementById('exagOut').textContent=EXAG+'×'; if(RB) build(); };
document.getElementById('drops').onchange=e=>{ dropsOn=e.target.checked; if(RB) build(); };

init();
