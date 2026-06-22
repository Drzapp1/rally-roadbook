'use strict';
const PAPER='#f3f1ea', INK='#18120f', DIM='#78726a', LINE='#968c88', CYAN='#28b9eb', PINK='#cd2620';
const PER_PAGE = 12;
let RB = null, page = 0;
const signCache = {};
function totalKm(rb){ const b=(rb&&rb.boxes)||[]; const m=(rb&&rb.meta)||{}; return (rb&&rb.totalDistanceKm)||m.totalDistanceKm||(b.length?b[b.length-1].distTotalKm:0); }
function rbName(rb){ const m=(rb&&rb.meta)||{}; return (rb&&rb.trackName)||m.trackName||(rb&&rb.trackId)||m.trackId||'roadbook'; }
const TH={jump:1,water:1,cliff:1,dune:.6,dunes:.6,rocks:.5,sand:.3,ditch:.5,hole:.6,wadi:.4,compression:.4,bumpy:.2,narrow:.4,collapse:.6};
function difficulty(rb){ const b=(rb&&rb.boxes)||[]; const km=totalKm(rb); if(km<0.1)return ''; let h=0; b.forEach(x=>{h+=(x.dangerLevel||0)+(x.type==='hairpin'?2:0)+(x.pictograms||[]).reduce((s,p)=>s+(TH[p]||0),0);}); const s=h/km; return s<2?'Easy':s<5?'Moderate':s<9?'Hard':'Expert'; }

function sign(code){
  if(signCache[code]!==undefined) return signCache[code];
  const img = new Image(); img.src = 'signs/'+code+'.png';
  img.onload = ()=>render(); signCache[code]=img; return img;
}
function comma(v){ return v.toFixed(2).replace('.',','); }
function rr(c,x,y,w,h,r){ c.beginPath(); c.moveTo(x+r,y); c.arcTo(x+w,y,x+w,y+h,r); c.arcTo(x+w,y+h,x,y+h,r); c.arcTo(x,y+h,x,y,r); c.arcTo(x,y,x+w,y,r); c.closePath(); }

function turnAbbrev(b){ if(b.type==='start')return'S'; if(b.type==='finish')return'F'; if(b.type==='straight')return'kpS'; return b.turnDir==='right'?'R':'L'; }

// stock tulip catalog (x:0..1 L->R, y:0..1 where 1=top, 0=bottom; entry first)
const STOCK={
 start:[[.5,.10],[.5,.84]], straight:[[.5,.08],[.5,.92]],
 slightL:[[.5,.08],[.5,.52],[.30,.88]], slightR:[[.5,.08],[.5,.52],[.70,.88]],
 left:[[.5,.08],[.5,.56],[.14,.56]], right:[[.5,.08],[.5,.56],[.86,.56]],
 sharpL:[[.5,.08],[.5,.60],[.26,.36]], sharpR:[[.5,.08],[.5,.60],[.74,.36]],
 hairpinL:[[.5,.08],[.5,.70],[.30,.80],[.30,.34]], hairpinR:[[.5,.08],[.5,.70],[.70,.80],[.70,.34]],
 finish:[[.5,.08],[.5,.80]]};
function tulipBend(pts){ if(!pts||pts.length<2)return 90; const a=pts[0],b=pts[1],c=pts[pts.length-2],d=pts[pts.length-1];
 const v1=[b[0]-a[0],b[1]-a[1]],v2=[d[0]-c[0],d[1]-c[1]],m1=Math.hypot(v1[0],v1[1])||1,m2=Math.hypot(v2[0],v2[1])||1;
 return Math.acos(Math.max(-1,Math.min(1,(v1[0]*v2[0]+v1[1]*v2[1])/(m1*m2))))*180/Math.PI; }
function tulipCode(b){ const t=b.type, R=(b.turnDir==='right');
 if(t==='start')return'start'; if(t==='finish')return'finish'; if(t==='hairpin')return R?'hairpinR':'hairpinL';
 if(t==='turn'){ const m=tulipBend(b.tulip&&b.tulip.points); if(m<32)return R?'slightR':'slightL'; if(m>=100)return R?'sharpR':'sharpL'; return R?'right':'left'; }
 return 'straight'; }
function drawTulip(c, b, zx, zy, cw, ch){
  const code=tulipCode(b);
  const pts=STOCK[code]||(b.tulip&&b.tulip.points)||[]; if(pts.length<2) return;
  const m=10, uw=cw-2*m, uh=ch-2*m;
  const SX=p=>zx+m+p[0]*uw, SY=p=>zy+m+(1-p[1])*uh;
  c.lineCap='round'; c.lineJoin='round';
  c.strokeStyle=CYAN; c.lineWidth=9; c.beginPath(); c.moveTo(SX(pts[0]),SY(pts[0])); for(let i=1;i<pts.length;i++)c.lineTo(SX(pts[i]),SY(pts[i])); c.stroke();
  c.strokeStyle=INK; c.lineWidth=5; c.beginPath(); c.moveTo(SX(pts[0]),SY(pts[0])); for(let i=1;i<pts.length;i++)c.lineTo(SX(pts[i]),SY(pts[i])); c.stroke();
  const e=pts[0]; c.fillStyle=INK; c.beginPath(); c.arc(SX(e),SY(e),5,0,7); c.fill();
  if(code==='start'){ c.strokeStyle=INK; c.lineWidth=2.5; c.beginPath(); c.arc(SX(e),SY(e),10,0,7); c.stroke(); }
  const a=pts[pts.length-1];
  if(code==='finish'){ const fx=SX(a),fy=SY(a); for(let k=-2;k<3;k++){ c.fillStyle=(k&1)?INK:'#9a9a9a'; c.fillRect(fx+k*7-3,fy-7,8,9);} }
  else { const p=pts[pts.length-2], ax=SX(a),ay=SY(a),dx=ax-SX(p),dy=ay-SY(p),L=Math.hypot(dx,dy)||1,ux=dx/L,uy=dy/L,px=-uy,py=ux,s=13;
    c.fillStyle=INK; c.beginPath(); c.moveTo(ax+ux*s,ay+uy*s); c.lineTo(ax+px*s*0.62,ay+py*s*0.62); c.lineTo(ax-px*s*0.62,ay-py*s*0.62); c.closePath(); c.fill(); }
  if(typeof b.crossDeg==='number' && b.crossDeg>-900){ const r=b.crossDeg*Math.PI/180, cx=zx+cw/2, cy=zy+ch/2, dxc=Math.sin(r),dyc=-Math.cos(r),Ln=uw*0.4;
    c.strokeStyle='rgba(24,18,15,.35)'; c.lineWidth=3; c.beginPath(); c.moveTo(cx-dxc*Ln,cy-dyc*Ln); c.lineTo(cx+dxc*Ln,cy+dyc*Ln); c.stroke(); }
  (b.branchDeg||[]).forEach(bd=>{ const r=bd*Math.PI/180, cx=zx+cw/2, cy=zy+ch/2, Ln=uw*0.42;
    c.strokeStyle='rgba(40,30,24,.45)'; c.lineWidth=3; c.setLineDash([5,3]); c.beginPath(); c.moveTo(cx,cy); c.lineTo(cx+Math.sin(r)*Ln,cy-Math.cos(r)*Ln); c.stroke(); c.setLineDash([]); });
}

// co-driver feature word for a box's pictograms (mirrors the plugin's signClip)
function paceFeature(pgs){
  const map={bump:'crest',jump:'jump',dip:'dip',water:'water',bumpy:'bumpy',rough:'bumpy',ruts:'ruts',compression:'compression',
    downhill:'steep descent',dune:'sand',dunes:'sand',sandpit:'sand',hole:'hole',ditch:'ditch',rocks:'rocks',rocky:'rocks',
    bridge:'bridge',gate:'gate',narrow:'narrows',tree:'tree',village:'village',cliff:'cliff',fence:'fence'};
  for(const p of (pgs||[])){ if(map[p]) return map[p]; }
  return null;
}
// the co-driver call for a box, as text (mirrors the plugin's paceNoteClips order:
// danger -> direction -> distance -> into feature)
function paceNote(b){
  if(b.type==='start') return 'start';
  const w=[];
  if(b.dangerLevel===1)w.push('caution'); else if(b.dangerLevel===2)w.push('danger two'); else if(b.dangerLevel>=3)w.push('danger three');
  if(b.type==='hairpin')w.push('hairpin '+(b.turnDir==='right'?'right':'left'));
  else if(b.type==='turn')w.push(b.turnDir==='right'?'right':'left');
  else if(b.type==='finish')w.push('finish');
  else if(b.type==='straight')w.push('keep straight');
  if(b.type!=='finish'){ const d=Math.round((b.distPartialM||0)/10)*10; if(d>=10)w.push(String(d)); }
  const f=paceFeature(b.pictograms); if(f)w.push('into '+f);
  return w.join(', ')||'—';
}
function drawCase(cv, b, idx){
  const W=600,H=384,CH=350; cv.width=W; cv.height=H; const c=cv.getContext('2d');
  c.fillStyle=PAPER; c.fillRect(0,0,W,H); c.strokeStyle=LINE; c.lineWidth=2; c.strokeRect(1,1,W-2,H-2);
  const lw=W*0.30, cw=W*0.42, zx1=lw, zx2=lw+cw;
  c.beginPath(); c.moveTo(zx1,0); c.lineTo(zx1,CH); c.moveTo(zx2,0); c.lineTo(zx2,CH); c.stroke();
  c.fillStyle=INK; c.textBaseline='alphabetic';
  c.font='bold 52px "Roboto Mono",monospace'; c.fillText(comma(b.distTotalKm),16,68);
  c.strokeStyle=LINE; c.lineWidth=2; rr(c,12,CH-72,lw*0.62,52,5); c.stroke();
  c.font='28px "Roboto Mono",monospace'; c.fillText(b.type==='start'?'0,00':(b.type==='finish'?'END':comma((b.distPartialM||0)/1000)),22,CH-36);
  c.strokeRect(zx1-42,CH-46,36,40); c.font='24px "Roboto Mono",monospace'; c.fillText(idx,zx1-34,CH-18);
  drawTulip(c,b,zx1,4,cw,CH-58);
  // signs row
  const sgs=(b.pictograms||[]).slice(0,4); const sw=Math.min(70,(cw-16)/Math.max(1,sgs.length));
  sgs.forEach((code,i)=>{ const im=sign(code); if(im&&im.complete&&im.naturalWidth) c.drawImage(im, zx1+8+i*sw, CH-70, sw-8, sw-8); });
  // right column
  const rcx=zx2+(W-zx2)/2; c.textAlign='center';
  c.font='bold 52px "Roboto Mono",monospace'; c.fillText(turnAbbrev(b),rcx,72);
  c.font='24px "Roboto Mono",monospace'; c.fillStyle=DIM; c.fillText(String(b.capDeg||0).padStart(3,'0'),rcx,104);
  if(b.dangerLevel>0){ c.fillStyle=PINK; c.font='bold 40px "Roboto Mono",monospace'; c.fillText('!'.repeat(b.dangerLevel),rcx,CH-24); }
  if(b.note){ c.fillStyle=INK; c.font='18px "Roboto Mono",monospace'; c.fillText(b.note.slice(0,14),rcx,150); }
  c.textAlign='left';
  // co-driver pace-note caption strip (learning aid: what the co-driver calls)
  c.strokeStyle=LINE; c.lineWidth=1; c.beginPath(); c.moveTo(4,CH); c.lineTo(W-4,CH); c.stroke();
  c.fillStyle=DIM; c.font='italic 19px "Roboto Mono",monospace'; c.textAlign='center'; c.textBaseline='middle';
  c.fillText('“'+paceNote(b)+'”', W/2, CH+(H-CH)/2, W-16); c.textAlign='left'; c.textBaseline='alphabetic';
}

function drawElev(){
  const cv=document.getElementById('elev'); const W=cv.clientWidth*2,H=cv.clientHeight*2; cv.width=W; cv.height=H;
  const c=cv.getContext('2d'); c.fillStyle='#1c1f26'; c.fillRect(0,0,W,H);
  const rt=(RB&&RB.route)||[]; if(rt.length<2){ return; }
  let lo=1e9,hi=-1e9,tot=rt[rt.length-1][2]||1; rt.forEach(r=>{lo=Math.min(lo,r[3]||0);hi=Math.max(hi,r[3]||0);}); const sp=Math.max(1,hi-lo);
  const PX=d=>20+(d/tot)*(W-40), PY=e=>H-22-((e-lo)/sp)*(H-44);
  c.strokeStyle='#90db78'; c.lineWidth=3; c.beginPath(); c.moveTo(PX(rt[0][2]),PY(rt[0][3]||0)); rt.forEach(r=>c.lineTo(PX(r[2]),PY(r[3]||0))); c.stroke();
  c.fillStyle='#8b919c'; c.font='20px sans-serif'; c.fillText('ELEVATION  '+Math.round(lo)+'–'+Math.round(hi)+' m   ('+(tot/1000).toFixed(2)+' km)',22,28);
  (RB.boxes||[]).forEach(b=>{ const d=b.distTotalKm*1000; c.fillStyle=b.dangerLevel>0?'#ef5a46':'#ffc83c'; c.fillRect(PX(d)-2,PY((function(){let e=0;for(const r of rt){if(r[2]>=d){e=r[3]||0;break;}}return e;})())-3,4,6); });
}

function drawRouteMap(){
  const cv=document.getElementById('routemap'); if(!cv)return;
  const W=cv.clientWidth*2,H=cv.clientHeight*2; cv.width=W; cv.height=H;
  const c=cv.getContext('2d'); c.fillStyle='#1c1f26'; c.fillRect(0,0,W,H);
  const rt=(RB&&RB.route)||[]; if(rt.length<2)return;
  let minx=1e9,maxx=-1e9,minz=1e9,maxz=-1e9;
  rt.forEach(r=>{minx=Math.min(minx,r[0]);maxx=Math.max(maxx,r[0]);minz=Math.min(minz,r[1]);maxz=Math.max(maxz,r[1]);});
  const m=12,s=Math.min((W-2*m)/Math.max(1,maxx-minx),(H-2*m)/Math.max(1,maxz-minz));
  const ox=(W-(maxx-minx)*s)/2,oz=(H-(maxz-minz)*s)/2;
  const PX=x=>ox+(x-minx)*s,PY=z=>H-(oz+(z-minz)*s);
  c.strokeStyle='#28b9eb';c.lineWidth=2;c.lineJoin='round';c.beginPath();c.moveTo(PX(rt[0][0]),PY(rt[0][1]));rt.forEach(r=>c.lineTo(PX(r[0]),PY(r[1])));c.stroke();
  (RB.boxes||[]).forEach(b=>{ if(b.dangerLevel>0){ const d=b.distTotalKm*1000; let bx=rt[rt.length-1][0],bz=rt[rt.length-1][1]; for(const r of rt){if(r[2]>=d){bx=r[0];bz=r[1];break;}} c.fillStyle='#ffc83c';c.fillRect(PX(bx)-1.5,PY(bz)-1.5,3,3);} });
  c.fillStyle='#1f9d4e';c.beginPath();c.arc(PX(rt[0][0]),PY(rt[0][1]),4,0,7);c.fill();
  c.fillStyle='#cd2620';c.beginPath();c.arc(PX(rt[rt.length-1][0]),PY(rt[rt.length-1][1]),4,0,7);c.fill();
}
function render(){
  const grid=document.getElementById('grid'); grid.innerHTML='';
  if(!RB){ return; }
  const n=RB.boxes.length, pages=Math.max(1,Math.ceil(n/PER_PAGE)); page=Math.max(0,Math.min(page,pages-1));
  document.getElementById('pageLbl').textContent='page '+(page+1)+' / '+pages;
  document.getElementById('meta').innerHTML='<b>'+rbName(RB)+'</b> — '+n+' boxes, '+totalKm(RB).toFixed(2)+' km · <span style="color:var(--accent)">'+difficulty(RB)+'</span>';
  for(let i=page*PER_PAGE;i<Math.min(n,(page+1)*PER_PAGE);i++){ const cv=document.createElement('canvas'); grid.appendChild(cv); drawCase(cv,RB.boxes[i],i); }
  drawElev(); drawRouteMap();
}

function loadRB(obj){ RB=obj; page=0; render(); document.getElementById('tabViewer').click(); }
function loadFile(f){ const r=new FileReader(); r.onload=()=>{ try{ loadRB(JSON.parse(r.result)); }catch(e){ alert('Invalid roadbook JSON'); } }; r.readAsText(f); }

document.getElementById('file').addEventListener('change',e=>{ if(e.target.files[0])loadFile(e.target.files[0]); });
document.body.addEventListener('dragover',e=>e.preventDefault());
document.body.addEventListener('drop',e=>{ e.preventDefault(); if(e.dataTransfer.files[0])loadFile(e.dataTransfer.files[0]); });
document.getElementById('prev').onclick=()=>{ page--; render(); };
document.getElementById('next').onclick=()=>{ page++; render(); };

function tab(id){ ['viewer','library','hub','legend'].forEach(s=>document.getElementById(s).classList.toggle('hide',s!==id));
  ['Viewer','Library','Hub','Legend'].forEach(t=>document.getElementById('tab'+t).classList.toggle('on','tab'+t==='tab'+id.charAt(0).toUpperCase()+id.slice(1))); }
document.getElementById('tabViewer').onclick=()=>tab('viewer');
document.getElementById('tabLibrary').onclick=()=>tab('library');
document.getElementById('tabHub').onclick=()=>tab('hub');
document.getElementById('tabLegend').onclick=()=>tab('legend');

const SIGN_CATS=[
 ['Direction',['keepL','keepR','keepS','left','right','onL','onR','hairpin','roundaboutL','roundaboutR','keepMain']],
 ['Danger',['caution','danger2','danger3','cutL','cutR','crestL','crestR','downhill','inclineL','inclineR','slowdown','deadend','lessVisible','noentry','stop']],
 ['Terrain',['bump','jump','dip','ditch','compression','hole','bumpy','ruts','rough','dune','dunes','sandpit','camelgrass','rocky','rocks','chott','mountain','hill','cliff','collapse','narrow']],
 ['Water',['water','river','lake','pond','dryriver','wadi','bigwadi','canal']],
 ['Man-made',['fence','gate','bridge','pole','antenna','pipeline','railroad','wall','barbwire','cattleguard','gatebar','hvline','pump','mine','cairn','wellpost','roadworks']],
 ['Scenery',['tree','bush','village','house','church','cemetery','monument','ruins','castle','camp','animals']],
 ['Service',['fuel','fuelzone','checkpoint','mediapt','photo','spectators','police','medical','helicopter','info','north']]];
const NAMES={keepL:'Keep left',keepR:'Keep right',keepS:'Keep straight',onL:'On the left',onR:'On the right',hairpin:'Hairpin',roundaboutL:'Roundabout L',roundaboutR:'Roundabout R',keepMain:'Keep on main track',
 caution:'Caution',danger2:'Danger 2',danger3:'Danger 3',cutL:'Cut left',cutR:'Cut right',crestL:'Crest left',crestR:'Crest right',downhill:'Steep descent',inclineL:'Camber left',inclineR:'Camber right',slowdown:'Slow down',deadend:'Dead end',lessVisible:'Less visible',noentry:'No entry / off-track',
 compression:'Compression',jump:'Jump (kicker)',bumpy:'Bumpy',dryriver:'Dry river',bigwadi:'Big wadi',barbwire:'Barbed wire',cattleguard:'Cattle guard',gatebar:'Gate / barrier',hvline:'High-voltage line',pump:'Pumpjack',wellpost:'Marker post',roadworks:'Roadworks',
 fuelzone:'Fuel zone',checkpoint:'Checkpoint',mediapt:'Media point',camelgrass:'Camel grass',sandpit:'Sand pit',chott:'Salt flat / chott',wadi:'Wadi',canal:'Canal / aqueduct'};
const signName=c=>NAMES[c]||c.charAt(0).toUpperCase()+c.slice(1);
function buildLegend(){
  const el=document.getElementById('legendBody'); el.innerHTML='';
  SIGN_CATS.forEach(([group,codes])=>{
    const h=document.createElement('h2'); h.textContent=group; h.style.cssText='margin:14px 0 6px;color:var(--accent);font-size:13px;text-transform:uppercase;letter-spacing:1px'; el.appendChild(h);
    const g=document.createElement('div'); g.style.cssText='display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px';
    codes.forEach(code=>{ const d=document.createElement('div'); d.style.cssText='display:flex;align-items:center;gap:9px;background:var(--panel);border:1px solid var(--line);border-radius:6px;padding:7px 9px';
      d.innerHTML='<img src="signs/'+code+'.png" style="width:26px;height:26px;object-fit:contain;filter:invert(.9)" onerror="this.style.opacity=0"><span style="font-size:12px">'+signName(code)+'</span>';
      g.appendChild(d); });
    el.appendChild(g); });
}

const LIBRARY=[
  ['Real rally stages',['rally_empty_quarter','rally_merzouga','rally_erzberg','rally_baja_sanfelipe','rally_finke','rally_hellas_crete']],
  ['Tracks',['alps','almeria','stage01','KBOLTZ_DUNED_II','MOTOBROS-PUNTA-GORDA','union_demo']],
  ['GPX stages',['stage_desert_loop','stage_mountain_pass','stage_coastal_sprint','stage_canyon_run','stage_river_delta','stage_high_plateau','stage_forest_track','stage_big_dunes']],
  ['Practice drills',['drill_junctions','drill_hairpins','drill_crest_dip','drill_slalom','drill_mixed','drill_esses','drill_technical','drill_long_mixed']],
];
let MANIFEST=null;
function manifestGroups(){ const g={}; (MANIFEST||[]).forEach(it=>{ (g[it.group]=g[it.group]||[]).push(it); }); return g; }
const GRPHEAD='grid-column:1/-1;color:var(--accent);font-size:11px;text-transform:uppercase;letter-spacing:1px;margin:8px 0 0';
function buildLibrary(){
  const el=document.getElementById('libCards'); el.innerHTML='';
  if(MANIFEST){
    const g=manifestGroups();
    Object.keys(g).forEach(group=>{
      const h=document.createElement('div'); h.style.cssText=GRPHEAD; h.textContent=group+' ('+g[group].length+')'; el.appendChild(h);
      g[group].forEach(it=>{ const card=document.createElement('div'); card.className='card';
        card.innerHTML='<h3>'+it.name+'</h3><div class="sub">'+group+' · '+it.km.toFixed(1)+' km · '+it.difficulty+'</div>';
        const b=document.createElement('button'); b.textContent='Open'; b.onclick=()=>fetch(it.file).then(r=>r.json()).then(loadRB).catch(()=>alert('Run from a local web server — file:// blocks fetch.'));
        card.appendChild(b); el.appendChild(card); });
    });
    return;
  }
  LIBRARY.forEach(([group,names])=>{
    const h=document.createElement('div'); h.style.cssText=GRPHEAD; h.textContent=group; el.appendChild(h);
    names.forEach(name=>{ const card=document.createElement('div'); card.className='card';
      card.innerHTML='<h3>'+name.replace(/_/g,' ')+'</h3><div class="sub">'+group+'</div>';
      fetch('roadbooks/roadbook_'+name+'.json').then(r=>r.json()).then(rb=>{ const sub=card.querySelector('.sub'); if(sub)sub.textContent=group+' · '+totalKm(rb).toFixed(1)+' km · '+difficulty(rb); }).catch(()=>{});
      const b=document.createElement('button'); b.textContent='Open'; b.onclick=()=>{ fetch('roadbooks/roadbook_'+name+'.json').then(r=>r.json()).then(loadRB).catch(()=>alert('Run from a local web server (or use the file loader) — file:// blocks fetch.')); };
      card.appendChild(b); el.appendChild(card); });
  });
}
function buildHub(){
  const el=document.getElementById('hubCards'); el.innerHTML='';
  const items = MANIFEST ? MANIFEST.map(it=>({name:it.name,file:it.file,dl:it.id,sub:it.group+' · '+it.km.toFixed(1)+' km · '+it.difficulty}))
                         : LIBRARY.flatMap(([g,names])=>names.map(n=>({name:n.replace(/_/g,' '),file:'roadbooks/roadbook_'+n+'.json',dl:n,sub:g})));
  items.forEach(it=>{ const c=document.createElement('div'); c.className='card';
    c.innerHTML='<h3>'+it.name+'</h3><div class="sub">'+it.sub+'</div>';
    if(!MANIFEST) fetch(it.file).then(r=>r.json()).then(rb=>{ const sub=c.querySelector('.sub'); if(sub)sub.textContent=totalKm(rb).toFixed(1)+' km · '+difficulty(rb); }).catch(()=>{});
    const b=document.createElement('button'); b.textContent='Download .json'; b.onclick=()=>{ fetch(it.file).then(r=>r.blob()).then(blob=>{ const a=document.createElement('a'); a.href=URL.createObjectURL(blob); a.download='roadbook_'+it.dl+'.json'; a.click(); }).catch(()=>alert('Run from a local web server to download.')); };
    c.appendChild(b); el.appendChild(c); });
}
fetch('roadbooks/manifest.json').then(r=>r.ok?r.json():null).then(m=>{ MANIFEST=m; }).catch(()=>{}).finally(()=>{ buildLibrary(); buildHub(); buildLegend(); });
// ---- theme switcher (chrome only; the roadbook cases stay authentic paper) ----
const THEMES={
  midnight:{'--bg':'#14161b','--panel':'#1c1f26','--ink':'#e6e8ec','--dim':'#8b919c','--accent':'#ffb028','--line':'#2a2e37'},
  paper:{'--bg':'#ded9cc','--panel':'#efece3','--ink':'#1a1612','--dim':'#6b6356','--accent':'#c0392b','--line':'#cbc4b6'},
  dakar:{'--bg':'#17110a','--panel':'#241a0c','--ink':'#f0e2c0','--dim':'#b09a6a','--accent':'#ff9e1b','--line':'#3a2c14'},
  blueprint:{'--bg':'#0b1422','--panel':'#102234','--ink':'#cfe6ff','--dim':'#6f93b8','--accent':'#34b3eb','--line':'#1d3550'}
};
function applyTheme(name){ const t=THEMES[name]||THEMES.midnight; for(const k in t) document.documentElement.style.setProperty(k,t[k]); try{localStorage.setItem('rbTheme',name);}catch(e){} }
(function(){ let saved='midnight'; try{saved=localStorage.getItem('rbTheme')||'midnight';}catch(e){} const sel=document.getElementById('theme'); if(sel){ sel.value=saved; sel.onchange=()=>applyTheme(sel.value); } applyTheme(saved); })();
// builder hand-off (?preview=1 -> localStorage) or a demo roadbook so the viewer isn't empty
let _pv=null; if(location.search.indexOf('preview=1')>=0){ try{ _pv=JSON.parse(localStorage.getItem('rb_preview')||'null'); }catch(e){} }
if(_pv){ loadRB(_pv); } else loadRB({trackName:'Demo loop',totalDistanceKm:0.46,route:[[0,0,0,100],[0,200,200,108],[150,250,460,96]],boxes:[
 {distTotalKm:0,distPartialM:0,capDeg:23,type:'start',turnDir:'none',dangerLevel:0,pictograms:[],tulip:{points:[[0.5,0.05],[0.5,0.95]]}},
 {distTotalKm:0.20,distPartialM:200,capDeg:38,type:'turn',turnDir:'right',dangerLevel:1,pictograms:['caution'],tulip:{points:[[0.5,0.9],[0.5,0.5],[0.84,0.28]]}},
 {distTotalKm:0.45,distPartialM:159,capDeg:180,type:'hairpin',turnDir:'right',dangerLevel:2,pictograms:['water','bumpy'],tulip:{points:[[0.4,0.9],[0.4,0.45],[0.6,0.3],[0.66,0.1]]}},
 {distTotalKm:0.46,distPartialM:3,capDeg:180,type:'finish',turnDir:'none',dangerLevel:0,pictograms:[],tulip:{points:[[0.5,0.05],[0.5,0.95]]}}]});
