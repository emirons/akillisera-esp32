// web_ui.h — tek sayfalık mobil kontrol arayüzü, PROGMEM'de.
// Harici CDN/font/kütüphane YOK (sera internetsiz çalışır). localStorage YOK.
// send_P ile servis edilir (RAM'e kopyalanmaz). Raw literal sınırlayıcı: HTMLDOC.
// PWA: manifest/service-worker EKLENMEDİ — flash tasarrufu; "ana ekrana ekle"
// theme-color + apple-mobile-web-app-capable meta'larıyla bunlarsız da çalışır.
#pragma once

const char INDEX_HTML[] PROGMEM = R"HTMLDOC(<!doctype html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#0d1b12">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Akilli Sera</title>
<style>
:root{--bg:#0d1b12;--card:#16281c;--ln:#24402e;--tx:#e8f5e9;--mut:#9fc7ad;--ac:#43d17a;--dg:#ff5a5a}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--tx);font-family:system-ui,-apple-system,sans-serif;padding:12px;max-width:520px;margin:0 auto}
h1{font-size:22px;margin:0}
.hdr{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.st{display:flex;align-items:center;gap:8px;font-size:14px;color:var(--mut)}
.dot{width:12px;height:12px;border-radius:50%;background:var(--dg)}
.dot.ok{background:var(--ac)}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
@media(max-width:360px){.grid{grid-template-columns:1fr}}
.card{background:var(--card);border:1px solid var(--ln);border-radius:12px;padding:14px}
.card h2{font-size:13px;margin:0 0 6px;color:var(--mut);font-weight:600}
.big{font-size:34px;font-weight:700;line-height:1}
.unit{font-size:16px;color:var(--mut)}
.bar{height:8px;background:var(--ln);border-radius:5px;margin-top:8px;overflow:hidden}
.bar>i{display:block;height:100%;background:var(--ac);width:0}
.card.dim{opacity:.45}
.ctrl{background:var(--card);border:1px solid var(--ln);border-radius:12px;padding:14px;margin-bottom:10px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;min-height:44px}
button{font:inherit;color:var(--tx);background:var(--ln);border:1px solid var(--ac);border-radius:10px;min-height:48px;padding:0 16px;cursor:pointer}
.primary{width:100%;background:var(--ac);color:#04120a;font-size:18px;font-weight:700;border:none}
.primary:disabled{opacity:.5;cursor:default}
button:focus-visible,input[type=range]:focus-visible{outline:3px solid var(--ac);outline-offset:2px}
.tg{position:relative;width:76px;height:44px;border-radius:22px;background:var(--ln);border:1px solid var(--ln);flex:0 0 auto}
.tg[aria-checked=true]{background:var(--ac)}
.tg:focus-visible{outline:3px solid var(--ac);outline-offset:2px}
.tg::after{content:"";position:absolute;top:4px;left:4px;width:36px;height:36px;border-radius:50%;background:#fff;transition:left .15s}
.tg[aria-checked=true]::after{left:36px}
input[type=range]{width:100%;height:44px}
.lbl{font-size:15px}
.val{font-weight:700;color:var(--ac)}
.disabled{opacity:.45;pointer-events:none}
.ft{display:flex;justify-content:space-between;font-size:12px;color:var(--mut);margin-top:8px;flex-wrap:wrap;gap:6px}
</style>
</head>
<body>
<div class="hdr">
  <h1>Akilli Sera</h1>
  <div class="st"><span id="dot" class="dot"></span><span id="stt">Baglaniyor...</span></div>
</div>

<div class="grid">
  <div class="card" id="cSic"><h2>Sicaklik</h2><div><span class="big" id="sic">--</span><span class="unit"> &deg;C</span></div></div>
  <div class="card" id="cNem"><h2>Nem</h2><div><span class="big" id="nem">--</span><span class="unit"> %</span></div></div>
  <div class="card"><h2>Toprak Nemi</h2><div><span class="big" id="top">--</span><span class="unit"> %</span></div><div class="bar"><i id="topb"></i></div></div>
  <div class="card"><h2>Isik</h2><div><span class="big" id="isk">--</span><span class="unit"> %</span></div><div class="bar"><i id="iskb"></i></div></div>
</div>

<div class="ctrl">
  <button class="primary" id="sula" onclick="sula()">SULA (5 sn)</button>
</div>

<div class="ctrl">
  <div class="row"><span class="lbl">Mod</span>
    <div style="display:flex;align-items:center;gap:10px">
      <span id="modt" class="val">Otomatik</span>
      <div class="tg" id="mod" role="switch" aria-checked="false" tabindex="0" aria-label="Otomatik veya Manuel mod" onclick="mod()" onkeydown="if(event.key==' '||event.key=='Enter'){event.preventDefault();mod()}"></div>
    </div>
  </div>
</div>

<div class="ctrl" id="mctrl">
  <div class="row"><span class="lbl">Fan</span>
    <div class="tg" id="fan" role="switch" aria-checked="false" tabindex="0" aria-label="Fan ac veya kapa" onclick="fan()" onkeydown="if(event.key==' '||event.key=='Enter'){event.preventDefault();fan()}"></div>
  </div>
  <div class="row" style="display:block"><div style="display:flex;justify-content:space-between"><span class="lbl">Serit LED Parlaklik</span><span class="val" id="ledv">0</span></div>
    <input type="range" id="led" min="0" max="255" value="0" aria-label="LED parlaklik 0 ile 255 arasi"
      oninput="document.getElementById('ledv').textContent=this.value" onchange="ledGonder(this.value)">
  </div>
</div>

<div class="ft"><span id="up">Calisma: --</span><span id="rssi">Sinyal: --</span><span id="son">Guncelleme: --</span></div>

<script>
// Sürüklerken DEĞİL, yalnızca bırakınca (change) istek atılır — aksi halde ESP32
// saniyede onlarca isteğe boğulur. oninput sadece etiketi günceller.
var otomatik=true, hataSay=0;
function q(i){return document.getElementById(i)}
async function post(u,b){try{await fetch(u,{method:'POST',body:JSON.stringify(b)});}catch(e){}await durumCek();}
function sula(){var b=q('sula');post('/api/sula',{});var n=5;b.disabled=true;b.textContent='Sulaniyor '+n+'s';var t=setInterval(function(){n--;if(n<=0){clearInterval(t);b.disabled=false;b.textContent='SULA (5 sn)';}else b.textContent='Sulaniyor '+n+'s';},1000);}
function fan(){if(otomatik)return;post('/api/fan',{acik:q('fan').getAttribute('aria-checked')!='true'});}
function ledGonder(v){if(otomatik)return;post('/api/led',{parlaklik:parseInt(v)});}
function mod(){post('/api/mod',{otomatik:!otomatik});}
function manuelGorunum(){q('mctrl').className='ctrl'+(otomatik?' disabled':'');}
async function durumCek(){
  try{
    var r=await fetch('/api/durum');if(!r.ok)throw 0;var d=await r.json();hataSay=0;
    q('dot').className='dot ok';q('stt').textContent='Bagli';
    var ge=d.dhtGecerli;
    q('sic').textContent=ge?d.sicaklik.toFixed(1):'—';q('nem').textContent=ge?Math.round(d.nem):'—';
    q('cSic').className='card'+(ge?'':' dim');q('cNem').className='card'+(ge?'':' dim');
    q('top').textContent=d.toprakYuzde;q('topb').style.width=d.toprakYuzde+'%';
    q('isk').textContent=d.isikYuzde;q('iskb').style.width=d.isikYuzde+'%';
    q('fan').setAttribute('aria-checked',d.fan?'true':'false');
    var ls=q('led');if(document.activeElement!==ls){ls.value=d.led;q('ledv').textContent=d.led;}
    otomatik=d.otomatik;q('mod').setAttribute('aria-checked',otomatik?'false':'true');
    q('modt').textContent=otomatik?'Otomatik':'Manuel';manuelGorunum();
    q('up').textContent='Calisma: '+Math.floor(d.calismaSuresi/1000)+'s';
    q('rssi').textContent='Sinyal: '+(d.rssi||'--')+' dBm';
    q('son').textContent='Guncelleme: simdi';
  }catch(e){hataSay++;q('dot').className='dot';q('stt').textContent='Baglanti yok';}
}
durumCek();setInterval(durumCek,2000);
</script>
</body>
</html>)HTMLDOC";
