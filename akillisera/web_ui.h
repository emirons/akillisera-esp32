// web_ui.h — single-page control UI, stored in PROGMEM, English.
// No external CDN/font/library (works offline). No localStorage.
// Served via send_P. Raw literal delimiter: HTMLDOC.
// Layout: 2 columns (left = readings/water/schedule, right = fan/LED),
// full-width analytics row below (live trend charts + insights).
// Theme: follows system by default (prefers-color-scheme); toggle switches
// per-session (no localStorage persistence — that constraint is intentional).
#pragma once

const char INDEX_HTML[] PROGMEM = R"HTMLDOC(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#0d1b12">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Smart Greenhouse</title>
<style>
:root{--bg:#0d1b12;--card:#16281c;--ln:#24402e;--tx:#e8f5e9;--mut:#9fc7ad;--ac:#43d17a;--dg:#ff5a5a}
@media(prefers-color-scheme:light){:root{--bg:#eef4f0;--card:#ffffff;--ln:#d6e4da;--tx:#12241a;--mut:#4a6a55;--ac:#1e9e5a;--dg:#d33}}
:root[data-theme=dark]{--bg:#0d1b12;--card:#16281c;--ln:#24402e;--tx:#e8f5e9;--mut:#9fc7ad;--ac:#43d17a;--dg:#ff5a5a}
:root[data-theme=light]{--bg:#eef4f0;--card:#ffffff;--ln:#d6e4da;--tx:#12241a;--mut:#4a6a55;--ac:#1e9e5a;--dg:#d33}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--tx);font-family:system-ui,-apple-system,sans-serif;padding:12px;max-width:900px;margin:0 auto}
h1{font-size:22px;margin:0}
.hdr{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px;gap:10px}
.st{display:flex;align-items:center;gap:8px;font-size:14px;color:var(--mut)}
.dot{width:12px;height:12px;border-radius:50%;background:var(--dg)}
.dot.ok{background:var(--ac)}
.theme{min-width:44px;min-height:44px;border-radius:10px;background:var(--ln);border:1px solid var(--ln);color:var(--tx);font-size:18px;cursor:pointer}
.wrap{display:grid;grid-template-columns:1fr 1fr;gap:12px;align-items:start}
@media(max-width:640px){.wrap{grid-template-columns:1fr}}
.col{display:flex;flex-direction:column;gap:12px}
.full{grid-column:1/-1}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.card{background:var(--card);border:1px solid var(--ln);border-radius:12px;padding:14px}
.card h2{font-size:13px;margin:0 0 6px;color:var(--mut);font-weight:600}
.big{font-size:32px;font-weight:700;line-height:1}
.unit{font-size:15px;color:var(--mut)}
.bar{height:8px;background:var(--ln);border-radius:5px;margin-top:8px;overflow:hidden}
.bar>i{display:block;height:100%;background:var(--ac);width:0}
.card.dim{opacity:.45}
.ctrl{background:var(--card);border:1px solid var(--ln);border-radius:12px;padding:14px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;min-height:44px}
.ctrl .row+.row{margin-top:12px;border-top:1px solid var(--ln);padding-top:12px}
button{font:inherit;color:var(--tx);background:var(--ln);border:1px solid var(--ac);border-radius:10px;min-height:48px;padding:0 16px;cursor:pointer}
.primary{width:100%;background:var(--ac);color:#04120a;font-size:18px;font-weight:700;border:none}
.primary:disabled{opacity:.5;cursor:default}
button:focus-visible,input:focus-visible,.tg:focus-visible{outline:3px solid var(--ac);outline-offset:2px}
.tg{position:relative;width:76px;height:44px;border-radius:22px;background:var(--ln);border:1px solid var(--ln);flex:0 0 auto}
.tg[aria-checked=true]{background:var(--ac)}
.tg::after{content:"";position:absolute;top:4px;left:4px;width:36px;height:36px;border-radius:50%;background:#fff;transition:left .15s}
.tg[aria-checked=true]::after{left:36px}
input[type=range]{width:100%;height:44px}
input[type=time]{flex:1;min-height:48px;background:var(--ln);color:var(--tx);border:1px solid var(--ln);border-radius:10px;padding:0 12px;font:inherit}
.lbl{font-size:15px}
.val{font-weight:700;color:var(--ac)}
.disabled{opacity:.45;pointer-events:none}
ul{list-style:none;padding:0;margin:12px 0 10px}
.ins{display:flex;align-items:flex-start;gap:8px;padding:8px 0;border-bottom:1px solid var(--ln);font-size:14px}
canvas{width:100%;height:180px;display:block}
.lg{display:flex;gap:14px;font-size:12px;color:var(--mut);margin-top:6px;flex-wrap:wrap}
.lg b{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:4px}
.ft{display:flex;justify-content:space-between;font-size:12px;color:var(--mut);margin-top:12px;flex-wrap:wrap;gap:6px}
.note{font-size:12px;color:var(--mut);margin-top:8px}
</style>
</head>
<body>
<div class="hdr">
  <h1>Smart Greenhouse</h1>
  <div class="st"><span id="dot" class="dot"></span><span id="stt">Connecting...</span>
    <button class="theme" id="tbtn" onclick="tema()" aria-label="Toggle light or dark theme">&#9788;</button>
  </div>
</div>

<div class="ctrl" style="margin-bottom:12px">
  <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap">
    <span class="lbl">Plant</span>
    <select id="bitki" aria-label="Select plant" onchange="bitkiSec(this.value)"
      style="min-height:44px;flex:1 1 140px;background:var(--ln);color:var(--tx);border:1px solid var(--ln);border-radius:10px;padding:0 10px;font:inherit"></select>
    <span class="lbl">Stage</span>
    <select id="evre" aria-label="Growth stage" onchange="evreSec(this.value)"
      style="min-height:44px;flex:1 1 130px;background:var(--ln);color:var(--tx);border:1px solid var(--ln);border-radius:10px;padding:0 10px;font:inherit">
      <option value="fide">Seedling</option><option value="vejetatif">Vegetative</option><option value="meyve">Flowering/Fruiting</option>
    </select>
    <span class="lbl">Season</span>
    <select id="mevsim" aria-label="Season" onchange="mevsimSec(this.value)"
      style="min-height:44px;flex:1 1 120px;background:var(--ln);color:var(--tx);border:1px solid var(--ln);border-radius:10px;padding:0 10px;font:inherit">
      <option value="ilkbahar">Spring</option><option value="yaz">Summer</option><option value="sonbahar">Autumn</option><option value="kis">Winter</option>
    </select>
  </div>
</div>

<div class="wrap">
  <!-- LEFT COLUMN -->
  <div class="col">
    <div class="grid">
      <div class="card" id="cSic"><h2>Temperature</h2><div><span class="big" id="sic">--</span><span class="unit"> &deg;C</span></div></div>
      <div class="card" id="cNem"><h2>Humidity</h2><div><span class="big" id="nem">--</span><span class="unit"> %</span></div></div>
      <div class="card"><h2>Soil Moisture</h2><div><span class="big" id="top">--</span><span class="unit"> %</span></div><div class="bar"><i id="topb"></i></div></div>
      <div class="card"><h2>Light</h2><div><span class="big" id="isk">--</span><span class="unit"> %</span></div><div class="bar"><i id="iskb"></i></div></div>
    </div>
    <div class="ctrl">
      <div class="row"><span class="lbl">Watering</span>
        <div style="display:flex;align-items:center;gap:10px">
          <span id="smodt" class="val">Auto</span>
          <div class="tg" id="smod" role="switch" aria-checked="false" tabindex="0" aria-label="Watering auto or manual" onclick="smod()" onkeydown="if(event.key==' '||event.key=='Enter'){event.preventDefault();smod()}"></div>
        </div>
      </div>
      <div class="row" id="sularow"><button class="primary" id="sula" onclick="sula()" style="width:100%">WATER (5s)</button></div>
    </div>
    <div class="ctrl">
      <div style="display:flex;justify-content:space-between;align-items:center">
        <span class="lbl" id="schedttl">Scheduled Watering</span><span class="val" id="saat">--:--:--</span>
      </div>
      <ul id="plan"></ul>
      <div id="addform" style="display:flex;gap:10px">
        <input type="time" id="yenizaman" value="08:00" aria-label="New watering time">
        <button onclick="planEkle()">Add</button>
      </div>
    </div>
  </div>

  <!-- RIGHT COLUMN -->
  <div class="col">
    <div class="ctrl">
      <div class="row"><span class="lbl">Fan</span>
        <div style="display:flex;align-items:center;gap:10px">
          <span id="fmodt" class="val">Auto</span>
          <div class="tg" id="fmod" role="switch" aria-checked="false" tabindex="0" aria-label="Fan auto or manual" onclick="fmod()" onkeydown="if(event.key==' '||event.key=='Enter'){event.preventDefault();fmod()}"></div>
        </div>
      </div>
      <div class="row" id="fanrow"><span class="lbl">Fan status</span>
        <div class="tg" id="fan" role="switch" aria-checked="false" tabindex="0" aria-label="Fan on or off" onclick="fan()" onkeydown="if(event.key==' '||event.key=='Enter'){event.preventDefault();fan()}"></div>
      </div>
    </div>
    <div class="ctrl">
      <div class="row"><span class="lbl">Strip LED</span>
        <div style="display:flex;align-items:center;gap:10px">
          <span id="lmodt" class="val">Auto</span>
          <div class="tg" id="lmod" role="switch" aria-checked="false" tabindex="0" aria-label="LED auto or manual" onclick="lmod()" onkeydown="if(event.key==' '||event.key=='Enter'){event.preventDefault();lmod()}"></div>
        </div>
      </div>
      <div class="row" id="ledrow" style="display:block"><div style="display:flex;justify-content:space-between"><span class="lbl">Brightness</span><span class="val" id="ledv">0</span></div>
        <input type="range" id="led" min="0" max="255" value="0" aria-label="LED brightness 0 to 255" oninput="q('ledv').textContent=this.value" onchange="ledGonder(this.value)">
      </div>
    </div>
  </div>

  <!-- FULL-WIDTH ANALYTICS -->
  <div class="full card">
    <h2>Trends</h2>
    <canvas id="chart" width="600" height="180" aria-label="Sensor trend chart"></canvas>
    <div class="lg"><span><b style="background:#ff7a45"></b>Temp</span><span><b style="background:#43a3ff"></b>Humidity</span><span><b style="background:#43d17a"></b>Soil</span></div>
    <div class="note">Live trends since page load. Long-term history is logged to ThingSpeak (cloud) — charts added in the cloud phase.</div>
  </div>

  <div class="full card">
    <h2>Condition Ranges</h2>
    <div id="ranges"></div>
    <div class="note">Marker = current value, green band = ideal range. Adjust if a value is low or high.</div>
  </div>

  <div class="full card">
    <h2>Activity &amp; Next Watering</h2>
    <div id="act" style="display:flex;gap:18px;flex-wrap:wrap;align-items:center"></div>
  </div>

  <div class="full card">
    <h2>History (ThingSpeak)</h2>
    <div id="ts" class="grid"></div>
    <div class="note">Long-term cloud history. Needs internet and a public ThingSpeak channel.</div>
  </div>

  <div class="full card">
    <h2>Insights &amp; Recommendations</h2>
    <ul id="ins"></ul>
  </div>
</div>

<div class="ft"><span id="up">Uptime: --</span><span id="rssi">Signal: --</span><span id="son">Updated: --</span></div>

<script>
// Fan and LED are INDEPENDENT (each has its own auto/manual). Slider posts only
// on release (change), never while dragging, so the ESP32 isn't flooded.
var fanOto=true, ledOto=true, sulamaOto=true, hataSay=0, planTazele=true;
var hist={t:[],h:[],s:[]}, HMAX=60;
var acc={n:0,fan:0,led:0,pump:0}, planList=[], bitki='custom';
// Bitki bilgi tabanı — bahçecilik referanslarından derlenmiş ideal aralıklar +
// bayraklar (kombinasyon önerileri için) + kısa bakım notu. Genişletilebilir
// (ileride harici bir API/kaynaktan da beslenebilir). t/h/s/l = [min,max].
var PLANTS={
 custom:{name:'Custom / General',t:[18,28],h:[40,70],s:[40,80],l:[30,80],f:{},tip:'General ranges. Pick a plant for tailored advice.'},
 tomato:{name:'Tomato',t:[18,27],h:[50,70],s:[50,75],l:[60,90],f:{humid:1,light:1,warm:1},tip:'Steady moisture, strong light, good airflow.'},
 lettuce:{name:'Lettuce',t:[10,20],h:[50,70],s:[60,80],l:[40,70],f:{cool:1,thirsty:1},tip:'Cool-season; keep soil moist, avoid heat (bolting).'},
 pepper:{name:'Pepper',t:[20,30],h:[50,70],s:[45,70],l:[60,95],f:{light:1,warm:1},tip:'Warm and bright; let soil dry slightly between waterings.'},
 cucumber:{name:'Cucumber',t:[20,28],h:[60,80],s:[60,85],l:[60,90],f:{humid:1,thirsty:1,warm:1},tip:'Loves warmth and humidity; water generously.'},
 basil:{name:'Basil',t:[18,28],h:[40,60],s:[40,65],l:[55,90],f:{light:1,warm:1},tip:'Bright light; avoid soggy soil (dislikes wet feet).'},
 strawberry:{name:'Strawberry',t:[15,24],h:[50,70],s:[50,75],l:[55,90],f:{light:1},tip:'Moderate temps, steady moisture, good light.'},
 spinach:{name:'Spinach',t:[10,22],h:[50,70],s:[55,80],l:[40,70],f:{cool:1,thirsty:1},tip:'Cool-season; bolts in heat, keep soil moist.'}
};
var evre='vejetatif', mevsim='ilkbahar', gecemi=false;
function aktifBitki(){return PLANTS[bitki]||PLANTS.custom;}
function clampB(v,lo,hi){return v<lo?lo:(v>hi?hi:v);}
// Etkin ideal aralıklar: bitki tabanı + büyüme evresi + mevsim (backend etkinParam ile aynı mantık).
function etkinBitki(){
  var P=aktifBitki();var dT=0,dH=0,dS=0,dL=0;
  if(evre=='fide'){dS+=10;dL-=10;dH+=5;dT-=2;} else if(evre=='meyve'){dS+=10;dL+=10;dH-=5;dT+=1;}
  if(mevsim=='yaz'){dT-=2;dS+=8;dL-=10;} else if(mevsim=='kis'){dT+=2;dS-=8;dL+=15;}
  if(gecemi)dT-=2;   // gece serin (kontrol ile ayni)
  function sh(b,o,lo,hi){return [clampB(b[0]+o,lo,hi),clampB(b[1]+o,lo,hi)];}
  return {name:P.name,tip:P.tip,f:P.f,t:sh(P.t,dT,0,50),h:sh(P.h,dH,0,100),s:sh(P.s,dS,0,100),l:sh(P.l,dL,0,100)};
}
function evreAdi(){return {fide:'Seedling',vejetatif:'Vegetative',meyve:'Flowering/Fruiting'}[evre]||evre;}
function mevsimAdi(){return {ilkbahar:'Spring',yaz:'Summer',sonbahar:'Autumn',kis:'Winter'}[mevsim]||mevsim;}
async function evreSec(v){evre=v;try{await fetch('/api/bitki',{method:'POST',body:JSON.stringify({evre:v})});}catch(e){}durumCek();}
async function mevsimSec(v){mevsim=v;try{await fetch('/api/bitki',{method:'POST',body:JSON.stringify({mevsim:v})});}catch(e){}durumCek();}
function trend(a){var v=a.filter(function(x){return x!=null;});if(v.length<4)return 0;return v[v.length-1]-v[Math.max(0,v.length-10)];}
async function bitkiSec(k){bitki=k;try{await fetch('/api/bitki',{method:'POST',body:JSON.stringify({bitki:k})});}catch(e){}durumCek();}
function bitkiDoldur(){var s=q('bitki');if(s.options.length)return;var h='';for(var k in PLANTS)h+='<option value="'+k+'">'+PLANTS[k].name+'</option>';s.innerHTML=h;}
function q(i){return document.getElementById(i)}
function tema(){var r=document.documentElement;var cur=r.getAttribute('data-theme')||(matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light');var nx=cur=='dark'?'light':'dark';r.setAttribute('data-theme',nx);q('tbtn').innerHTML=nx=='dark'?'&#9788;':'&#9789;';drawChart();}
async function post(u,b){try{await fetch(u,{method:'POST',body:JSON.stringify(b)});}catch(e){}await durumCek();}
function sula(){var b=q('sula');post('/api/sula',{});var n=5;b.disabled=true;b.textContent='Watering '+n+'s';var t=setInterval(function(){n--;if(n<=0){clearInterval(t);b.disabled=false;b.textContent='WATER (5s)';}else b.textContent='Watering '+n+'s';},1000);}
function fmod(){post('/api/mod',{fan:!fanOto});}
function lmod(){post('/api/mod',{led:!ledOto});}
function smod(){post('/api/mod',{sulama:!sulamaOto});}
// Sulama modu görünümü: OTO -> WATER+ekleme soluk, plan = bitki optimum saatleri
// (salt-okunur). MANUEL -> WATER+ekleme aktif, plan = kullanıcı planı.
function renderSulama(d){
  if(sulamaOto!==d.sulamaOto)planTazele=true;   // mod degisti -> plani bir kez tazele
  sulamaOto=d.sulamaOto;
  q('smod').setAttribute('aria-checked',sulamaOto?'false':'true');
  q('smodt').textContent=sulamaOto?'Auto':'Manual';
  q('sularow').className='row'+(sulamaOto?' disabled':'');
  q('addform').style.display=sulamaOto?'none':'flex';
  var sn=d.sulamaSn||0;
  q('schedttl').textContent=sulamaOto
    ? 'Optimal Times ('+aktifBitki().name+') — '+(d.otoSulama||[]).length+'x/day, '+sn+'s each'
    : 'Scheduled Watering';
  if(sulamaOto){
    var arr=d.otoSulama||[];planList=arr;var u=q('plan');u.innerHTML='';
    if(!arr.length){u.innerHTML='<li style="color:var(--mut);padding:6px 0">No auto times</li>';return;}
    arr.forEach(function(z){var li=document.createElement('li');li.style.cssText='display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid var(--ln)';
      var t=document.createElement('span');t.style.fontSize='18px';t.textContent=('0'+z.saat).slice(-2)+':'+('0'+z.dakika).slice(-2);
      var b=document.createElement('span');b.className='val';b.textContent=(d.sulamaSn||0)+'s';li.appendChild(t);li.appendChild(b);u.appendChild(li);});
  }else if(planTazele){planTazele=false;planCek();}   // yalnizca mod degisince cek
}
function fan(){if(fanOto)return;post('/api/fan',{acik:q('fan').getAttribute('aria-checked')!='true'});}
function ledGonder(v){if(ledOto)return;post('/api/led',{parlaklik:parseInt(v)});}
function modGorunum(){
  q('fanrow').className='row'+(fanOto?' disabled':'');
  q('ledrow').className='row'+(ledOto?' disabled':'');
  q('fmod').setAttribute('aria-checked',fanOto?'false':'true');q('fmodt').textContent=fanOto?'Auto':'Manual';
  q('lmod').setAttribute('aria-checked',ledOto?'false':'true');q('lmodt').textContent=ledOto?'Auto':'Manual';
}
async function planCek(){
  try{var r=await fetch('/api/plan');var d=await r.json();planList=d.zamanlar||[];var u=q('plan');u.innerHTML='';
    if(!d.zamanlar||!d.zamanlar.length){u.innerHTML='<li style="color:var(--mut);padding:6px 0">No schedules yet</li>';return;}
    d.zamanlar.forEach(function(z,i){
      var li=document.createElement('li');li.style.cssText='display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid var(--ln)';
      var t=document.createElement('span');t.style.fontSize='18px';t.textContent=('0'+z.saat).slice(-2)+':'+('0'+z.dakika).slice(-2);
      var b=document.createElement('button');b.textContent='Delete';b.setAttribute('aria-label','Delete watering time');
      b.style.cssText='min-height:44px;background:var(--dg);border-color:var(--dg);color:#fff';b.onclick=function(){planSil(i)};
      li.appendChild(t);li.appendChild(b);u.appendChild(li);
    });
  }catch(e){}
}
async function planEkle(){var v=q('yenizaman').value;if(!v)return;var p=v.split(':');
  try{await fetch('/api/plan',{method:'POST',body:JSON.stringify({saat:parseInt(p[0]),dakika:parseInt(p[1])})});}catch(e){}planCek();}
async function planSil(i){try{await fetch('/api/plan',{method:'DELETE',body:JSON.stringify({index:i})});}catch(e){}planCek();}
function pushHist(d){var ge=d.dhtGecerli;hist.t.push(ge?d.sicaklik:null);hist.h.push(ge?d.nem:null);hist.s.push(d.toprakYuzde);
  if(hist.t.length>HMAX){hist.t.shift();hist.h.shift();hist.s.shift();}}
function line(c,arr,col,mn,mx,W,H){c.strokeStyle=col;c.lineWidth=2;c.beginPath();var n=arr.length,first=true;
  for(var i=0;i<n;i++){if(arr[i]==null){first=true;continue;}var x=n<2?0:i/(n-1)*(W-8)+4;var y=H-6-(arr[i]-mn)/(mx-mn||1)*(H-12);
    if(first){c.moveTo(x,y);first=false;}else c.lineTo(x,y);}c.stroke();}
function drawChart(){var cv=q('chart');if(!cv)return;var W=cv.clientWidth||600,H=180;cv.width=W;cv.height=H;
  var c=cv.getContext('2d');var g=getComputedStyle(document.documentElement).getPropertyValue('--ln');
  c.clearRect(0,0,W,H);c.strokeStyle=g;c.lineWidth=1;for(var k=1;k<4;k++){var y=k/4*H;c.beginPath();c.moveTo(0,y);c.lineTo(W,y);c.stroke();}
  line(c,hist.t,'#ff7a45',0,50,W,H);line(c,hist.h,'#43a3ff',0,100,W,H);line(c,hist.s,'#43d17a',0,100,W,H);}
// Öneri motoru: Condition Ranges (bitki ideal aralığı) + History trend + bitki
// bilgi tabanı bayraklarının BİRLEŞİMİnden özgün, kombinasyona özel tavsiyeler.
function insights(d){var P=etkinBitki(),f=P.f||{},out=[];
  if(d.alarm)out.push(['&#9888;','High-temperature alarm — ventilation active. Check greenhouse cooling.']);
  if(!d.dhtGecerli)out.push(['&#9888;','Temperature/humidity sensor fault — readings unavailable.']);
  // Bitkiye özel aralık kontrolleri
  function rng(v,r,unit,lbl,lo,hi){if(v<r[0])out.push(['&#9660;',lbl+' '+v+unit+' is below '+P.name+' ideal '+r[0]+'-'+r[1]+unit+'. '+lo]);else if(v>r[1])out.push(['&#9650;',lbl+' '+v+unit+' is above '+P.name+' ideal '+r[0]+'-'+r[1]+unit+'. '+hi]);}
  if(d.dhtGecerli){rng(Math.round(d.sicaklik),P.t,'°C','Temperature','Add warmth or cut ventilation.','Increase ventilation or shade.');rng(Math.round(d.nem),P.h,'%','Humidity','Mist or reduce airflow.','Ventilate to lower humidity.');}
  rng(d.toprakYuzde,P.s,'%','Soil moisture','Water now.','Ease off watering — root-rot risk.');
  rng(d.isikYuzde,P.l,'%','Light','Add grow light.','Provide shade.');
  // Kombinasyon + trend -> bitkiye özgü öneriler
  var tT=trend(hist.t),sT=trend(hist.s);
  if(f.humid&&d.dhtGecerli&&d.nem>P.h[1]&&d.sicaklik>=(P.t[0]+P.t[1])/2)out.push(['&#9888;','Warm + humid air raises fungal-disease risk for '+P.name+'. Boost airflow and keep foliage dry.']);
  if(f.cool&&d.dhtGecerli&&d.sicaklik>P.t[1])out.push(['&#9888;',P.name+' is a cool-season crop — sustained heat triggers bolting. Ventilate or shade now.']);
  if(f.warm&&d.dhtGecerli&&d.sicaklik<P.t[0])out.push(['&#10052;',P.name+' prefers warmth — current temperature is too low; add heat or reduce venting.']);
  if(f.light&&d.isikYuzde<P.l[0])out.push(['&#128161;',P.name+' is light-hungry; set the strip LED to manual and raise brightness.']);
  if(f.thirsty&&sT<-3)out.push(['&#128167;','Soil is drying quickly and '+P.name+' prefers steady moisture — add a watering schedule soon.']);
  if(tT>3&&d.dhtGecerli&&d.sicaklik>P.t[1]-2)out.push(['&#127777;','Temperature is climbing toward '+P.name+" upper limit — pre-emptive ventilation advised."]);
  // VPD (hava stresi) + gündüz/gece
  if(d.dhtGecerli){var vp=d.vpd||0;
    if(vp>1.6)out.push(['&#9888;','VPD '+vp.toFixed(2)+' kPa — air too dry (transpiration stress). No humidifier in hardware: mist manually or reduce ventilation.']);
    else if(vp>0&&vp<0.5)out.push(['&#128167;','VPD '+vp.toFixed(2)+' kPa — air too humid (fungal risk). Increase ventilation.']);}
  if(d.gece)out.push(['&#127769;','Night mode: grow light off and cooler target — plants resting.']);
  if(!out.length)out.push(['&#9989;','All conditions are within ideal range for '+P.name+' (auto mode holding the band).']);
  var u=q('ins');u.innerHTML='';out.forEach(function(o){var li=document.createElement('li');li.className='ins';
    li.innerHTML='<span>'+o[0]+'</span><span>'+o[1]+'</span>';u.appendChild(li);});}
function gauge(name,val,mn,mx,lo,hi,unit){
  var v=Number(val);var p=Math.max(0,Math.min(100,(v-mn)/(mx-mn)*100));
  var bl=(lo-mn)/(mx-mn)*100,bw=(hi-lo)/(mx-mn)*100;var ok=v>=lo&&v<=hi;var tag=ok?'OK':(v<lo?'low':'high');
  return '<div style="margin:12px 0"><div style="display:flex;justify-content:space-between;font-size:13px"><span>'+name+'</span><span style="font-weight:700;color:'+(ok?'var(--ac)':'var(--dg)')+'">'+val+unit+' · '+tag+'</span></div><div style="position:relative;height:12px;background:var(--ln);border-radius:6px;margin-top:5px"><div style="position:absolute;left:'+bl+'%;width:'+bw+'%;top:0;bottom:0;background:rgba(67,209,122,.35);border-radius:6px"></div><div style="position:absolute;left:'+p+'%;top:-4px;width:4px;height:20px;background:var(--tx);border-radius:2px;transform:translateX(-2px)"></div></div></div>';
}
function renderRanges(d){var P=etkinBitki();var h='';
  if(d.dhtGecerli){h+=gauge('Temperature',d.sicaklik.toFixed(1),0,50,P.t[0],P.t[1],' °C');h+=gauge('Humidity',Math.round(d.nem),0,100,P.h[0],P.h[1],'%');}
  h+=gauge('Soil Moisture',d.toprakYuzde,0,100,P.s[0],P.s[1],'%');h+=gauge('Light',d.isikYuzde,0,100,P.l[0],P.l[1],'%');
  h+='<div class="note">Ideal ranges for <b>'+P.name+'</b> · '+evreAdi()+' · '+mevsimAdi()+'. '+P.tip+'</div>';q('ranges').innerHTML=h;}
function ring(name,pct){return '<div style="text-align:center"><div style="width:72px;height:72px;border-radius:50%;background:conic-gradient(var(--ac) '+pct+'%,var(--ln) 0)"><div style="position:relative;top:10px;left:10px;width:52px;height:52px;border-radius:50%;background:var(--card);display:flex;align-items:center;justify-content:center;font-weight:700">'+pct+'%</div></div><div style="font-size:12px;color:var(--mut);margin-top:4px">'+name+'</div></div>';}
function nextWatering(saat){
  if(!planList.length||!saat||saat.charAt(0)=='-')return '—';
  var p=saat.split(':');var now=parseInt(p[0])*60+parseInt(p[1]);var best=1e9,bt='';
  planList.forEach(function(z){var sm=z.saat*60+z.dakika;var df=(sm-now+1440)%1440;if(df<best){best=df;bt=('0'+z.saat).slice(-2)+':'+('0'+z.dakika).slice(-2);}});
  return bt+' (in '+Math.floor(best/60)+'h '+(best%60)+'m)';}
function renderTS(k){var t=q('ts');
  if(!k||k=='THINGSPEAK_CHANNEL_ID'||k=='kanal_id'||k=='KANAL_ID'){t.innerHTML='<div class="note" style="grid-column:1/-1">Set a public ThingSpeak channel to see history charts.</div>';return;}
  if(t.getAttribute('data-k')==k)return;t.setAttribute('data-k',k);   // iframe'leri bir kez kur
  var f=[[1,'Temperature','d62020'],[2,'Humidity','1f6feb'],[3,'Soil','2ea043'],[4,'Light','e3b341']];
  t.innerHTML=f.map(function(x){return '<iframe title="'+x[1]+'" loading="lazy" style="border:1px solid var(--ln);border-radius:8px;width:100%;height:160px" src="https://thingspeak.com/channels/'+k+'/charts/'+x[0]+'?bgcolor=%23ffffff&color=%23'+x[2]+'&dynamic=true&days=1&results=60&type=line&title='+encodeURIComponent(x[1])+'&xaxis=Time&yaxis="></iframe>';}).join('');}
function renderActivity(d){var n=acc.n||1;
  var html=ring('Fan',Math.round(acc.fan/n*100))+ring('LED',Math.round(acc.led/n*100))+ring('Pump',Math.round(acc.pump/n*100));
  var vpd=(d.vpd||0).toFixed(2);
  html+='<div style="flex:1;min-width:150px">'
    +'<div style="font-size:13px;color:var(--mut)">Next watering</div><div style="font-size:19px;font-weight:700;color:var(--ac)">'+nextWatering(d.saat)+'</div>'
    +'<div style="font-size:13px;color:var(--mut);margin-top:8px">VPD · Mode</div>'
    +'<div style="font-size:16px;font-weight:700">'+vpd+' kPa · '+(d.gece?'&#127769; Night':'&#9728; Day')+'</div></div>';
  q('act').innerHTML=html;}
async function durumCek(){
  try{
    var r=await fetch('/api/durum');if(!r.ok)throw 0;var d=await r.json();hataSay=0;
    q('dot').className='dot ok';q('stt').textContent='Connected';
    var ge=d.dhtGecerli;
    q('sic').textContent=ge?d.sicaklik.toFixed(1):'--';q('nem').textContent=ge?Math.round(d.nem):'--';
    q('cSic').className='card'+(ge?'':' dim');q('cNem').className='card'+(ge?'':' dim');
    q('top').textContent=d.toprakYuzde;q('topb').style.width=d.toprakYuzde+'%';
    q('isk').textContent=d.isikYuzde;q('iskb').style.width=d.isikYuzde+'%';
    q('fan').setAttribute('aria-checked',d.fan?'true':'false');
    var ls=q('led');if(document.activeElement!==ls){ls.value=d.led;q('ledv').textContent=d.led;}
    fanOto=d.fanOto;ledOto=d.ledOto;modGorunum();
    if(d.bitki)bitki=d.bitki;var bs=q('bitki');if(bs&&bs.value!=bitki)bs.value=bitki;
    if(d.evre)evre=d.evre;var es=q('evre');if(es&&es.value!=evre)es.value=evre;
    if(d.mevsim)mevsim=d.mevsim;var ms=q('mevsim');if(ms&&ms.value!=mevsim)ms.value=mevsim;
    gecemi=!!d.gece;
    renderSulama(d);
    q('saat').textContent=d.saat||'--:--:--';
    q('up').textContent='Uptime: '+Math.floor(d.calismaSuresi/1000)+'s';
    q('rssi').textContent='Signal: '+(d.rssi||'--')+' dBm';
    q('son').textContent='Updated: now';
    acc.n++;if(d.fan)acc.fan++;if(d.led>0)acc.led++;if(d.pompa)acc.pump++;
    pushHist(d);drawChart();insights(d);renderRanges(d);renderActivity(d);renderTS(d.tsKanal);
  }catch(e){hataSay++;q('dot').className='dot';q('stt').textContent='No connection';}
}
bitkiDoldur();durumCek();setInterval(durumCek,2000);
addEventListener('resize',drawChart);
</script>
</body>
</html>)HTMLDOC";
