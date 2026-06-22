'use strict';
// ---------------------------------------------------------------------------
// Route builder: draw a path on the canvas -> auto-generate a rally roadbook
// (turns classified into the same stock tulips the viewer + plugin use).
// ---------------------------------------------------------------------------
const PAPER='#f3f1ea', INK='#18120f', LINE='#968c88', CYAN='#28b9eb', GREEN='#1f9d4e', PINK='#cd2620';

// stock tulip catalog + classifier (shared shape vocabulary with the viewer)
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
const TH={jump:1,water:1,cliff:1,dune:.6,dunes:.6,rocks:.5,sand:.3,ditch:.5,hole:.6,wadi:.4,compression:.4,bumpy:.2,narrow:.4,collapse:.6};
function difficulty(rb){ const b=rb.boxes||[]; const km=b.length?b[b.length-1].distTotalKm:0; if(km<0.1)return ''; let h=0;
 b.forEach(x=>{h+=(x.dangerLevel||0)+(x.type==='hairpin'?2:0)+(x.pictograms||[]).reduce((s,p)=>s+(TH[p]||0),0);}); const s=h/km;
 return s<2?'Easy':s<5?'Moderate':s<9?'Hard':'Expert'; }

// ---------------------------------------------------------------------------
// generator: world path (metres, z=north) -> roadbook boxes
// ---------------------------------------------------------------------------
function compass(dx,dz){ return (Math.atan2(dx,dz)*180/Math.PI+360)%360; } // 0=N(+z) 90=E(+x)
const MAXSP=600;  // force a "keep straight" box at least this often

function genRoadbook(world, name){
  const n=world.length, nm=name||'New route';
  if(n<2) return {schemaVersion:1,trackName:nm,meta:{trackName:nm,totalDistanceKm:0},boxes:[],route:[]};
  const seg=[]; const cum=[0];
  for(let i=1;i<n;i++){ const dx=world[i][0]-world[i-1][0], dz=world[i][1]-world[i-1][1]; const len=Math.hypot(dx,dz);
    seg.push({dx,dz,len,h:compass(dx,dz)}); cum[i]=cum[i-1]+len; }
  const total=cum[n-1];
  const headAt=dM=>{ for(let i=1;i<n;i++){ if(cum[i]>=dM) return seg[i-1].h; } return seg[seg.length-1].h; };
  const mk=(distM,type,dir,cap,turnDeg,danger)=>{
    const t=[[0.5,0.10],[0.5,0.50]];
    if(type==='start'||type==='finish'||type==='straight') t[1]=[0.5,0.90];
    else { const a=turnDeg*Math.PI/180; t.push([0.5+Math.sin(a)*0.4, 0.5+Math.cos(a)*0.4]); }
    return {index:0,distTotalKm:distM/1000,distPartialM:0,capDeg:Math.round(cap)%360,type,turnDir:dir,
            dangerLevel:danger,pictograms:[],note:'',tulip:{points:t,entry:t[0],exit:{at:t[t.length-1],headingDeg:Math.round(cap)%360}}};
  };
  let boxes=[ mk(0,'start','none',seg[0].h,0,0) ];
  for(let i=1;i<n-1;i++){
    const turn=((seg[i].h-seg[i-1].h+540)%360)-180, mag=Math.abs(turn);
    if(mag<15) continue;
    boxes.push(mk(cum[i], mag>120?'hairpin':'turn', turn>0?'right':'left', seg[i].h, turn, mag>140?2:mag>95?1:0));
  }
  boxes.push(mk(total,'finish','none',seg[n-2].h,0,0));
  // insert keep-straight boxes on long gaps
  const out=[]; for(let k=0;k<boxes.length;k++){
    if(k>0){ const a=boxes[k-1].distTotalKm*1000, b=boxes[k].distTotalKm*1000, gap=b-a;
      if(gap>MAXSP){ const c=Math.floor(gap/MAXSP); for(let j=1;j<=c;j++){ const dM=a+gap*j/(c+1); out.push(mk(dM,'straight','none',headAt(dM),0,0)); } } }
    out.push(boxes[k]);
  }
  out.forEach((b,k)=>{ b.index=k; b.distPartialM = k? Math.round((b.distTotalKm-out[k-1].distTotalKm)*1000):0; });
  return {schemaVersion:1,trackName:nm,meta:{trackName:nm,totalDistanceKm:total/1000},totalDistanceKm:total/1000,
          boxes:out, route:world.map(p=>[p[0],0,p[1],0])};
}

// ---------------------------------------------------------------------------
// canvas: draw the path + interact
// ---------------------------------------------------------------------------
const cv=document.getElementById('cv'), ctx=cv.getContext('2d');
let pts=[];           // canvas-pixel waypoints
let drag=-1, mPerPx=8; // scale (metres per pixel)
let RB=null;

function worldOf(){ return pts.map(p=>[p[0]*mPerPx, -p[1]*mPerPx]); }   // canvas y down -> z north up
function regen(){ RB=genRoadbook(worldOf(), document.getElementById('name').value); draw(); summary(); }

function draw(){
  const W=cv.width,H=cv.height; ctx.clearRect(0,0,W,H);
  ctx.fillStyle='#11141a'; ctx.fillRect(0,0,W,H);
  ctx.strokeStyle='#1b2030'; ctx.lineWidth=1;
  for(let x=0;x<W;x+=40){ ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,H); ctx.stroke(); }
  for(let y=0;y<H;y+=40){ ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke(); }
  if(pts.length>=2){
    ctx.lineCap='round'; ctx.lineJoin='round';
    ctx.strokeStyle=CYAN; ctx.lineWidth=6; ctx.beginPath(); ctx.moveTo(pts[0][0],pts[0][1]); for(let i=1;i<pts.length;i++)ctx.lineTo(pts[i][0],pts[i][1]); ctx.stroke();
    ctx.strokeStyle='#dfe3ea'; ctx.lineWidth=2.5; ctx.beginPath(); ctx.moveTo(pts[0][0],pts[0][1]); for(let i=1;i<pts.length;i++)ctx.lineTo(pts[i][0],pts[i][1]); ctx.stroke();
  }
  // mark generated turn boxes on the path
  if(RB){ const wl=RB.route.length; RB.boxes.forEach(b=>{
    if(b.type==='start'||b.type==='finish'||b.type==='straight') return;
    // place marker at the matching waypoint by distance fraction
    const frac=b.distTotalKm/(RB.totalDistanceKm||1); const idx=Math.round(frac*(pts.length-1));
    const p=pts[Math.max(0,Math.min(pts.length-1,idx))]; if(!p) return;
    ctx.fillStyle = b.dangerLevel>=2?PINK : b.dangerLevel?'#e0902a' : '#cfd3da';
    ctx.beginPath(); ctx.arc(p[0],p[1],5,0,7); ctx.fill();
  }); }
  // waypoint handles + start/finish
  pts.forEach((p,i)=>{ ctx.beginPath(); ctx.arc(p[0],p[1], i===0||i===pts.length-1?7:4, 0,7);
    ctx.fillStyle = i===0?GREEN : i===pts.length-1?PINK : '#9aa0aa'; ctx.fill();
    ctx.lineWidth=2; ctx.strokeStyle='#0c0f14'; ctx.stroke(); });
}

function pos(e){ const r=cv.getBoundingClientRect(); return [(e.clientX-r.left)*cv.width/r.width, (e.clientY-r.top)*cv.height/r.height]; }
function hit(p){ for(let i=0;i<pts.length;i++) if(Math.hypot(pts[i][0]-p[0],pts[i][1]-p[1])<10) return i; return -1; }
cv.addEventListener('mousedown',e=>{ const p=pos(e); const h=hit(p); if(h>=0){ drag=h; } else { pts.push(p); regen(); } });
window.addEventListener('mousemove',e=>{ if(drag<0)return; pts[drag]=pos(e); regen(); });
window.addEventListener('mouseup',()=>drag=-1);
cv.addEventListener('dblclick',e=>{ const h=hit(pos(e)); if(h>=0){ pts.splice(h,1); regen(); } });

// ---------------------------------------------------------------------------
// roadbook preview strip (stock tulips) + summary
// ---------------------------------------------------------------------------
function miniTulip(c, b, x, y, w, h){
  const pts=STOCK[tulipCode(b)]||[[.5,.08],[.5,.92]]; const m=6, SX=p=>x+m+p[0]*(w-2*m), SY=p=>y+m+(1-p[1])*(h-2*m);
  c.lineCap='round'; c.lineJoin='round';
  c.strokeStyle=CYAN; c.lineWidth=5; c.beginPath(); c.moveTo(SX(pts[0]),SY(pts[0])); for(let i=1;i<pts.length;i++)c.lineTo(SX(pts[i]),SY(pts[i])); c.stroke();
  c.strokeStyle=INK; c.lineWidth=2.5; c.beginPath(); c.moveTo(SX(pts[0]),SY(pts[0])); for(let i=1;i<pts.length;i++)c.lineTo(SX(pts[i]),SY(pts[i])); c.stroke();
  c.fillStyle=INK; c.beginPath(); c.arc(SX(pts[0]),SY(pts[0]),3,0,7); c.fill();
  const a=pts[pts.length-1], code=tulipCode(b);
  if(code==='finish'){ const fx=SX(a),fy=SY(a); for(let k=-2;k<3;k++){ c.fillStyle=(k&1)?INK:'#9a9a9a'; c.fillRect(fx+k*5-2,fy-5,6,7);} }
  else { const p=pts[pts.length-2],ax=SX(a),ay=SY(a),dx=ax-SX(p),dy=ay-SY(p),L=Math.hypot(dx,dy)||1,ux=dx/L,uy=dy/L,px=-uy,py=ux,s=8;
    c.fillStyle=INK; c.beginPath(); c.moveTo(ax+ux*s,ay+uy*s); c.lineTo(ax+px*s*.6,ay+py*s*.6); c.lineTo(ax-px*s*.6,ay-py*s*.6); c.closePath(); c.fill(); }
}
const AB={start:'',straight:'S',slightL:'L',slightR:'R',left:'L',right:'R',sharpL:'L',sharpR:'R',hairpinL:'L',hairpinR:'R',finish:'F'};
function summary(){
  const km=(RB&&RB.totalDistanceKm)||0, nb=(RB&&RB.boxes.length)||0;
  document.getElementById('stat').innerHTML = pts.length<2
    ? 'Click on the canvas to drop waypoints and draw a route.'
    : `<b>${RB.trackName||'route'}</b> — ${km.toFixed(2)} km · ${nb} boxes · <span style="color:var(--accent)">${difficulty(RB)||'—'}</span>`;
  const strip=document.getElementById('strip'); strip.innerHTML='';
  if(!RB) return;
  RB.boxes.forEach(b=>{ const cell=document.createElement('canvas'); cell.width=86; cell.height=120; cell.className='cell';
    const c=cell.getContext('2d'); c.fillStyle=PAPER; c.fillRect(0,0,86,120); c.strokeStyle=LINE; c.strokeRect(.5,.5,85,119);
    c.fillStyle=INK; c.font='bold 12px monospace'; c.fillText(b.type==='start'?'0,00':(b.distTotalKm).toFixed(2).replace('.',','),5,15);
    const code=tulipCode(b); c.font='bold 16px monospace'; c.fillText(AB[code]||'',66,16);
    c.font='9px monospace'; c.fillStyle='#888'; c.fillText(String(b.capDeg).padStart(3,'0'),60,28);
    if(b.dangerLevel){ c.fillStyle=PINK; c.font='bold 12px monospace'; c.fillText('!'.repeat(b.dangerLevel),60,112); }
    miniTulip(c,b,8,20,52,95); strip.appendChild(cell); });
}

// ---------------------------------------------------------------------------
// controls
// ---------------------------------------------------------------------------
document.getElementById('name').oninput=()=>regen();
document.getElementById('scale').oninput=e=>{ mPerPx=+e.target.value; document.getElementById('scaleOut').textContent=mPerPx+' m/px'; regen(); };
document.getElementById('clear').onclick=()=>{ pts=[]; regen(); };
document.getElementById('undo').onclick=()=>{ pts.pop(); regen(); };
document.getElementById('save').onclick=()=>{ if(!RB||RB.boxes.length<2){alert('Draw a route first.');return;}
  const blob=new Blob([JSON.stringify(RB,null,1)],{type:'application/json'}); const a=document.createElement('a');
  a.href=URL.createObjectURL(blob); a.download=(RB.trackName||'route').replace(/\s+/g,'_')+'.json'; a.click(); };
document.getElementById('view').onclick=()=>{ if(!RB||RB.boxes.length<2){alert('Draw a route first.');return;}
  try{ localStorage.setItem('rb_preview', JSON.stringify(RB)); }catch(e){}
  location.href='index.html?preview=1'; };

regen();
