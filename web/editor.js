'use strict';
const PAPER='#f3f1ea', INK='#18120f', LINE='#968c88', CYAN='#28b9eb', GREEN='#1f9d4e';
// sign codes grouped to match the plugin's categorized picker
const SIGN_CATS = [
 ['Direction',['keepL','keepR','keepS','left','right','onL','onR','hairpin','roundaboutL','roundaboutR','keepMain']],
 ['Danger',['caution','danger2','danger3','cutL','cutR','crestL','crestR','downhill','inclineL','inclineR','slowdown','deadend','lessVisible','noentry','stop']],
 ['Terrain',['bump','jump','dip','ditch','compression','hole','bumpy','ruts','rough','dune','dunes','sandpit','camelgrass','rocky','rocks','chott','mountain','hill','cliff','collapse','narrow']],
 ['Water',['water','river','lake','pond','dryriver','wadi','bigwadi','canal']],
 ['Man-made',['fence','gate','bridge','pole','antenna','pipeline','railroad','wall','barbwire','cattleguard','gatebar','hvline','pump','mine','cairn','wellpost','roadworks']],
 ['Scenery',['tree','bush','village','house','church','cemetery','monument','ruins','castle','camp','animals']],
 ['Service',['fuel','fuelzone','checkpoint','mediapt','photo','spectators','police','medical','helicopter','info','north']],
];
const TYPES=['start','straight','turn','hairpin','finish'];
let RB=null, sel=0, drag=-1;
const cv=document.getElementById('cv'), ctx=cv.getContext('2d');
const M=40, W=cv.width, H=cv.height;
const SX=p=>M+p[0]*(W-2*M), SY=p=>M+(1-p[1])*(H-2*M);
const IX=x=>Math.max(0,Math.min(1,(x-M)/(W-2*M))), IY=y=>Math.max(0,Math.min(1,1-(y-M)/(H-2*M)));

function blankBox(i,type){ return {index:i,distTotalKm:0,distPartialM:0,capDeg:0,type:type||'turn',turnDir:'none',dangerLevel:0,note:'',pictograms:[],tulip:{points:[[0.5,0.08],[0.5,0.92]],entry:[0.5,0.92],exit:{at:[0.5,0.08],headingDeg:0}}}; }
function newBook(){ RB={schemaVersion:1,trackName:'New roadbook',generatedBy:'editor',boxes:[blankBox(0,'start'),blankBox(1,'finish')],route:[]}; sel=0; renderAll(); }

function curBox(){ return RB && RB.boxes[sel]; }

function drawCanvas(){
  ctx.fillStyle=PAPER; ctx.fillRect(0,0,W,H);
  ctx.strokeStyle='#dcd8cf'; ctx.lineWidth=1;
  for(let g=0;g<=10;g++){ const x=M+g/10*(W-2*M),y=M+g/10*(H-2*M); ctx.beginPath();ctx.moveTo(x,M);ctx.lineTo(x,H-M);ctx.moveTo(M,y);ctx.lineTo(W-M,y);ctx.stroke(); }
  const b=curBox(); if(!b) return; const pts=b.tulip.points;
  ctx.lineCap='round'; ctx.lineJoin='round';
  ctx.strokeStyle=CYAN; ctx.lineWidth=12; stroke(pts);
  ctx.strokeStyle=INK; ctx.lineWidth=7; stroke(pts);
  // arrow at exit (last)
  if(pts.length>=2){ const a=pts[pts.length-1],p=pts[pts.length-2],ax=SX(a),ay=SY(a),dx=ax-SX(p),dy=ay-SY(p),L=Math.hypot(dx,dy)||1,ux=dx/L,uy=dy/L,px=-uy,py=ux,s=18;
    ctx.fillStyle=INK; ctx.beginPath(); ctx.moveTo(ax+ux*s,ay+uy*s); ctx.lineTo(ax+px*s*.6,ay+py*s*.6); ctx.lineTo(ax-px*s*.6,ay-py*s*.6); ctx.closePath(); ctx.fill(); }
  // handles
  pts.forEach((p,i)=>{ ctx.beginPath(); ctx.arc(SX(p),SY(p),i===0?8:6,0,7); ctx.fillStyle=i===0?GREEN:'#c9462f'; ctx.fill(); ctx.lineWidth=2; ctx.strokeStyle='#fff'; ctx.stroke(); });
}
function stroke(pts){ ctx.beginPath(); ctx.moveTo(SX(pts[0]),SY(pts[0])); for(let i=1;i<pts.length;i++)ctx.lineTo(SX(pts[i]),SY(pts[i])); ctx.stroke(); }

function hitPoint(mx,my){ const pts=curBox().tulip.points; for(let i=0;i<pts.length;i++){ if(Math.hypot(SX(pts[i])-mx,SY(pts[i])-my)<11) return i; } return -1; }
function nearestSeg(mx,my){ const pts=curBox().tulip.points; let bi=pts.length-1,bd=1e9; for(let i=0;i<pts.length-1;i++){ const ax=SX(pts[i]),ay=SY(pts[i]),bx=SX(pts[i+1]),by=SY(pts[i+1]); const t=Math.max(0,Math.min(1,((mx-ax)*(bx-ax)+(my-ay)*(by-ay))/((bx-ax)**2+(by-ay)**2||1))); const d=Math.hypot(ax+t*(bx-ax)-mx,ay+t*(by-ay)-my); if(d<bd){bd=d;bi=i+1;} } return bi; }
function pos(e){ const r=cv.getBoundingClientRect(); return [(e.clientX-r.left)*W/r.width,(e.clientY-r.top)*H/r.height]; }
cv.addEventListener('mousedown',e=>{ const[mx,my]=pos(e); const h=hitPoint(mx,my); if(h>=0){ drag=h; } else { const idx=nearestSeg(mx,my); curBox().tulip.points.splice(idx,0,[IX(mx),IY(my)]); syncTulip(); drawCanvas(); } });
window.addEventListener('mousemove',e=>{ if(drag<0)return; const[mx,my]=pos(e); curBox().tulip.points[drag]=[IX(mx),IY(my)]; syncTulip(); drawCanvas(); });
window.addEventListener('mouseup',()=>drag=-1);
cv.addEventListener('dblclick',e=>{ const[mx,my]=pos(e); const h=hitPoint(mx,my); const pts=curBox().tulip.points; if(h>=0&&pts.length>2){ pts.splice(h,1); syncTulip(); drawCanvas(); } });
function syncTulip(){ const b=curBox(),pts=b.tulip.points; b.tulip.entry=pts[0].slice(); const a=pts[pts.length-1],p=pts[pts.length-2]||pts[0]; b.tulip.exit={at:a.slice(),headingDeg:Math.round((Math.atan2(a[0]-p[0],a[1]-p[1])*180/Math.PI+360))%360}; }

function renderList(){
  const el=document.getElementById('boxlist'); el.innerHTML='';
  RB.boxes.forEach((b,i)=>{ const d=document.createElement('div'); d.className='boxrow'+(i===sel?' sel':'');
    d.innerHTML=`<span class="n">${i}</span><span class="t">${b.type}${b.turnDir!=='none'?' '+b.turnDir:''} · ${b.distTotalKm.toFixed(2)}</span>`;
    d.onclick=()=>{ sel=i; renderAll(); }; el.appendChild(d); });
}
function field(label,val,type,onch,opts){
  const l=document.createElement('label'); l.textContent=label; let inp;
  if(type==='select'){ inp=document.createElement('select'); opts.forEach(o=>{const op=document.createElement('option');op.value=o;op.textContent=o;inp.appendChild(op);}); inp.value=val; }
  else { inp=document.createElement('input'); inp.type=type; inp.value=val; if(type==='number')inp.step='any'; }
  inp.onchange=()=>onch(type==='number'?parseFloat(inp.value)||0:inp.value);
  return [l,inp];
}
function renderFields(){
  const el=document.getElementById('fields'); el.innerHTML=''; const b=curBox(); if(!b)return;
  const add=(...xs)=>xs.forEach(x=>el.appendChild(x));
  add(...field('Total km',b.distTotalKm,'number',v=>{b.distTotalKm=v;renderList();}));
  add(...field('Partial m',b.distPartialM,'number',v=>b.distPartialM=v));
  add(...field('Type',b.type,'select',v=>{b.type=v;renderList();drawCanvas();},TYPES));
  add(...field('Turn',b.turnDir,'select',v=>{b.turnDir=v;renderList();},['none','left','right']));
  add(...field('Danger',b.dangerLevel,'select',v=>b.dangerLevel=parseInt(v),[0,1,2,3]));
  add(...field('CAP°',b.capDeg,'number',v=>b.capDeg=Math.round(v)));
  add(...field('Note',b.note||'','text',v=>b.note=v));
}
function renderSigns(){
  const el=document.getElementById('signs'); el.innerHTML=''; const b=curBox(); if(!b)return;
  SIGN_CATS.forEach(([name,codes])=>{ const h=document.createElement('div'); h.className='catname'; h.textContent=name; el.appendChild(h);
    const g=document.createElement('div'); g.className='signs';
    codes.forEach(code=>{ const d=document.createElement('div'); d.className='sgn'+(b.pictograms.includes(code)?' on':'');
      d.innerHTML=`<img src="signs/${code}.png" alt="${code}"><span class="lbl">${code}</span>`;
      d.onclick=()=>{ const i=b.pictograms.indexOf(code); if(i>=0)b.pictograms.splice(i,1); else b.pictograms.push(code); renderSigns(); };
      g.appendChild(d); });
    el.appendChild(g); });
}
function renderAll(){ if(RB)document.getElementById('trackName').value=RB.trackName||''; document.getElementById('meta').textContent=RB?`${RB.trackName||'roadbook'} — ${RB.boxes.length} boxes`:''; renderList(); renderFields(); renderSigns(); drawCanvas(); }
document.getElementById('trackName').oninput=e=>{ if(RB){ RB.trackName=e.target.value; document.getElementById('meta').textContent=`${RB.trackName||'roadbook'} — ${RB.boxes.length} boxes`; } };

document.getElementById('bNew').onclick=()=>{ if(confirm('Start a new roadbook?'))newBook(); };
document.getElementById('bAdd').onclick=()=>{ const i=Math.min(sel+1,RB.boxes.length); RB.boxes.splice(i,0,blankBox(i,'turn')); RB.boxes.forEach((b,k)=>b.index=k); sel=i; renderAll(); };
document.getElementById('bDel').onclick=()=>{ if(RB.boxes.length>1){ RB.boxes.splice(sel,1); RB.boxes.forEach((b,k)=>b.index=k); sel=Math.max(0,sel-1); renderAll(); } };
document.getElementById('bUp').onclick=()=>{ if(sel>0){ [RB.boxes[sel-1],RB.boxes[sel]]=[RB.boxes[sel],RB.boxes[sel-1]]; RB.boxes.forEach((b,k)=>b.index=k); sel--; renderAll(); } };
document.getElementById('bDown').onclick=()=>{ if(sel<RB.boxes.length-1){ [RB.boxes[sel+1],RB.boxes[sel]]=[RB.boxes[sel],RB.boxes[sel+1]]; RB.boxes.forEach((b,k)=>b.index=k); sel++; renderAll(); } };
document.getElementById('file').onchange=e=>{ const f=e.target.files[0]; if(!f)return; const r=new FileReader(); r.onload=()=>{ try{ RB=JSON.parse(r.result); if(!RB.trackName&&RB.meta)RB.trackName=RB.meta.trackName||RB.meta.trackId||''; RB.boxes.forEach(b=>{b.tulip=b.tulip||{points:[[.5,.1],[.5,.9]]};b.tulip.points=b.tulip.points||[[.5,.1],[.5,.9]];b.pictograms=b.pictograms||[];if(b.turnDir==null)b.turnDir='none';}); sel=0; renderAll(); }catch(err){ alert('Invalid roadbook JSON'); } }; r.readAsText(f); };
document.getElementById('bSave').onclick=()=>{ RB.boxes.forEach((b,k)=>b.index=k); const blob=new Blob([JSON.stringify(RB,null,1)],{type:'application/json'}); const a=document.createElement('a'); a.href=URL.createObjectURL(blob); a.download=(RB.trackName||'roadbook').replace(/\s+/g,'_')+'.json'; a.click(); };

newBook();
