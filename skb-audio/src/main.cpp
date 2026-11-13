#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5Unified.h>
#include "skb_logo.h"

static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t BLOCK_SAMPLES = 512;
static int16_t pcm[BLOCK_SAMPLES];
static WebServer web(80);
static WiFiServer audioServer(81);
static WiFiClient audioClient;
static uint32_t lastAudioBlock=0;
static constexpr uint32_t AUDIO_INTERVAL_MS=32;
static bool capturing = false;
static bool micReady = false;
static uint32_t micInitAttempts = 0;
static bool ensureMic(){
 if(micReady && M5.Mic.isEnabled())return true;
 ++micInitAttempts;
 M5.Speaker.end();
 M5.Mic.end();
 delay(40);
 // M5StickC Plus2 SPM1423: PDM clock GPIO0, data GPIO34.
 auto micCfg = M5.Mic.config();
 micCfg.pin_bck = 0;
 micCfg.pin_data_in = 34;
 micCfg.pin_ws = -1;
 micCfg.pin_mck = -1;
 micCfg.sample_rate = SAMPLE_RATE;
 micCfg.use_adc = false;
 M5.Mic.config(micCfg);
 micReady = M5.Mic.begin();
 Serial.printf("SKB MIC init attempt=%lu success=%d enabled=%d board=%d\\n",
               (unsigned long)micInitAttempts, micReady, M5.Mic.isEnabled(), (int)M5.getBoard());
 return micReady;
}
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

/* SKB brushed-metal dashboard, mobile-first */
body{background:repeating-linear-gradient(0deg,#1c1e20,#292c2f 2px,#202225 5px);color:#e1e2e4;padding:10px}
main{max-width:800px}.logo{font-size:clamp(88px,24vw,190px);line-height:1.03;letter-spacing:-.105em;margin:10px 0 0;padding-right:.12em;color:#34373a;-webkit-text-stroke:1px #858a8f;text-shadow:0 1px 0 #e1e4e7,1px 3px 0 #070809,0 8px 12px #08090a;font-weight:1000}
.sub{font-size:clamp(10px,2.5vw,22px);font-weight:900;letter-spacing:.05em;color:#aeb2b6;text-shadow:0 2px #000}
.plate{background:linear-gradient(155deg,#56595d 0%,#292b2e 45%,#111214 100%);border:2px solid #969b9e;border-radius:9px;box-shadow:inset 0 2px #c5c9cb,0 4px 6px #090a0b;font-size:clamp(14px,4vw,25px);letter-spacing:2px;margin:17px 8px;padding:13px}
.panel{background:linear-gradient(155deg,#343639,#1b1d20 60%,#0d0e10);border:2px solid #6c7074;border-radius:16px;padding:16px;margin:13px 0;box-shadow:inset 0 2px #a5aaae,0 4px 10px #0b0c0d}
button{background:linear-gradient(#505357 0%,#2c2e31 48%,#121416 100%);border:2px solid #74797d;box-shadow:inset 0 2px #a4a9ad,0 5px 7px #0a0b0c;border-radius:12px;min-height:65px;font-weight:850;letter-spacing:.02em}
button.active{background:linear-gradient(#6a6f74,#303438)}
canvas{height:170px;background:#050607;border:2px solid #777;border-radius:12px;box-shadow:inset 0 0 12px #000}
#heroControls{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px;margin:13px 0}
#heroControls button{min-width:0;padding:8px 3px;font-size:clamp(10px,2.5vw,15px)}
#heroControls #listen:before{content:'◉';display:block;font-size:26px;margin-bottom:5px}
#heroControls #record:before{content:'●';display:block;font-size:27px;margin-bottom:5px}
#heroControls #stop:before{content:'■';display:block;font-size:26px;margin-bottom:5px}
#heroControls #save:before{content:'↓';display:block;font-size:27px;margin-bottom:5px}
#dashboardStatus{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:4px;font-size:clamp(10px,2.5vw,15px);text-align:center}
#dashboardStatus .pill{border-radius:6px;padding:10px 2px;background:linear-gradient(#242628,#0e0f10);overflow-wrap:anywhere}
#recordIndicator{display:flex;justify-content:space-between;align-items:center;padding:12px 18px;background:#090a0b;border:1px solid #777;border-radius:15px;margin:12px 0;font-weight:800;letter-spacing:.08em}
#recordIndicator.recording{color:#eee}#recordIndicator.recording:before{content:'●';color:#e5e5e5;margin-right:8px}
#micMeter{display:flex;align-items:center;gap:8px;margin:12px 0;font-weight:700}
#meterTrack{flex:1;background:#090a0b;border:1px solid #777;border-radius:5px;height:18px;overflow:hidden}
#meterFill{height:100%;width:0;background:repeating-linear-gradient(90deg,#b5b8bc 0,#b5b8bc 8px,#333 9px,#333 12px);transition:width .09s}
#navLinks{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:7px}
#navLinks button{font-size:clamp(10px,2.4vw,14px);padding:5px}
#qualityLabel{font-size:13px;color:#b7bcc0;text-align:right}
</style></head><body><main>
<img id="realLogo" src="/skb-logo.jpg" alt="SKB Secure Hardware Solutions" style="display:none;width:100%;height:auto;border-radius:8px"><div id="textLogo"><div class="logo">SKB</div><div class="sub">SECURE HARDWARE SOLUTIONS</div></div><div class="plate">SKB AUDIO RECORDER</div>
<div class="panel"><div id="dashboardStatus" class="row"><span class="pill" id="connection">DISCONNECTED</span><span class="pill" id="clock">00:00</span><span class="pill" id="battery">BATTERY —</span></div>
<p id="status">Tap LIVE LISTEN to activate microphone streaming. Use with permission.</p>
<div id="heroControls"><button id="listen">LISTEN</button><button id="record" disabled>RECORD</button><button id="stop" disabled>STOP</button><button id="save" disabled>SAVE WAV</button></div><div id="recordIndicator"><span id="recordText">READY</span><span id="recordTime">00:00</span></div><button id="mute" disabled>MUTE</button>
<div id="micMeter"><span>MIC LEVEL</span><div id="meterTrack"><div id="meterFill"></div></div><span id="meterDb">−∞ dB</span></div><label>Playback volume <output id="volLabel">80%</output></label><input id="volume" type="range" min="0" max="100" value="80">
<label>Input gain (digital) <output id="gainLabel">1.0×</output></label><input id="gain" type="range" min="1" max="6" step=".1" value="1">
<div class="row"><label><input type="checkbox" id="noise"> Low-cut / rumble reduction</label><label><input type="checkbox" id="gate"> Adaptive noise gate</label><label><input type="checkbox" id="agc"> Automatic gain control</label></div><div class="row"><label>Gate sensitivity <input id="gateLevel" type="range" min="1" max="8" step="1" value="3"></label><label>AGC target <input id="agcTarget" type="range" min="5" max="35" value="18"></label></div><div class="row"><label>Save <select id="saveMode"><option value="original">Original PCM</option><option value="processed">Full EQ processed</option></select></label></div>
</div>
<div class="panel" id="wavePanel"><h3>LIVE WAVEFORM</h3><canvas id="wave" width="680" height="140"></canvas><small>Real microphone samples · 16 kHz / 16-bit mono</small></div>
<div class="panel" id="spectrumPanel"><h3>LIVE SPECTRUM · TAP A PEAK</h3><canvas id="spectrum" width="680" height="140"></canvas><div class="row"><span>Peak: <b id="peak">—</b></span><span>Selected: <b id="selected">1200 Hz</b></span></div><small>Frequency axis 0–8 kHz. Tap the spectrum to select the nearest detected peak for Band A or B.</small></div>
<div class="panel" id="autotunePanel"><h3>SKB AUTOTUNE · TWO INDEPENDENT BANDS</h3>
<div class="row"><button id="pickA" class="active">SELECT BAND A</button><button id="pickB">SELECT BAND B</button></div>
<div id="bands"></div><div class="row"><button id="auto">AUTO TUNE SELECTED</button><button id="track">AUTO TRACK: OFF</button><button id="bypass">EQ BYPASS: OFF</button></div>
<small>AutoTune centres a peaking EQ filter on the selected frequency. Auto Track follows the strongest detected peak; this can jump between sounds.</small>
</div>
<div class="panel" id="eqPanel"><h3>10-BAND GRAPHIC EQUALIZER</h3><div class="row"><button id="flat">FLAT</button><button id="voice">VOICE CLARITY</button><button id="hum">RUMBLE REDUCTION</button></div><div id="eq"></div></div>
<div class="panel"><h3>STREAM DIAGNOSTICS</h3><p id="diagnostics">No stream started</p><button id="testWav">DOWNLOAD 2-SECOND TEST TONE WAV</button><small>The test tone is generated on your phone and checks WAV export, not the M5 microphone.</small></div>
<div class="panel" id="statusPanel"><h3>STATUS</h3><p id="info">Wi-Fi AP: SKB-Audio · 192.168.4.1</p><button id="reboot">REBOOT DEVICE</button><small>Audio is sent only while Listen is active. WAV files are saved to the phone, not device flash. Maximum recording: 3 minutes.</small></div>
<div id="navLinks"><button onclick="document.getElementById('wavePanel').scrollIntoView()">WAVEFORM</button><button onclick="document.getElementById('spectrumPanel').scrollIntoView()">SPECTRUM</button><button onclick="document.getElementById('autotunePanel').scrollIntoView()">AUTOTUNE</button><button onclick="document.getElementById('statusPanel').scrollIntoView()">STATUS</button></div><p id="qualityLabel">WAV · 16 kHz / 16-bit mono · actual capture format</p></main><script>
'use strict';
const RATE=16000,N=512,$=id=>document.getElementById(id);
let ctx=null,analyser=null,sourceGain=null,volumeNode=null,highpass=null,eqNodes=[],bandNodes=[],outputNode=null;
let listening=false,muted=false,recording=false,recordStart=0,recordOriginal=[],nextTime=0,abort=null;
let chosen=0,selectedHz=1200,track=false,bypass=false,latestPeak=0,lastSpectrum=null,frame=0,failures=0,lastTrack=0,blocks=0,dropouts=0,streamStart=0,noiseFloor=.004,agcGain=1,gateGain=1;
const MAX_RECORD_SECONDS=180;
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
 let chain=[sourceGain,analyser,highpass,...eqNodes,...bandNodes,volumeNode];
 for(let i=0;i<chain.length-1;i++)chain[i].connect(chain[i+1]);
 volumeNode.connect(ctx.destination);outputNode=sourceGain;updateFilters();
}
function condition(samples){
 let energy=0;for(let i=0;i<samples.length;i++)energy+=samples[i]*samples[i];
 const rms=Math.sqrt(energy/samples.length);
 if(rms<noiseFloor*2)noiseFloor=.99*noiseFloor+.01*rms;
 const gateThreshold=Math.max(.002,noiseFloor*Number($('gateLevel').value)*1.5);
 const desiredGate=$('gate').checked&&rms<gateThreshold?.06:1;
 gateGain=gateGain*.85+desiredGate*.15;
 const desiredAgc=$('agc').checked?clamp((Number($('agcTarget').value)/100)/Math.max(.01,rms),.5,8):1;
 agcGain=agcGain*.96+desiredAgc*.04;
 const out=new Float32Array(samples.length);
 for(let i=0;i<out.length;i++)out[i]=clamp(samples[i]*gateGain*agcGain,-1,1);
 return out;
}
function drawWave(samples){
 let energy=0;for(const v of samples)energy+=v*v;
 const rms=Math.sqrt(energy/samples.length),db=rms>0?20*Math.log10(rms):-96;
 $('meterFill').style.width=clamp((db+60)/60*100,0,100)+'%';$('meterDb').textContent=Math.round(db)+' dB';
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
 if(track&&peak>70&&performance.now()-lastTrack>750){lastTrack=performance.now();bands[chosen].hz=Math.round(peak/10)*10;bands[chosen].enabled=true;selectedHz=bands[chosen].hz;updateFilters();$('selected').textContent=fmt(selectedHz)}
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
async function renderProcessed(chunks){
 const length=chunks.reduce((n,a)=>n+a.length,0);
 const offline=new OfflineAudioContext(1,length,RATE);
 const audio=offline.createBuffer(1,length,RATE);
 const data=audio.getChannelData(0);let offset=0;
 for(const part of chunks){data.set(part,offset);offset+=part.length}
 const src=offline.createBufferSource();src.buffer=audio;
 const gain=offline.createGain();gain.gain.value=Number($('gain').value);
 src.connect(gain);let tail=gain;
 if($('noise').checked){const hp=offline.createBiquadFilter();hp.type='highpass';hp.frequency.value=100;tail.connect(hp);tail=hp}
 if(!bypass){
  for(let i=0;i<freqs.length;i++){const v=Number(document.querySelectorAll('[data-eq]')[i].value);if(!v)continue;const n=offline.createBiquadFilter();n.type='peaking';n.frequency.value=freqs[i];n.Q.value=1.2;n.gain.value=v;tail.connect(n);tail=n}
  for(const b of bands){if(!b.enabled)continue;const n=offline.createBiquadFilter();n.type='peaking';n.frequency.value=b.hz;n.Q.value=clamp(b.hz/b.width,.15,30);n.gain.value=b.gain;tail.connect(n);tail=n}
 }
 tail.connect(offline.destination);src.start();
 const result=await offline.startRendering();
 return [result.getChannelData(0).slice()];
}
async function stopRecord(){
 if(!recording)return;recording=false;$('record').disabled=false;$('stop').disabled=true;$('save').disabled=true;$('recordText').textContent='READY';$('recordIndicator').classList.remove('recording');
 const chunks=recordOriginal;recordOriginal=[];
 if(!chunks.length){status('No audio captured');return}
 try{
  status('Preparing WAV for download...');
  download($('saveMode').value==='processed'?await renderProcessed(chunks):chunks);
 }catch(e){status('Processed export failed: '+e.message+'; saving original');download(chunks)}
}
async function pump(){
 while(listening){
  let reader=null;
  try{
   abort=new AbortController();
   const r=await fetch('http://'+location.hostname+':81/live',{cache:'no-store',signal:abort.signal});
   if(!r.ok||!r.body)throw Error('Audio connection failed: '+r.status);
   reader=r.body.getReader();let pending=new Uint8Array(0);
   while(listening){
    const {value,done}=await reader.read();if(done)throw Error('Audio stream ended');
    const combined=new Uint8Array(pending.length+value.length);combined.set(pending);combined.set(value,pending.length);
    let offset=0;
    while(combined.length-offset>=N*2){
     const raw=combined.subarray(offset,offset+N*2);offset+=N*2;
     const view=new DataView(raw.buffer,raw.byteOffset,raw.byteLength),samples=new Float32Array(N);
     for(let i=0;i<N;i++)samples[i]=view.getInt16(i*2,true)/32768;
     drawWave(samples);
     const clean=condition(samples);
     const buffer=ctx.createBuffer(1,N,RATE);buffer.copyToChannel(clean,0);
     const src=ctx.createBufferSource();src.buffer=buffer;src.connect(sourceGain);
     nextTime=Math.max(nextTime,ctx.currentTime+.05);
     if(nextTime-ctx.currentTime>.45)nextTime=ctx.currentTime+.05;
     src.start(nextTime);nextTime+=N/RATE;
     blocks++;if(recording){recordOriginal.push(samples.slice());if((Date.now()-recordStart)/1000>=MAX_RECORD_SECONDS){stopRecord();status('Recording limit reached — WAV saved')}}
    }
    pending=combined.slice(offset);
    failures=0;
   }
  }catch(e){if(!listening)break;failures++;dropouts++;status('Stream error: '+e.message);if(failures>5){await stopListen();break}await new Promise(r=>setTimeout(r,250))}
  finally{if(reader)try{await reader.cancel()}catch(e){}}
 }
}
async function oldPumpUnused(){
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
   blocks++;if(recording){recordOriginal.push(samples.slice());if((Date.now()-recordStart)/1000>=MAX_RECORD_SECONDS){stopRecord();status('Recording limit reached — WAV saved')}}
   failures=0;
  }catch(e){if(!listening)break;failures++;dropouts++;status('Stream error: '+e.message);if(failures>5){await stopListen();break}await new Promise(r=>setTimeout(r,150))}
 }
}
async function startListen(){
 if(listening)return;
 try{initAudio();await ctx.resume();listening=true;noiseFloor=.004;agcGain=1;gateGain=1;blocks=0;dropouts=0;streamStart=Date.now();nextTime=ctx.currentTime+.08;status('LIVE — microphone streaming to phone');$('listen').textContent='■ END LISTEN';$('listen').classList.add('active');$('record').disabled=false;$('mute').disabled=false;$('connection').textContent='LIVE CONNECTED';drawSpectrum();pump()}
 catch(e){status('Audio start failed: '+e.message)}
}
async function stopListen(){
 if(!listening)return;listening=false;if(abort)abort.abort();if(recording)await stopRecord();
 cancelAnimationFrame(frame);$('listen').textContent='▶ LIVE LISTEN';$('listen').classList.remove('active');$('record').disabled=true;$('mute').disabled=true;$('connection').textContent='DISCONNECTED';status('Live audio stopped');
 if(ctx){await ctx.close();ctx=null;analyser=null}
}
$('listen').onclick=()=>listening?stopListen():startListen();
$('mute').onclick=()=>{muted=!muted;$('mute').textContent=muted?'UNMUTE':'MUTE';updateFilters()};
$('record').onclick=()=>{recordOriginal=[];recordStart=Date.now();recording=true;$('record').disabled=true;$('stop').disabled=false;$('save').disabled=false;$('recordText').textContent='RECORDING';$('recordIndicator').classList.add('recording');status('RECORDING — visible recording active')};
$('stop').onclick=stopRecord;
$('save').onclick=stopRecord;
$('volume').oninput=()=>{$('volLabel').textContent=$('volume').value+'%';if(ctx)updateFilters()};
$('gain').oninput=()=>{$('gainLabel').textContent=Number($('gain').value).toFixed(1)+'×';if(ctx)updateFilters()};
$('noise').onchange=updateFilters;
$('spectrum').addEventListener('pointerdown',e=>{
 const r=e.currentTarget.getBoundingClientRect(),target=clamp((e.clientX-r.left)/r.width*8000,80,7900);
 let hz=target;
 if(lastSpectrum){
  const step=RATE/2048,centre=Math.round(target/step);
  const radius=Math.max(4,Math.round(180/step));
  let best=-1;
  for(let i=Math.max(10,centre-radius);i<=Math.min(lastSpectrum.length-2,centre+radius);i++){
   const v=lastSpectrum[i];if(v>best&&v>=lastSpectrum[i-1]&&v>=lastSpectrum[i+1]){best=v;hz=i*step}
  }
 }
 selectedHz=clamp(Math.round(hz/10)*10,80,7900);
 $('selected').textContent=fmt(selectedHz);
});
$('pickA').onclick=()=>pick(0);$('pickB').onclick=()=>pick(1);
function pick(i){chosen=i;$('pickA').classList.toggle('active',i===0);$('pickB').classList.toggle('active',i===1);selectedHz=bands[i].hz;$('selected').textContent=fmt(selectedHz)}
$('auto').onclick=()=>{bands[chosen].hz=selectedHz;bands[chosen].enabled=true;updateFilters();renderBands();status('AutoTune: Band '+(chosen?'B':'A')+' centred on '+fmt(selectedHz))};
$('track').onclick=()=>{track=!track;$('track').textContent='AUTO TRACK: '+(track?'ON':'OFF')};
$('bypass').onclick=()=>{bypass=!bypass;$('bypass').textContent='EQ BYPASS: '+(bypass?'ON':'OFF');updateFilters()};
const eqControls=[];
for(let i=0;i<freqs.length;i++){const f=freqs[i],wrap=makeSlider(f+' Hz',-12,12,1,0,()=>updateFilters());wrap.querySelector('input').dataset.eq='1';$('eq').append(wrap);eqControls.push(wrap.querySelector('input'))}
function preset(values){eqControls.forEach((e,i)=>{e.value=values[i];e.parentElement.querySelector('output').textContent=values[i]});updateFilters()}
$('flat').onclick=()=>preset(freqs.map(()=>0));
$('voice').onclick=()=>preset([-6,-4,-2,0,2,4,3,1,-1,-3]);
$('hum').onclick=()=>{$('noise').checked=true;preset([-12,-8,-4,-2,0,0,0,0,0,0])};
$('reboot').onclick=async()=>{if(confirm('Reboot the M5StickC Plus2?')){await stopListen();fetch('/reboot',{method:'POST'}).catch(()=>{});status('Reboot requested')}};
setInterval(()=>{
 $('clock').textContent=recording?new Date(Date.now()-recordStart).toISOString().slice(14,19):'00:00';$('recordTime').textContent=$('clock').textContent;
 if(listening){const seconds=Math.max(1,(Date.now()-streamStart)/1000);
 $('diagnostics').textContent='Audio blocks: '+blocks+' · errors: '+dropouts+' · capture throughput: '+(blocks*N/seconds/RATE*100).toFixed(0)+'% of real time · recorded: '+Math.round(recordOriginal.length*N/RATE)+'s'}
},500);
$('testWav').onclick=()=>{
 const tone=new Float32Array(RATE*2);
 for(let i=0;i<tone.length;i++)tone[i]=Math.sin(2*Math.PI*440*i/RATE)*.2;
 download([tone]);
};
setInterval(async()=>{try{const s=await (await fetch('/status')).json();$('battery').textContent='BATTERY '+s.battery+'%';$('info').textContent='AP: '+s.ssid+' · IP: '+s.ip+' · Clients: '+s.clients}catch(e){}},4000);
fetch('/logo-status').then(r=>r.json()).then(s=>{if(s.available){$('realLogo').style.display='block';$('textLogo').style.display='none'}}).catch(()=>{});
renderBands();
</script></body></html>
)SKBHTML";

static void handleChunk(){
 if(capturing){web.send(503,"text/plain","busy");return;}
 capturing=true;
 M5.Speaker.end();
 if(!micReady)ensureMic();
 if(!micReady){
  capturing=false;web.send(500,"text/plain","mic init failed");return;
 }
 bool ok=M5.Mic.record(pcm,BLOCK_SAMPLES,SAMPLE_RATE);
 if(!ok){micReady=false;M5.Mic.end();capturing=false;web.send(500,"text/plain","mic capture failed");return;}
 web.setContentLength(BLOCK_SAMPLES*2);
 web.sendHeader("Cache-Control","no-store");
 web.send(200,"application/octet-stream","");
 WiFiClient c=web.client();
 size_t sent=0;
 while(sent<BLOCK_SAMPLES*2 && c.connected()){
  size_t n=c.write(((const uint8_t*)pcm)+sent,BLOCK_SAMPLES*2-sent);
  if(!n)break;
  sent+=n;
 }
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
 M5.Speaker.end();
 ensureMic();
 WiFi.mode(WIFI_AP);
 WiFi.softAP(AP_NAME,AP_PASSWORD);
 web.on("/",HTTP_GET,[]{web.send_P(200,"text/html",PAGE);});
 web.on("/logo-status",HTTP_GET,[]{web.send(200,"application/json",SKB_LOGO_LENGTH>0?"{\"available\":true}":"{\"available\":false}");});
 web.on("/skb-logo.jpg",HTTP_GET,[]{
  if(!SKB_LOGO_LENGTH){web.send(404,"text/plain","logo not embedded");return;}
  web.setContentLength(SKB_LOGO_LENGTH);
  web.send(200,"image/jpeg","");
  WiFiClient client=web.client();
  for(size_t i=0;i<SKB_LOGO_LENGTH;i+=512){
   uint8_t chunk[512];size_t n=SKB_LOGO_LENGTH-i;if(n>sizeof(chunk))n=sizeof(chunk);
   memcpy_P(chunk,SKB_LOGO_DATA+i,n);client.write(chunk,n);
  }
 });
 web.on("/chunk",HTTP_GET,handleChunk);
 web.on("/mic-diagnostic",HTTP_GET,[]{
  bool ready=ensureMic();
  String json=String("{\\\"mic_initialized\\\":")+(ready?"true":"false")
    +",\\\"mic_enabled\\\":"+(M5.Mic.isEnabled()?"true":"false")
    +",\\\"attempts\\\":"+String(micInitAttempts)
    +",\\\"board\\\":"+String((int)M5.getBoard())
    +",\\\"clock_pin\\\":0,\\\"data_pin\\\":34}";
  web.send(200,"application/json",json);
 });
 web.on("/status",HTTP_GET,handleStatus);
 web.on("/reboot",HTTP_POST,[]{web.send(200,"text/plain","rebooting");delay(100);ESP.restart();});
 web.begin();
 audioServer.begin();
 Serial.println("SKB Audio Lab v1.1: http://192.168.4.1");
}
void loop(){
 web.handleClient();
 M5.update();
 if(!audioClient || !audioClient.connected()){
  if(audioClient)audioClient.stop();
  WiFiClient candidate=audioServer.available();
  if(candidate){
   candidate.setTimeout(100);
   String request=candidate.readStringUntil('\n');
   if(request.startsWith("GET /live ")){
    while(candidate.available())candidate.read();
    candidate.print("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nCache-Control: no-store\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n");
    audioClient=candidate;
    lastAudioBlock=millis();
   }else{candidate.print("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");candidate.stop();}
  }
 }
 if(audioClient && audioClient.connected() && millis()-lastAudioBlock>=AUDIO_INTERVAL_MS){
  lastAudioBlock=millis();
  if(!micReady)ensureMic();
  if(micReady && M5.Mic.record(pcm,BLOCK_SAMPLES,SAMPLE_RATE)){
   if(audioClient.availableForWrite()>=BLOCK_SAMPLES*2)audioClient.write((const uint8_t*)pcm,BLOCK_SAMPLES*2);
  }
 }
 delay(1);
}
