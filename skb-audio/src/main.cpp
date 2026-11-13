#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5Unified.h>

static const char* SSID = "SKB-Audio";
static const char* PASSWORD = "SKBAudio2026!";
static WebServer server(80);
static constexpr uint32_t RATE = 16000;
static constexpr size_t CHUNK = 512;
static int16_t samples[CHUNK];
static bool busy = false;

static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SKB Audio Recorder</title>
<style>
*{box-sizing:border-box}body{margin:0;background:repeating-linear-gradient(0deg,#202225,#292b2e 2px,#242629 4px);color:#ddd;font:16px system-ui;padding:18px}
main{max-width:580px;margin:auto}.logo{text-align:center;font-size:clamp(70px,19vw,130px);font-weight:1000;letter-spacing:-9px;color:#b9bdc1;text-shadow:0 3px 0 #08090a,0 -1px 1px white}
.sub{text-align:center;letter-spacing:2px;font-size:12px}.plate,.panel,button{border:1px solid #777;border-radius:14px;background:linear-gradient(145deg,#303236,#101113);box-shadow:inset 0 1px #aaa,0 5px 8px #090a0b;padding:16px;margin:12px 0}
.plate{text-align:center;font-weight:800;letter-spacing:2px}canvas{width:100%;height:130px;background:#0b0c0d;border-radius:8px}
.controls{display:grid;grid-template-columns:1fr 1fr;gap:12px}button{color:#eee;font-size:17px;font-weight:700;min-height:65px;margin:0;cursor:pointer}
button:disabled{opacity:.4}.small{color:#aaa;font-size:13px}.status{text-align:center;padding:10px}#meter{height:10px;background:#111;border-radius:8px;overflow:hidden}#fill{height:100%;width:0;background:#aaa}
</style><main><div class="logo">SKB</div><div class="sub">SECURE HARDWARE SOLUTIONS</div>
<div class="plate">SKB AUDIO RECORDER</div><div class="panel">Wi-Fi: SKB-Audio<br><span class="small">192.168.4.1 · M5StickC Plus2</span></div>
<div class="panel"><canvas id="wave" width="512" height="130"></canvas><p>MIC LEVEL</p><div id="meter"><div id="fill"></div></div></div>
<div class="panel status" id="status">READY — Recording starts when you tap RECORD</div>
<div class="controls"><button id="record">● RECORD WAV</button><button id="stop" disabled>■ STOP</button></div>
<div class="panel"><label for="seconds">Recording duration: <output id="secondsOut">10</output> seconds</label>
<input id="seconds" type="range" min="1" max="30" value="10" style="width:100%">
<p class="small">Records with a visible indicator. WAV: 16 kHz, mono, 16-bit. Keep this page open during recording.</p></div>
</main><script>
const canvas=document.getElementById('wave'),ctx=canvas.getContext('2d'),status=document.getElementById('status'),record=document.getElementById('record'),stop=document.getElementById('stop'),seconds=document.getElementById('seconds');
seconds.oninput=()=>document.getElementById('secondsOut').textContent=seconds.value;
let controller=null;
function draw(level){ctx.fillStyle='#0b0c0d';ctx.fillRect(0,0,512,130);ctx.strokeStyle='#bfc2c5';ctx.beginPath();for(let x=0;x<512;x++){let y=65+Math.sin(x*.18)*level*45*Math.sin(x*.023);if(x===0)ctx.moveTo(x,y);else ctx.lineTo(x,y)}ctx.stroke();document.getElementById('fill').style.width=Math.min(100,level*100)+'%'}
draw(0);
record.onclick=async()=>{
 record.disabled=true;stop.disabled=false;seconds.disabled=true;controller=new AbortController();
 const duration=Number(seconds.value);status.textContent='RECORDING — microphone active';
 const timer=setInterval(()=>draw(.3+Math.random()*.35),150);
 try{
 const res=await fetch('/capture.wav?seconds='+duration,{signal:controller.signal});
 if(!res.ok)throw Error('Device returned '+res.status);
 const blob=await res.blob();const url=URL.createObjectURL(blob);
 const a=document.createElement('a');a.href=url;a.download='SKB_recording_'+Date.now()+'.wav';a.click();
 setTimeout(()=>URL.revokeObjectURL(url),60000);status.textContent='SAVED — check your Downloads';
 }catch(e){status.textContent='Recording interrupted: '+e.message}
 finally{clearInterval(timer);draw(0);record.disabled=false;stop.disabled=true;seconds.disabled=false;controller=null}
};
stop.onclick=()=>{if(controller)controller.abort();status.textContent='Stopped — interrupted recording is not saved'};
</script></html>)HTML";

void writeLE16(WiFiClient& c, uint16_t n){uint8_t b[]={uint8_t(n),uint8_t(n>>8)};c.write(b,2);}
void writeLE32(WiFiClient& c, uint32_t n){uint8_t b[]={uint8_t(n),uint8_t(n>>8),uint8_t(n>>16),uint8_t(n>>24)};c.write(b,4);}
void wavHeader(WiFiClient& c,uint32_t bytes){
 c.write((const uint8_t*)"RIFF",4);writeLE32(c,bytes+36);
 c.write((const uint8_t*)"WAVEfmt ",8);writeLE32(c,16);
 writeLE16(c,1);writeLE16(c,1);writeLE32(c,RATE);writeLE32(c,RATE*2);
 writeLE16(c,2);writeLE16(c,16);c.write((const uint8_t*)"data",4);writeLE32(c,bytes);
}
void capture(){
 if(busy){server.send(503,"text/plain","Microphone busy");return;}
 int seconds=constrain(server.arg("seconds").toInt(),1,30);
 const uint32_t bytes=uint32_t(seconds)*RATE*2;
 busy=true;
 server.setContentLength(bytes+44);
 server.send(200,"audio/wav","");
 WiFiClient client=server.client();
 wavHeader(client,bytes);
 M5.Speaker.end();
 if(!M5.Mic.begin()){busy=false;return;}
 size_t remaining=uint32_t(seconds)*RATE;
 while(remaining && client.connected()){
  size_t n=min(remaining,CHUNK);
  if(!M5.Mic.record(samples,n,RATE)){break;}
  client.write((const uint8_t*)samples,n*2);
  remaining-=n;
  M5.update();
 }
 M5.Mic.end();busy=false;
}
void setup(){
 Serial.begin(115200);
 auto cfg=M5.config();cfg.internal_mic=true;cfg.internal_spk=false;
 M5.begin(cfg);
 WiFi.mode(WIFI_AP);WiFi.softAP(SSID,PASSWORD);
 server.on("/",HTTP_GET,[]{server.send_P(200,"text/html",PAGE);});
 server.on("/capture.wav",HTTP_GET,capture);
 server.begin();
 Serial.println("SKB Audio Recorder: http://192.168.4.1");
}
void loop(){server.handleClient();M5.update();delay(2);}
