import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { FlyControls } from 'three/addons/controls/FlyControls.js';
import { tulipCode } from './roadbook.js';

let scene, camera, renderer, orbit, fly, group, clock, ok=false, mode='orbit';
let RB=null, EXAG=14, HF=null, noteGroup=null, NOTES_ON=true;   // HF = cached heightfield; noteGroup = roadbook billboards
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

// procedural ground texture (colour + normal map) chosen by the stage's surface
const TEXCACHE = {};
function themeOf(rb){ let s=0,r=0;
  (rb.boxes||[]).forEach(b=>(b.pictograms||[]).forEach(p=>{
    if(['sand','dune','dunes','sandpit','camelgrass','chott'].includes(p)) s++;
    if(['rocks','rocky','cliff','mountain','collapse'].includes(p)) r++; }));
  return (s>r && s>0) ? 'sand' : (r>0 ? 'rock' : 'earth'); }
function makeGround(theme){
  if(TEXCACHE[theme]) return TEXCACHE[theme];
  const S=256, P=8;
  const pal = ({ sand:{lo:[150,124,84],mid:[196,170,122],hi:[220,200,158],grain:22},
                 rock:{lo:[74,67,58],mid:[134,122,107],hi:[178,168,152],grain:30},
                 earth:{lo:[88,71,46],mid:[150,124,84],hi:[186,162,112],grain:24} })[theme];
  const h=(a,b)=>{ a=((a%P)+P)%P; b=((b%P)+P)%P; let n=(Math.imul(a,374761393)+Math.imul(b,668265263))|0; n=Math.imul(n^(n>>>13),1274126177); return ((n^(n>>>16))>>>0)/4294967295; };
  const vn=(x,y)=>{ const xi=Math.floor(x),yi=Math.floor(y),xf=x-xi,yf=y-yi,u=xf*xf*(3-2*xf),v=yf*yf*(3-2*yf);
    const a=h(xi,yi),b=h(xi+1,yi),c=h(xi,yi+1),d=h(xi+1,yi+1); return (a*(1-u)+b*u)*(1-v)+(c*(1-u)+d*u)*v; };
  const fbm=(x,y)=>{ let s=0,amp=.5,f=1; for(let o=0;o<5;o++){ s+=amp*vn(x*f,y*f); amp*=.5; f*=2; } return s; };
  const lerp=(A,B,k)=>A+(B-A)*k;
  const mapCv=document.createElement('canvas'); mapCv.width=mapCv.height=S; const mc=mapCv.getContext('2d'); const mi=mc.createImageData(S,S);
  const hgt=new Float32Array(S*S);
  for(let j=0;j<S;j++)for(let i=0;i<S;i++){ const x=i/S*P, y=j/S*P, t=fbm(x,y), g=fbm(x*4+9,y*4+3); hgt[j*S+i]=t;
    let r,gg,bb; if(t<.5){ const k=t*2; r=lerp(pal.lo[0],pal.mid[0],k); gg=lerp(pal.lo[1],pal.mid[1],k); bb=lerp(pal.lo[2],pal.mid[2],k); }
    else { const k=(t-.5)*2; r=lerp(pal.mid[0],pal.hi[0],k); gg=lerp(pal.mid[1],pal.hi[1],k); bb=lerp(pal.mid[2],pal.hi[2],k); }
    const gr=(g-.5)*pal.grain, o=(j*S+i)*4; mi.data[o]=r+gr; mi.data[o+1]=gg+gr; mi.data[o+2]=bb+gr; mi.data[o+3]=255; }
  mc.putImageData(mi,0,0);
  const nrmCv=document.createElement('canvas'); nrmCv.width=nrmCv.height=S; const nc=nrmCv.getContext('2d'); const ni=nc.createImageData(S,S);
  for(let j=0;j<S;j++)for(let i=0;i<S;i++){ const l=hgt[j*S+((i-1+S)%S)],rr=hgt[j*S+((i+1)%S)],u=hgt[((j-1+S)%S)*S+i],dn=hgt[((j+1)%S)*S+i];
    let nx=(l-rr)*2.5, ny=(u-dn)*2.5, nz=1; const il=1/Math.hypot(nx,ny,nz); nx*=il;ny*=il;nz*=il; const o=(j*S+i)*4;
    ni.data[o]=(nx*.5+.5)*255; ni.data[o+1]=(ny*.5+.5)*255; ni.data[o+2]=(nz*.5+.5)*255; ni.data[o+3]=255; }
  nc.putImageData(ni,0,0);
  const mt=new THREE.CanvasTexture(mapCv), nt=new THREE.CanvasTexture(nrmCv);
  [mt,nt].forEach(t=>{ t.wrapS=t.wrapT=THREE.RepeatWrapping; t.anisotropy=4; });
  return (TEXCACHE[theme]={ map:mt, normal:nt });
}

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

function roundRect(g, x, y, w, h, r){ g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); }
function noteSprite(b){                                   // a floating roadbook-note billboard: tulip + first sign
  const cv = document.createElement('canvas'); cv.width = cv.height = 128; const g = cv.getContext('2d');
  g.fillStyle = 'rgba(18,22,30,.92)'; roundRect(g, 3, 3, 122, 122, 16); g.fill();
  g.lineWidth = 5; g.strokeStyle = b.dangerLevel >= 2 ? '#e7574e' : b.dangerLevel ? '#e0902a' : 'rgba(255,176,40,.7)'; roundRect(g, 3, 3, 122, 122, 16); g.stroke();
  const tex = new THREE.CanvasTexture(cv);
  const img = new Image(); img.src = 'tulips/' + tulipCode(b) + '.png'; img.onload = () => { g.drawImage(img, 16, 8, 96, 96); tex.needsUpdate = true; };
  const sg = (b.pictograms || [])[0];
  if (sg){ const si = new Image(); si.src = 'signs/' + sg + '.png'; si.onload = () => { g.fillStyle = '#0e1218'; g.beginPath(); g.arc(99, 99, 23, 0, 7); g.fill(); g.drawImage(si, 82, 82, 34, 34); tex.needsUpdate = true; }; }
  return new THREE.Sprite(new THREE.SpriteMaterial({ map: tex, transparent: true, depthWrite: false }));
}
function build(){
  if(!ok) return; clear(); if(!RB) return;
  const R=RB.route||[]; const stat=document.getElementById('stat');
  if(R.length<2){ stat.textContent='This map has no route polyline to render in 3D.'; return; }
  const hf=heightfield(R);
  const { N,x0,x1,z0,z1,minx,maxx,minz,maxz,miny,maxy,H } = hf;
  const cx=(minx+maxx)/2, cz=(minz+maxz)/2, ext=Math.max(maxx-minx,maxz-minz)||1, hS=340/ext, vS=hS*EXAG, relief=Math.max(0.001,maxy-miny);

  // terrain mesh with a real ground texture (tiled) + normal map
  const REP = Math.max(8, Math.min(36, ext/150));
  const tex = makeGround(themeOf(RB));
  const verts=[],uvs=[],idx=[];
  for(let j=0;j<N;j++)for(let i=0;i<N;i++){ const wx=x0+(x1-x0)*i/(N-1), wz=z0+(z1-z0)*j/(N-1), e=H[j*N+i];
    verts.push((wx-cx)*hS, (e-miny)*vS, (wz-cz)*hS); uvs.push(i/(N-1)*REP, j/(N-1)*REP); }
  for(let j=0;j<N-1;j++)for(let i=0;i<N-1;i++){ const a=j*N+i,b=a+1,c2=a+N,d=c2+1; idx.push(a,c2,b, b,c2,d); }
  const tg=new THREE.BufferGeometry();
  tg.setAttribute('position', new THREE.Float32BufferAttribute(verts,3));
  tg.setAttribute('uv', new THREE.Float32BufferAttribute(uvs,2));
  tg.setIndex(idx); tg.computeVertexNormals();
  group.add(new THREE.Mesh(tg, new THREE.MeshStandardMaterial({ map:tex.map, normalMap:tex.normal, normalScale:new THREE.Vector2(0.8,0.8), roughness:1, metalness:0 })));

  // large base ground plane so a floor always fills the view out to the horizon
  const fmap=tex.map.clone(), fnrm=tex.normal.clone();
  [fmap,fnrm].forEach(t=>{ t.wrapS=t.wrapT=THREE.RepeatWrapping; t.repeat.set(120,120); t.needsUpdate=true; });
  const floor=new THREE.Mesh(new THREE.PlaneGeometry(6000,6000), new THREE.MeshStandardMaterial({ map:fmap, normalMap:fnrm, roughness:1, metalness:0 }));
  floor.rotation.x=-Math.PI/2; floor.position.y=-0.4; group.add(floor);

  // route draped over the terrain
  const off=Math.max(1.2, relief*vS*0.012);
  const v3=p=>new THREE.Vector3((p[0]-cx)*hS, (p[3]-miny)*vS+off, (p[1]-cz)*hS);
  const pts=R.map(v3);
  const tube=new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts,false,'catmullrom',0.1), Math.min(3000,pts.length*2), 1.5, 8, false);
  group.add(new THREE.Mesh(tube, new THREE.MeshStandardMaterial({ color:0x3ec6ff, emissive:0x0b3b58, emissiveIntensity:.7, roughness:.35 })));

  // waypoint markers by distance
  const cum=R.map(p=>p[2]); const totalM=cum[cum.length-1]||1;
  const atD=dM=>{ for(let i=1;i<R.length;i++) if(cum[i]>=dM) return pts[i]; return pts[pts.length-1]; };
  noteGroup=new THREE.Group(); noteGroup.visible=NOTES_ON;
  const SPR=Math.max(18, off*9+14), OFF=SPR*0.95;        // billboard size + height above the route
  (RB.boxes||[]).forEach(b=>{ const p=atD((b.distTotalKm||0)*1000); if(!p)return; let col=0xe9eef6,r=1.6;
    if(b.type==='start'){col=0x1f9d4e;r=3.4;} else if(b.type==='finish'){col=0xcd2620;r=3.4;}
    else if(b.dangerLevel>=2){col=0xcd2620;r=2.5;} else if(b.dangerLevel){col=0xe0902a;r=2.2;} else if(b.type==='hairpin'){col=0xffb028;r=2.3;}
    const m=new THREE.Mesh(new THREE.SphereGeometry(r,14,14), new THREE.MeshStandardMaterial({ color:col, emissive:col, emissiveIntensity:.35, roughness:.4 }));
    m.position.copy(p); group.add(m);
    if(b.type==='turn'||b.type==='hairpin'||b.type==='start'||b.type==='finish'){
      const sp=noteSprite(b); sp.position.set(p.x, p.y+OFF, p.z); sp.scale.set(SPR,SPR,1); noteGroup.add(sp);
      const lg=new THREE.BufferGeometry().setFromPoints([p, new THREE.Vector3(p.x, p.y+OFF-SPR*0.5, p.z)]);
      noteGroup.add(new THREE.Line(lg, new THREE.LineBasicMaterial({ color:0x3ec6ff, transparent:true, opacity:.45 })));
    } });
  group.add(noteGroup);

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
document.getElementById('notesBtn').onclick=e=>{ NOTES_ON=!NOTES_ON; e.target.classList.toggle('on',NOTES_ON); if(noteGroup) noteGroup.visible=NOTES_ON; };

init();
