import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { FlyControls } from 'three/addons/controls/FlyControls.js';

let scene, camera, renderer, orbit, fly, group, clock, ok=false, mode='orbit';
let EXAG=1, CUR=null;                       // CUR = { meta, heights, tex }
const W=1000;                                // terrain world width
const host=()=>document.getElementById('view');

function init(){
  const h=host();
  try{ renderer=new THREE.WebGLRenderer({antialias:true}); }
  catch(e){ document.getElementById('stat').textContent='WebGL is not available.'; return; }
  ok=true; clock=new THREE.Clock();
  scene=new THREE.Scene(); scene.background=new THREE.Color(0x0d1016); scene.fog=new THREE.Fog(0x0d1016, W*1.4, W*3.2);
  camera=new THREE.PerspectiveCamera(56, (h.clientWidth||1)/(h.clientHeight||1), 0.5, W*20);
  camera.position.set(W*0.6, W*0.5, W*0.6);
  renderer.setPixelRatio(Math.min(2,window.devicePixelRatio||1)); renderer.setSize(h.clientWidth,h.clientHeight); h.appendChild(renderer.domElement);
  new ResizeObserver(onResize).observe(h);
  orbit=new OrbitControls(camera,renderer.domElement); orbit.enableDamping=true; orbit.maxPolarAngle=Math.PI*0.495; orbit.minDistance=30; orbit.maxDistance=W*6;
  scene.add(new THREE.HemisphereLight(0xcfe0ff, 0x46443c, 0.9));
  const sun=new THREE.DirectionalLight(0xfff4e0, 1.25); sun.position.set(-0.4*W, W, 0.5*W); scene.add(sun);
  group=new THREE.Group(); scene.add(group);
  animate();
}
function onResize(){ const h=host(); if(!h||!ok)return; camera.aspect=(h.clientWidth||1)/(h.clientHeight||1); camera.updateProjectionMatrix(); renderer.setSize(h.clientWidth,h.clientHeight); }
function animate(){ if(!ok)return; requestAnimationFrame(animate); const dt=Math.min(0.05,clock.getDelta()); if(mode==='fly'&&fly) fly.update(dt); else orbit.update(); renderer.render(scene,camera); }
function clear(){ for(let i=group.children.length-1;i>=0;i--){ const o=group.children[i]; o.geometry&&o.geometry.dispose(); group.remove(o); } }

// Robust vertical range: PiBoSo heightmaps peg "outside the terrain" cells at
// exactly 0 or 65535 (no-data sentinels). Normalizing by raw min/max turns those
// blocks into towering walls / pits. Ignore the pegged extremes and scale by the
// real terrain's 0.5..99.5 percentile, then clamp — kills the random cliffs while
// keeping genuine relief (real canyon walls aren't at the encoding's exact max).
function robustRange(heights){
  const valid=[];
  for(let k=0;k<heights.length;k++){ const v=heights[k]; if(v>0 && v<65535) valid.push(v); }
  if(valid.length < heights.length*0.2){ let lo=Infinity,hi=-Infinity; for(let k=0;k<heights.length;k++){ const v=heights[k]; if(v<lo)lo=v; if(v>hi)hi=v; } return {lo,hi}; }
  valid.sort((a,b)=>a-b);
  return { lo: valid[Math.floor(valid.length*0.005)], hi: valid[Math.floor(valid.length*0.995)] };
}

// Adaptive vertical scale: the heightmaps carry no real metres, so a fixed
// scale over-exaggerates steep terrain into walls. Size the vertical so the
// terrain's *typical* slope lands near ~16deg, then clamp — flat tracks gain
// relief, extreme bimodal desert (Dubai, Empty Quarter) flattens toward sane.
function autoVScale(heights, lo, hi){
  const N=Math.round(Math.sqrt(heights.length)), cell=W/(N-1), range=Math.max(1,hi-lo);
  const nrm=k=>Math.min(1,Math.max(0,(heights[k]-lo)/range));
  let sum=0, cnt=0;
  for(let j=1;j<N-1;j++)for(let i=1;i<N-1;i++){ const k=j*N+i;
    const gx=(nrm(k+1)-nrm(k-1))*0.5, gy=(nrm(k+N)-nrm(k-N))*0.5;
    sum+=Math.hypot(gx,gy); cnt++; }
  const rough=Math.max(1e-4, sum/Math.max(1,cnt));
  const vs=Math.tan(16*Math.PI/180)*cell/rough;
  return Math.min(W*0.18, Math.max(W*0.035, vs));
}

function buildTerrain(){
  if(!ok||!CUR) return; clear();
  const { meta, heights, tex, lo, hi, autoVS } = CUR; const N=meta.size;
  const range=Math.max(1,hi-lo), vScale=autoVS*EXAG;
  const verts=new Float32Array(N*N*3), uvs=new Float32Array(N*N*2);
  for(let j=0;j<N;j++)for(let i=0;i<N;i++){ const k=j*N+i;
    const t=Math.min(1,Math.max(0,(heights[k]-lo)/range));
    verts[k*3]=(i/(N-1)-0.5)*W; verts[k*3+1]=t*vScale; verts[k*3+2]=(j/(N-1)-0.5)*W;
    uvs[k*2]=i/(N-1); uvs[k*2+1]=1-j/(N-1); }
  const idx=[]; for(let j=0;j<N-1;j++)for(let i=0;i<N-1;i++){ const a=j*N+i,b=a+1,c=a+N,d=c+1; idx.push(a,c,b,b,c,d); }
  const g=new THREE.BufferGeometry();
  g.setAttribute('position',new THREE.BufferAttribute(verts,3));
  g.setAttribute('uv',new THREE.BufferAttribute(uvs,2));
  g.setIndex(idx); g.computeVertexNormals();
  tex.colorSpace=THREE.SRGBColorSpace; tex.anisotropy=8;
  group.add(new THREE.Mesh(g, new THREE.MeshStandardMaterial({ map:tex, roughness:.96, metalness:0 })));
  const yc=vScale*0.45; orbit.target.set(0,yc,0); camera.position.set(W*0.62,W*0.5+yc,W*0.62); orbit.update();
  document.getElementById('stat').innerHTML=`<b>${meta.name}</b> — real terrain · ${N}×${N} heightmap · real aerial`;
}

function load(id, name){
  document.getElementById('stat').textContent='Loading '+(name||id)+'…';
  const base='tracks/'+id+'/';
  Promise.all([
    fetch(base+'meta.json').then(r=>r.json()),
    fetch(base+'height.bin').then(r=>r.arrayBuffer()).then(b=>new Uint16Array(b)),
    new Promise((res,rej)=>new THREE.TextureLoader().load(base+'aerial.jpg', res, undefined, rej))
  ]).then(([meta,heights,tex])=>{ const r=robustRange(heights); CUR={meta,heights,tex,...r,autoVS:autoVScale(heights,r.lo,r.hi)}; buildTerrain(); })
    .catch(()=>{ document.getElementById('stat').textContent='Could not load '+(name||id)+' (run from a web server).'; });
}

function setMode(m){ mode=m; const btn=document.getElementById('modeBtn'), hint=document.getElementById('flyhint');
  if(m==='fly'){ orbit.enabled=false; fly=new FlyControls(camera,renderer.domElement); fly.movementSpeed=W*0.4; fly.rollSpeed=0.5; fly.dragToLook=true; fly.autoForward=false;
    btn.textContent='Orbit view'; btn.classList.add('on'); if(hint)hint.style.display='block';
  } else { if(fly){ fly.dispose(); fly=null; } orbit.enabled=true; btn.textContent='Fly-through'; btn.classList.remove('on'); if(hint)hint.style.display='none'; }
}

fetch('tracks/manifest.json').then(r=>r.json()).then(list=>{
  const sel=document.getElementById('pick');
  list.forEach(t=>{ const o=document.createElement('option'); o.value=t.id; o.dataset.name=t.name; o.textContent=t.name; sel.appendChild(o); });
  sel.onchange=()=>load(sel.value, sel.selectedOptions[0].dataset.name);
  if(list[0]) load(list[0].id, list[0].name);
}).catch(()=>{ document.getElementById('stat').textContent='No extracted tracks yet (run tools/extract_track.py).'; });

document.getElementById('exag').oninput=e=>{ EXAG=+e.target.value; document.getElementById('exagOut').textContent=EXAG.toFixed(1)+'×'; if(CUR) buildTerrain(); };
document.getElementById('modeBtn').onclick=()=> setMode(mode==='orbit'?'fly':'orbit');

init();
