import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { FlyControls } from 'three/addons/controls/FlyControls.js';

let scene, camera, renderer, orbit, fly, group, clock, ok=false, mode='orbit';
let RB=null, EXAG=14, HF=null;            // HF = cached heightfield for the current map
const host = () => document.getElementById('view');

function init(){
  const h = host();
  try { renderer = new THREE.WebGLRenderer({ antialias:true }); }
  catch(e){ document.getElementById('stat').textContent='WebGL is not available in this browser.'; return; }
  ok = true; clock = new THREE.Clock();
  scene = new THREE.Scene(); scene.background = new THREE.Color(0x0e1118); scene.fog = new THREE.Fog(0x0e1118, 1100, 3200);
  camera = new THREE.PerspectiveCamera(58, (h.clientWidth||1)/(h.clientHeight||1), 0.5, 16000);
  camera.position.set(300, 220, 300);
  renderer.setPixelRatio(Math.min(2, window.devicePixelRatio||1)); renderer.setSize(h.clientWidth, h.clientHeight); h.appendChild(renderer.domElement);
  new ResizeObserver(onResize).observe(h);
  orbit = new OrbitControls(camera, renderer.domElement);
  orbit.enableDamping = true; orbit.maxPolarAngle = Math.PI*0.495; orbit.minDistance = 12; orbit.maxDistance = 4000;
  scene.add(new THREE.HemisphereLight(0xbcd0ff, 0x3a342c, 0.85));
  const sun = new THREE.DirectionalLight(0xfff1d8, 1.3); sun.position.set(-130, 280, 150); scene.add(sun);
  group = new THREE.Group(); scene.add(group);
  animate();
}
function onResize(){ const h=host(); if(!h||!ok)return; camera.aspect=(h.clientWidth||1)/(h.clientHeight||1); camera.updateProjectionMatrix(); renderer.setSize(h.clientWidth,h.clientHeight); }
function animate(){ if(!ok)return; requestAnimationFrame(animate); const dt=Math.min(0.05, clock.getDelta()); if(mode==='fly'&&fly) fly.update(dt); else orbit.update(); renderer.render(scene,camera); }
function clear(){ for(let i=group.children.length-1;i>=0;i--){ const o=group.children[i]; o.geometry&&o.geometry.dispose(); if(o.material){ Array.isArray(o.material)?o.material.forEach(m=>m.dispose()):o.material.dispose(); } group.remove(o); } }

// hypsometric terrain ramp: sand -> earth -> pale rock
function tRamp(t){ t=Math.max(0,Math.min(1,t));
  const a=new THREE.Color(0xb7a06b), b=new THREE.Color(0x877247), c=new THREE.Color(0xd9cfb6);
  return t<0.6 ? a.lerp(b, t/0.6) : b.lerp(c, (t-0.6)/0.4); }

// build (or reuse) a smoothed heightfield from the elevations sampled along the route
function heightfield(R){
  if(HF && HF.rb===RB) return HF;
  let minx=1e9,maxx=-1e9,minz=1e9,maxz=-1e9,miny=1e9,maxy=-1e9;
  R.forEach(p=>{ minx=Math.min(minx,p[0]);maxx=Math.max(maxx,p[0]);minz=Math.min(minz,p[1]);maxz=Math.max(maxz,p[1]);miny=Math.min(miny,p[3]);maxy=Math.max(maxy,p[3]); });
  const N=170, padx=(maxx-minx)*0.10||5, padz=(maxz-minz)*0.10||5;
  const x0=minx-padx, x1=maxx+padx, z0=minz-padz, z1=maxz+padz;
  const acc=new Float32Array(N*N), wt=new Float32Array(N*N), Rk=Math.max(3, Math.round(N*0.045));
  const GX=x=>(x-x0)/(x1-x0)*(N-1), GZ=z=>(z-z0)/(z1-z0)*(N-1);
  for(const p of R){ const ux=GX(p[0]), uz=GZ(p[1]), e=p[3];
    const i0=Math.max(0,Math.floor(ux-Rk)),i1=Math.min(N-1,Math.ceil(ux+Rk)),j0=Math.max(0,Math.floor(uz-Rk)),j1=Math.min(N-1,Math.ceil(uz+Rk));
    for(let j=j0;j<=j1;j++) for(let i=i0;i<=i1;i++){ const d2=(i-ux)*(i-ux)+(j-uz)*(j-uz); if(d2>Rk*Rk)continue; const w=Math.exp(-d2/(Rk*Rk*0.4)); const k=j*N+i; acc[k]+=w*e; wt[k]+=w; } }
  const H=new Float32Array(N*N); for(let k=0;k<N*N;k++) H[k]= wt[k]>0 ? acc[k]/wt[k] : miny;
  for(let pass=0;pass<3;pass++){ const T=H.slice(); for(let j=0;j<N;j++)for(let i=0;i<N;i++){ let s=0,n=0; for(let dj=-1;dj<=1;dj++)for(let di=-1;di<=1;di++){ const ii=i+di,jj=j+dj; if(ii<0||jj<0||ii>=N||jj>=N)continue; s+=T[jj*N+ii]; n++; } H[j*N+i]=s/n; } }
  HF={ rb:RB, N, x0,x1,z0,z1, minx,maxx,minz,maxz,miny,maxy, H }; return HF;
}

function build(){
  if(!ok) return; clear(); if(!RB) return;
  const R=RB.route||[]; const stat=document.getElementById('stat');
  if(R.length<2){ stat.textContent='This map has no route polyline to render in 3D.'; return; }
  const hf=heightfield(R);
  const { N,x0,x1,z0,z1,minx,maxx,minz,maxz,miny,maxy,H } = hf;
  const cx=(minx+maxx)/2, cz=(minz+maxz)/2, ext=Math.max(maxx-minx,maxz-minz)||1, hS=340/ext, vS=hS*EXAG, relief=Math.max(0.001,maxy-miny);

  // terrain mesh
  const verts=[],cols=[],idx=[];
  for(let j=0;j<N;j++)for(let i=0;i<N;i++){ const wx=x0+(x1-x0)*i/(N-1), wz=z0+(z1-z0)*j/(N-1), e=H[j*N+i];
    verts.push((wx-cx)*hS, (e-miny)*vS, (wz-cz)*hS); const c=tRamp((e-miny)/relief); cols.push(c.r,c.g,c.b); }
  for(let j=0;j<N-1;j++)for(let i=0;i<N-1;i++){ const a=j*N+i,b=a+1,c2=a+N,d=c2+1; idx.push(a,c2,b, b,c2,d); }
  const tg=new THREE.BufferGeometry();
  tg.setAttribute('position', new THREE.Float32BufferAttribute(verts,3));
  tg.setAttribute('color', new THREE.Float32BufferAttribute(cols,3));
  tg.setIndex(idx); tg.computeVertexNormals();
  group.add(new THREE.Mesh(tg, new THREE.MeshStandardMaterial({ vertexColors:true, roughness:.97, metalness:0 })));

  // route draped over the terrain
  const off=Math.max(1.2, relief*vS*0.012);
  const v3=p=>new THREE.Vector3((p[0]-cx)*hS, (p[3]-miny)*vS+off, (p[1]-cz)*hS);
  const pts=R.map(v3);
  const tube=new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts,false,'catmullrom',0.1), Math.min(3000,pts.length*2), 1.5, 8, false);
  group.add(new THREE.Mesh(tube, new THREE.MeshStandardMaterial({ color:0x3ec6ff, emissive:0x0b3b58, emissiveIntensity:.7, roughness:.35 })));

  // waypoint markers by distance
  const cum=R.map(p=>p[2]); const totalM=cum[cum.length-1]||1;
  const atD=dM=>{ for(let i=1;i<R.length;i++) if(cum[i]>=dM) return pts[i]; return pts[pts.length-1]; };
  (RB.boxes||[]).forEach(b=>{ const p=atD((b.distTotalKm||0)*1000); if(!p)return; let col=0xe9eef6,r=1.6;
    if(b.type==='start'){col=0x1f9d4e;r=3.4;} else if(b.type==='finish'){col=0xcd2620;r=3.4;}
    else if(b.dangerLevel>=2){col=0xcd2620;r=2.5;} else if(b.dangerLevel){col=0xe0902a;r=2.2;} else if(b.type==='hairpin'){col=0xffb028;r=2.3;}
    const m=new THREE.Mesh(new THREE.SphereGeometry(r,14,14), new THREE.MeshStandardMaterial({ color:col, emissive:col, emissiveIntensity:.35, roughness:.4 }));
    m.position.copy(p); group.add(m); });

  const yc=relief*vS*0.5; orbit.target.set(0,yc,0); camera.position.set(320, 200+yc, 320); orbit.update();
  stat.innerHTML=`<b>${RB.trackName||(RB.meta&&RB.meta.trackName)||'route'}</b> — ${(totalM/1000).toFixed(2)} km · ▲ ${Math.round(maxy-miny)} m relief`;
}

function load(file){ document.getElementById('stat').textContent='Loading…'; HF=null;
  fetch(file).then(r=>r.json()).then(rb=>{ RB=rb; HF=null; build(); }).catch(()=>{ document.getElementById('stat').textContent='Could not load that map.'; }); }

// fly / orbit toggle
function setMode(m){
  mode=m; const btn=document.getElementById('modeBtn'), hint=document.getElementById('flyhint');
  if(m==='fly'){ orbit.enabled=false;
    fly=new FlyControls(camera, renderer.domElement); fly.movementSpeed=Math.max(60, 340*0.35); fly.rollSpeed=0.5; fly.dragToLook=true; fly.autoForward=false;
    btn.textContent='Orbit view'; btn.classList.add('on'); if(hint) hint.style.display='block';
  } else {
    if(fly){ fly.dispose(); fly=null; } orbit.enabled=true;
    btn.textContent='Fly-through'; btn.classList.remove('on'); if(hint) hint.style.display='none';
  }
}

// controls + picker
fetch('roadbooks/manifest.json').then(r=>r.json()).then(m=>{
  const sel=document.getElementById('pick'), groups={};
  m.forEach(it=>{ (groups[it.group]=groups[it.group]||[]).push(it); });
  Object.keys(groups).forEach(g=>{ const og=document.createElement('optgroup'); og.label=g;
    groups[g].forEach(it=>{ const o=document.createElement('option'); o.value=it.file; o.textContent=it.name+' · '+it.km+' km'; og.appendChild(o); });
    sel.appendChild(og); });
  sel.onchange=()=>load(sel.value);
  const first=m.find(it=>it.id.startsWith('rally_'))||m[0]; if(first){ sel.value=first.file; load(first.file); }
}).catch(()=>{ document.getElementById('stat').textContent='Run from a web server to load maps (file:// blocks fetch).'; });

document.getElementById('exag').oninput=e=>{ EXAG=+e.target.value; document.getElementById('exagOut').textContent=EXAG+'×'; if(RB) build(); };
document.getElementById('modeBtn').onclick=()=> setMode(mode==='orbit'?'fly':'orbit');

init();
