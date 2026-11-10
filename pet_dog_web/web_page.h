#pragma once

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Pet Dog</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#e0e0e0;
  min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:16px}
h1{font-size:1.4rem;margin:12px 0 4px;color:#fff}
.sub{font-size:.8rem;color:#888;margin-bottom:16px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;width:100%;max-width:400px}
.btn{border:none;border-radius:12px;padding:18px 8px;font-size:1rem;font-weight:600;
  cursor:pointer;transition:transform .1s,opacity .1s;color:#fff;text-align:center}
.btn:active{transform:scale(.93);opacity:.7}
.b0{background:#4CAF50}.b1{background:#2196F3}.b2{background:#9C27B0}
.b3{background:#FF9800}.b4{background:#00BCD4}.b5{background:#E91E63}
.bw{background:#607D8B}.bt{background:#795548}
h2{font-size:1rem;margin:20px 0 8px;color:#aaa;width:100%;max-width:400px}
.solo{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;width:100%;max-width:400px}
.bs{background:#455A64;border:none;border-radius:10px;padding:14px 4px;font-size:.85rem;
  font-weight:600;cursor:pointer;transition:transform .1s,opacity .1s;color:#fff}
.bs:active{transform:scale(.93);opacity:.7}
#status{margin-top:16px;font-size:.85rem;color:#666;min-height:1.2em}
</style>
</head>
<body>
<h1>Pet Dog</h1>
<p class="sub">Tap a button to send a command</p>
<div class="grid">
  <button class="btn b0" onclick="cmd(0)">Good Boy</button>
  <button class="btn b1" onclick="cmd(1)">Sit Down</button>
  <button class="btn b2" onclick="cmd(2)">Lie Down</button>
  <button class="btn b3" onclick="cmd(3)">Stretch</button>
  <button class="btn b4" onclick="cmd(4)">Walk</button>
  <button class="btn b5" onclick="cmd(5)">Dance</button>
  <button class="btn bw" onclick="cmd(99)">Wiggle</button>
  <button class="btn bt" onclick="cmd(98)">Test All</button>
</div>
<h2>Solo Servo</h2>
<div class="solo">
  <button class="bs" onclick="cmd(90)">FL</button>
  <button class="bs" onclick="cmd(91)">FR</button>
  <button class="bs" onclick="cmd(92)">BL</button>
  <button class="bs" onclick="cmd(93)">BR</button>
</div>
<p id="status"></p>
<script>
const names={0:'Good Boy',1:'Sit Down',2:'Lie Down',3:'Stretch',4:'Walk',
  5:'Dance',99:'Wiggle',98:'Test All',90:'Solo FL',91:'Solo FR',92:'Solo BL',93:'Solo BR'};
function cmd(id){
  const s=document.getElementById('status');
  s.textContent='Sending: '+(names[id]||id)+'...';
  fetch('/cmd?id='+id).then(r=>r.text()).then(t=>{
    s.textContent=(names[id]||id)+' — '+t;
  }).catch(e=>{s.textContent='Error: '+e;});
}
</script>
</body>
</html>
)rawliteral";
