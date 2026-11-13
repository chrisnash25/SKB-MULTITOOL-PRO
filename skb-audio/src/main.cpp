#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5Unified.h>

static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t BLOCK_SAMPLES = 512;
static int16_t pcm[BLOCK_SAMPLES];
static WebServer web(80);
static bool capturing = false;
static const char *AP_NAME = "SKB-Audio";
static const char *AP_PASSWORD = "SKBAudio2026!";

static const char PAGE[] PROGMEM = R"SKBHTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SKB Audio Lab</title><style>
:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:repeating-linear-gradient(0deg,#202225,#292b2e 2px,#242629 4px);color:#e1e3e5;font:15px system-ui,sans-serif;padding:12px}
main{max-width:740px;margin:auto}.logo{text-align:center;font-size:clamp(65px,17vw,115px);font-weight:900;letter-spacing:-8px;color:#bfc3c7;text-shadow:0 3px #090a0b,0 -1px #fff}
.sub{text-align:center;letter-spacing:2px;font-size:10px}.panel,.plate,button{border:1px solid #686d71;border-radius:12px;background:linear-gradient(145deg,#34373a,#151719);box-shadow:inset 0 1px #8b9094,0 4px 9px #0a0b0c;padding:13px;margin:10px 0}
.plate{text-align:center;font-weight:800;letter-spacing:2px}.row{display:flex;gap:9px;align-items:center;flex-wrap:wrap}.row>*{flex:1}
button{color:#eee;font-weight:700;font-size:14px;min-height:45px;cursor:pointer;margin:3px 0}button.active{background:linear-gradient(#777,#40464b)}button:disabled{opacity:.4}
canvas{display:block;width:100%;height:140px;background:#0b0d0e;border:1px solid #444;border-radius:6px;touch-action:none}
label{display:block;margin:9px 0 3px}input[type=range]{width:100%;accent-color:#c2c6ca}
small,.hint{color:#aeb3b7}output{float:right}h3{margin:6px 0 10px}select{background:#222;color:white;padding:8px;border:1px solid #777;border-radius:6px}
#status{font-size:13px;min-height:20px}.pill{padding:4px 9px;border:1px solid #777;border-radius:20px;font-size:12px}
</style></head><body><main>
<div class="logo">SKB</div><div class="sub">SECURE HARDWARE SOLUTIONS</div><div class="plate">AUDIO LAB · LIVE LISTEN / RECORD / SPECTRUM</div>
<div class="panel"><div class="row"><span class="pill" id="connection">DISCONNECTED</span><span class="pill" id="clock">00:00</span><span class="pill" id="battery">BATTERY —</span></div>
<p id="status">Tap LIVE LISTEN to activate microphone streaming. Use with permission.</p>
<div class="row"><button id="listen">▶ LIVE LISTEN</button><button id="mute" disabled>MUTE</button><button id="record" disabled>● RECORD</button><button id="stop" disabled>■ STOP & SAVE</button></div>
<label>Playback volume <output id="volLabel">80%</output></label><input id="volume" type="range" min="0" max="100" value="80">
<label>Input gain (digital) <output id="gainLabel">1.0×</output></label><input id="gain" type="range" min="1" max="6" step=".1" value="1">
<div class="row"><label><input type="checkbox" id="noise"> Low-cut / rumble reduction</label><label>Save <select id="saveMode"><option value="original">Original PCM</option><option value="processed">Processed playback</option></select></label></div>
</div>
<div class="panel"><h3>LIVE WAVEFORM</h3><canvas id="wave" width="680" height="140"></canvas><small>Real microphone samples · 16 kHz / 16-bit mono</small></div>
<div class="panel"><h3>LIVE SPECTRUM · TAP A PEAK</h3><canvas id="spectrum" width="680" height="140"></canvas><div class="row"><span>Peak: <b id="peak">—</b></span><span>Selected: <b id="selected">1200 Hz</b></span></div><small>Frequency axis 0–8 kHz. Tap the spectrum to select a frequency for Band A or B.</small></div>
<div class="panel"><h3>SKB AUTOTUNE · TWO INDEPENDENT BANDS</h3>
<div class="row"><button id="pickA" class="active">SELECT BAND A</button><button id="pickB">SELECT BAND B</button></div>
<div id="bands"></div><div class="row"><button id="auto">AUTO TUNE SELECTED</button><button id="track">AUTO TRACK: OFF</button><button id="bypass">EQ BYPASS: OFF</button></div>
<small>AutoTune centres a peaking EQ filter on the selected frequency. Auto Track follows the strongest detected peak; this can jump between sounds.</small>
</div>
<div class="panel"><h3>10-BAND GRAPHIC EQUALIZER</h3><div class="row"><button id="flat">FLAT</button><button id="voice">VOICE CLARITY</button><button id="hum">RUMBLE REDUCTION</button></div><div id="eq"></div></div>
<div class="panel"><h3>STATUS</h3><p id="info">Wi-Fi AP: SKB-Audio · 192.168.4.1</p><button id="reboot">REBOOT DEVICE</button><small>Audio is sent only while Listen is active. WAV files are saved to the phone, not device flash.</small></div>
</main><script>
'use strict';
const RATE=16000,N=512,$=id=>document.getElementById(id);
let ctx=null,analyser=null,sourceGain=null,volumeNode=null,highpass=null,eqNodes=[],bandNodes=[],outputNode=null;
let listening=false,muted=false,recording=false,recordStart=0,recordOriginal=[],recordProcessed=[],nextTime=0,abort=null;
let chosen=0,selectedHz=1200,track=false,bypass=false,latestPeak=0,lastSpectrum=null,frame=0,failures=0;
const freqs=[80,160,315,630,1000,2000,3150,4000,6300,7900];
const bands=[{hz:1200,gain:6,width:400,enabled:false},{hz:2400,gain:6,width:500,enabled:false}];
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const fmt=v=>Math.round(v).toLocaleString()+' Hz';
function status(s){$('status').textContent=s}
function makeSlider(label,min,max,step,value,cb){
 const wrap=document.createElement('div'),lab=document.createElement('label'),out=document.createElement('output'),input=document.createElement('input');
 lab.textContent=label;out.textContent=value;lab.append(out);input.type='range';input.min=min;input.max=max;input.step=step;input.value=value;
 input.oninput=()=>{out.textContent=input.value;cb(Number(input.value))};wrap.append(lab,input);return wrap;
}
function renderBands(){
 $('bands').replaceChildren();
 bands.forEach((b,i)=>{
 const box=document.createElement('div');box.className='panel';
 const head=document.createElement('div');head.className='row';
 const name=document.createElement('b');name.textContent='BAND '+(i?'B':'A');
 const toggle=document.createElement('button');toggle.textContent=b.enabled?'ENABLED':'DISABLED';toggle.className=b.enabled?'active':'';
 toggle.onclick=()=>{b.enabled=!b.enabled;updateFilters();renderBands()};
 head.append(name,toggle);box.append(head);
 box.append(makeSlider('Centre frequency',80,7900,10,b.hz,v=>{b.hz=v;selectedHz=v;updateFilters();$('selected').textContent=fmt(v)}));
 box.append(makeSlider('Boost / cut (dB)',-12,12,1,b.gain,v=>{b.gain=v;updateFilters()}));
 box.append(makeSlider('Bandwidth (Hz)',50,2000,50,b.width,v=>{b.width=v;updateFilters()}));
 $('bands').append(box);
 });
}
function updateFilters(){
 if(!ctx)return;
 const t=ctx.currentTime;
 highpass.frequency.setTargetAtTime($('noise').checked?100:20,t,.03);
 eqNodes.forEach((f,i)=>f.gain.setTargetAtTime(bypass?0:Number(document.querySelectorAll('[data-eq]')[i].value),t,.03));
 bandNodes.forEach((f,i)=>{const b=bands[i];f.frequency.setTargetAtTime(b.hz,t,.03);f.Q.setTargetAtTime(clamp(b.hz/b.width,.15,30),t,.03);f.gain.setTargetAtTime(bypass||!b.enabled?0:b.gain,t,.03)});
 sourceGain.gain.setTargetAtTime(Number($('gain').value),t,.03);
 volumeNode.gain.setTargetAtTime(muted?0:Number($('volume').value)/100,t,.03);
}
function initAudio(){
 ctx=new (window.AudioContext||window.webkitAudioContext)({sampleRate:RATE});
 sourceGain=ctx.createGain();highpass=ctx.createBiquadFilter();highpass.type='highpass';
 eqNodes=freqs.map(f=>{let n=ctx.createBiquadFilter();n.type='peaking';n.frequency.value=f;n.Q.value=1.2;return n});
 bandNodes=bands.map(b=>{let n=ctx.createBiquadFilter();n.type='peaking';n.frequency.value=b.hz;return n});
 volumeNode=ctx.createGain();analyser=ctx.createAnalyser();analyser.fftSize=2048;analyser.smoothingTimeConstant=.72;
 let chain=[sourceGain,highpass,...eqNodes,...bandNodes,analyser,volumeNode];
 for(let i=0;i<chain.length-1;i++)chain[i].connect(chain[i+1]);
 volumeNode.connect(ctx.destination);outputNode=sourceGain;updateFilters();
}
function drawWave(samples){
 const c=$('wave'),g=c.getContext('2d');g.fillStyle='#0b0d0e';g.fillRect(0,0,c.width,c.height);g.strokeStyle='#d5d9dc';g.beginPath();
 for(let i=0;i<samples.length;i++){const x=i/(samples.length-1)*c.width,y=c.height/2-samples[i]*c.height*.44;i?g.lineTo(x,y):g.moveTo(x,y)}g.stroke();
}
function drawSpectrum(){
 if(!listening||!analyser)return;
 const bins=new Uint8Array(analyser.frequencyBinCount);analyser.getByteFrequencyData(bins);lastSpectrum=bins;
 const c=$('spectrum'),g=c.getContext('2d');g.fillStyle='#0b0d0e';g.fillRect(0,0,c.width,c.height);
 let best=0,peak=0;
 for(let x=1;x<c.width;x+=2){const hz=x/c.width*8000,idx=clamp(Math.round(hz/(RATE/2)*bins.length),0,bins.length-1),v=bins[idx];if(hz>70&&v>best){best=v;peak=hz}g.fillStyle='#9ba3aa';g.fillRect(x,c.height-v/255*c.height,2,v/255*c.height)}
 latestPeak=peak;$('peak').textContent=fmt(peak);
 if(track&&peak>70){bands[chosen].hz=Math.round(peak/10)*10;bands[chosen].enabled=true;selectedHz=bands[chosen].hz;updateFilters();$('selected').textContent=fmt(selectedHz)}
 const x=selectedHz/8000*c.width;g.strokeStyle='#fff';g.beginPath();g.moveTo(x,0);g.lineTo(x,c.height);g.stroke();
 frame=requestAnimationFrame(drawSpectrum);
}
function pcmToWav(chunks){
 const count=chunks.reduce((n,a)=>n+a.length,0),buf=new ArrayBuffer(44+count*2),view=new DataView(buf);
 function ascii(p,s){for(let i=0;i<s.length;i++)view.setUint8(p+i,s.charCodeAt(i))}
 ascii(0,'RIFF');view.setUint32(4,36+count*2,true);ascii(8,'WAVE');ascii(12,'fmt ');view.setUint32(16,16,true);view.setUint16(20,1,true);view.setUint16(22,1,true);
 view.setUint32(24,RATE,true);view.setUint32(28,RATE*2,true);view.setUint16(32,2,true);view.setUint16(34,16,true);ascii(36,'data');view.setUint32(40,count*2,true);
 let offset=44;for(const chunk of chunks){for(let i=0;i<chunk.length;i++){view.setInt16(offset,Math.round(clamp(chunk[i],-1,1)*32767),true);offset+=2}}
 return new Blob([buf],{type:'audio/wav'});
}
function download(chunks){
 if(!chunks.length){status('No audio captured');return}
 const url=URL.createObjectURL(pcmToWav(chunks)),a=document.createElement('a');a.href=url;a.download='SKB_'+new Date().toISOString().replace(/[:.]/g,'-')+'.wav';document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),60000);
 status('WAV saved to phone Downloads');
}
function stopRecord(){
 if(!recording)return;recording=false;$('record').disabled=false;$('stop').disabled=true;
 const chunks=$('saveMode').value==='processed'?recordProcessed:recordOriginal;download(chunks);recordOriginal=[];recordProcessed=[];
}
async function pump(){
 while(listening){
  try{
   abort=new AbortController();
   const r=await fetch('/chunk',{cache:'no-store',signal:abort.signal});if(!r.ok)throw Error('HTTP '+r.status);
   const raw=await r.arrayBuffer();if(raw.byteLength!==N*2)throw Error('Incomplete audio block');
   const ints=new Int16Array(raw),samples=new Float32Array(N);
   for(let i=0;i<N;i++)samples[i]=ints[i]/32768;
   drawWave(samples);
   const buffer=ctx.createBuffer(1,N,RATE);buffer.copyToChannel(samples,0);
   const src=ctx.createBufferSource();src.buffer=buffer;src.connect(sourceGain);
   nextTime=Math.max(nextTime,ctx.currentTime+.045);
   if(nextTime-ctx.currentTime>.5)nextTime=ctx.currentTime+.05;
   src.start(nextTime);nextTime+=N/RATE;
   if(recording){recordOriginal.push(samples.slice());const processed=samples.map(v=>clamp(v*Number($('gain').value),-1,1));recordProcessed.push(processed)}
   failures=0;
  }catch(e){if(!listening)break;failures++;status('Stream error: '+e.message);if(failures>5){await stopListen();break}await new Promise(r=>setTimeout(r,150))}
 }
}
async function startListen(){
 if(listening)return;
 try{initAudio();await ctx.resume();listening=true;nextTime=ctx.currentTime+.08;status('LIVE — microphone streaming to phone');$('listen').textContent='■ END LISTEN';$('listen').classList.add('active');$('record').disabled=false;$('mute').disabled=false;$('connection').textContent='LIVE CONNECTED';drawSpectrum();pump()}
 catch(e){status('Audio start failed: '+e.message)}
}
async function stopListen(){
 if(!listening)return;listening=false;if(abort)abort.abort();if(recording)stopRecord();
 cancelAnimationFrame(frame);$('listen').textContent='▶ LIVE LISTEN';$('listen').classList.remove('active');$('record').disabled=true;$('mute').disabled=true;$('connection').textContent='DISCONNECTED';status('Live audio stopped');
 if(ctx){await ctx.close();ctx=null;analyser=null}
}
$('listen').onclick=()=>listening?stopListen():startListen();
$('mute').onclick=()=>{muted=!muted;$('mute').textContent=muted?'UNMUTE':'MUTE';updateFilters()};
$('record').onclick=()=>{recordOriginal=[];recordProcessed=[];recordStart=Date.now();recording=true;$('record').disabled=true;$('stop').disabled=false;status('RECORDING — visible recording active')};
$('stop').onclick=stopRecord;
$('volume').oninput=()=>{$('volLabel').textContent=$('volume').value+'%';if(ctx)updateFilters()};
$('gain').oninput=()=>{$('gainLabel').textContent=Number($('gain').value).toFixed(1)+'×';if(ctx)updateFilters()};
$('noise').onchange=updateFilters;
$('spectrum').addEventListener('pointerdown',e=>{const r=e.currentTarget.getBoundingClientRect();selectedHz=clamp(Math.round((e.clientX-r.left)/r.width*8000/10)*10,80,7900);$('selected').textContent=fmt(selectedHz)});
$('pickA').onclick=()=>pick(0);$('pickB').onclick=()=>pick(1);
function pick(i){chosen=i;$('pickA').classList.toggle('active',i===0);$('pickB').classList.toggle('active',i===1);selectedHz=bands[i].hz;$('selected').textContent=fmt(selectedHz)}
$('auto').onclick=()=>{bands[chosen].hz=selectedHz;bands[chosen].enabled=true;updateFilters();renderBands();status('AutoTune: Band '+(chosen?'B':'A')+' centred on '+fmt(selectedHz))};
$('track').onclick=()=>{track=!track;$('track').textContent='AUTO TRACK: '+(track?'ON':'OFF')};
$('bypass').onclick=()=>{bypass=!bypass;$('bypass').textContent='EQ BYPASS: '+(bypass?'ON':'OFF');updateFilters()};
const eqControls=[];
for(let i=0;i<freqs.length;i++){const f=freqs[i],wrap=makeSlider(f+' Hz',-12,12,1,0,()=>updateFilters());wrap.querySelector('input').dataset.eq='1';$('eq').append(wrap);eqControls.push(wrap.querySelector('input'))}
function preset(values){eqControls.forEach((e,i)=>{e.value=values[i];e.previousSibling.querySelector('output').textContent=values[i]});updateFilters()}
$('flat').onclick=()=>preset(freqs.map(()=>0));
$('voice').onclick=()=>preset([-6,-4,-2,0,2,4,3,1,-1,-3]);
$('hum').onclick=()=>{$('noise').checked=true;preset([-12,-8,-4,-2,0,0,0,0,0,0])};
$('reboot').onclick=async()=>{if(confirm('Reboot the M5StickC Plus2?')){await stopListen();fetch('/reboot',{method:'POST'}).catch(()=>{});status('Reboot requested')}};
setInterval(()=>{$('clock').textContent=recording?new Date(Date.now()-recordStart).toISOString().slice(14,19):'00:00'},250);
setInterval(async()=>{try{const s=await (await fetch('/status')).json();$('battery').textContent='BATTERY '+s.battery+'%';$('info').textContent='AP: '+s.ssid+' · IP: '+s.ip+' · Clients: '+s.clients}catch(e){}},4000);
renderBands();
</script></body></html>
)SKBHTML";

static void handleChunk(){
 if(capturing){web.send(503,"text/plain","busy");return;}
 capturing=true;
 M5.Speaker.end();
 if(!M5.Mic.isRunning() && !M5.Mic.begin()){
  capturing=false;web.send(500,"text/plain","mic init failed");return;
 }
 bool ok=M5.Mic.record(pcm,BLOCK_SAMPLES,SAMPLE_RATE);
 if(!ok){capturing=false;web.send(500,"text/plain","mic capture failed");return;}
 web.setContentLength(BLOCK_SAMPLES*2);
 web.send(200,"application/octet-stream","");
 WiFiClient c=web.client();
 c.write((const uint8_t*)pcm,BLOCK_SAMPLES*2);
 capturing=false;
}
static void handleStatus(){
 String json="{\"ssid\":\""+String(AP_NAME)+"\",\"ip\":\""+WiFi.softAPIP().toString()+"\",\"clients\":"+String(WiFi.softAPgetStationNum())+",\"battery\":"+String(M5.Power.getBatteryLevel())+"}";
 web.send(200,"application/json",json);
}
void setup(){
 Serial.begin(115200);
 auto cfg=M5.config();
 cfg.internal_mic=true;cfg.internal_spk=false;
 M5.begin(cfg);
 WiFi.mode(WIFI_AP);
 WiFi.softAP(AP_NAME,AP_PASSWORD);
 web.on("/",HTTP_GET,[]{web.send_P(200,"text/html",PAGE);});
 web.on("/chunk",HTTP_GET,handleChunk);
 web.on("/status",HTTP_GET,handleStatus);
 web.on("/reboot",HTTP_POST,[]{web.send(200,"text/plain","rebooting");delay(100);ESP.restart();});
 web.begin();
 Serial.println("SKB Audio Lab v1.1: http://192.168.4.1");
}
void loop(){web.handleClient();M5.update();delay(1);}
