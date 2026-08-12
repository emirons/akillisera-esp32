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

<div class="wrap">
  <!-- LEFT COLUMN -->
  <div class="col">
    <div class="grid">
      <div class="card" id="cSic"><h2>Temperature</h2><div><span class="big" id="sic">--</span><span class="unit"> &deg;C</span></div></div>
      <div class="card" id="cNem"><h2>Humidity</h2><div><span class="big" id="nem">--</span><span class="unit"> %</span></div></div>
      <div class="card"><h2>Soil Moisture</h2><div><span class="big" id="top">--</span><span class="unit"> %</span></div><div class="bar"><i id="topb"></i></div></div>
      <div class="card"><h2>Light</h2><div><span class="big" id="isk">--</span><span class="unit"> %</span></div><div class="bar"><i id="iskb"></i></div></div>
    </div>
    <div class="ctrl"><button class="primary" id="sula" onclick="sula()">WATER (5s)</button></div>
    <div class="ctrl">
      <div style="display:flex;justify-content:space-between;align-items:center">
        <span class="lbl">Scheduled Watering</span><span class="val" id="saat">--:--:--</span>
      </div>
      <ul id="plan"></ul>
      <div style="display:flex;gap:10px">
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
var fanOto=true, ledOto=true, hataSay=0;
var hist={t:[],h:[],s:[]}, HMAX=60;
var acc={n:0,fan:0,led:0,pump:0}, planList=[];
function q(i){return document.getElementById(i)}
function tema(){var r=document.documentElement;var cur=r.getAttribute('data-theme')||(matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light');var nx=cur=='dark'?'light':'dark';r.setAttribute('data-theme',nx);q('tbtn').innerHTML=nx=='dark'?'&#9788;':'&#9789;';drawChart();}
async function post(u,b){try{await fetch(u,{method:'POST',body:JSON.stringify(b)});}catch(e){}await durumCek();}
function sula(){var b=q('sula');post('/api/sula',{});var n=5;b.disabled=true;b.textContent='Watering '+n+'s';var t=setInterval(function(){n--;if(n<=0){clearInterval(t);b.disabled=false;b.textContent='WATER (5s)';}else b.textContent='Watering '+n+'s';},1000);}
function fmod(){post('/api/mod',{fan:!fanOto});}
function lmod(){post('/api/mod',{led:!ledOto});}
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
function insights(d){var out=[];
  if(d.alarm)out.push(['&#9888;','High temperature alarm — ventilation active. Check greenhouse cooling.']);
  if(!d.dhtGecerli)out.push(['&#9888;','Temperature/humidity sensor fault — readings unavailable.']);
  if(d.dhtGecerli&&d.sicaklik>30&&!d.alarm)out.push(['&#127777;','Temperature rising — monitor ventilation.']);
  if(d.dhtGecerli&&d.nem>70)out.push(['&#128167;','High humidity — extra ventilation recommended to prevent mold.']);
  if(d.toprakYuzde<30)out.push(['&#127793;','Soil is dry — watering recommended (auto/scheduled will handle it).']);
  if(d.isikYuzde<25)out.push(['&#128161;','Low light — supplemental grow light recommended.']);
  if(!out.length)out.push(['&#9989;','All conditions are optimal.']);
  var u=q('ins');u.innerHTML='';out.forEach(function(o){var li=document.createElement('li');li.className='ins';
    li.innerHTML='<span>'+o[0]+'</span><span>'+o[1]+'</span>';u.appendChild(li);});}
function gauge(name,val,mn,mx,lo,hi,unit){
  var v=Number(val);var p=Math.max(0,Math.min(100,(v-mn)/(mx-mn)*100));
  var bl=(lo-mn)/(mx-mn)*100,bw=(hi-lo)/(mx-mn)*100;var ok=v>=lo&&v<=hi;var tag=ok?'OK':(v<lo?'low':'high');
  return '<div style="margin:12px 0"><div style="display:flex;justify-content:space-between;font-size:13px"><span>'+name+'</span><span style="font-weight:700;color:'+(ok?'var(--ac)':'var(--dg)')+'">'+val+unit+' · '+tag+'</span></div><div style="position:relative;height:12px;background:var(--ln);border-radius:6px;margin-top:5px"><div style="position:absolute;left:'+bl+'%;width:'+bw+'%;top:0;bottom:0;background:rgba(67,209,122,.35);border-radius:6px"></div><div style="position:absolute;left:'+p+'%;top:-4px;width:4px;height:20px;background:var(--tx);border-radius:2px;transform:translateX(-2px)"></div></div></div>';
}
function renderRanges(d){var h='';
  if(d.dhtGecerli){h+=gauge('Temperature',d.sicaklik.toFixed(1),0,50,18,28,' °C');h+=gauge('Humidity',Math.round(d.nem),0,100,40,70,'%');}
  h+=gauge('Soil Moisture',d.toprakYuzde,0,100,40,80,'%');h+=gauge('Light',d.isikYuzde,0,100,30,80,'%');q('ranges').innerHTML=h;}
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
  html+='<div style="flex:1;min-width:150px"><div style="font-size:13px;color:var(--mut)">Next watering</div><div style="font-size:19px;font-weight:700;color:var(--ac)">'+nextWatering(d.saat)+'</div></div>';
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
    q('saat').textContent=d.saat||'--:--:--';
    q('up').textContent='Uptime: '+Math.floor(d.calismaSuresi/1000)+'s';
    q('rssi').textContent='Signal: '+(d.rssi||'--')+' dBm';
    q('son').textContent='Updated: now';
    acc.n++;if(d.fan)acc.fan++;if(d.led>0)acc.led++;if(d.pompa)acc.pump++;
    pushHist(d);drawChart();insights(d);renderRanges(d);renderActivity(d);renderTS(d.tsKanal);
  }catch(e){hataSay++;q('dot').className='dot';q('stt').textContent='No connection';}
}
durumCek();planCek();setInterval(durumCek,2000);
addEventListener('resize',drawChart);
</script>
</body>
</html>)HTMLDOC";
