const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const firmware = fs.readFileSync('src/main.cpp','utf8');
const html = firmware.split('R"SKBHTML(\n')[1].split('\n)SKBHTML"')[0];
assert.equal(fs.readFileSync('data/index.html','utf8'),html+'\n','embedded and filesystem UI must match');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements = new Map();
const element = id => {
  if (!elements.has(id)) elements.set(id,{value:'',textContent:'',style:{},files:[]});
  return elements.get(id);
};
const calls=[];
let configFailure=false;
const context = {
  document:{getElementById:element,querySelector:element},
  TextEncoder, console, setInterval(){},setTimeout(){},
  async fetch(url,options={}){
    calls.push({url,options});
    const ok=!(configFailure && url==='/api/config' && options.method==='POST');
    let value={ok:true};
    if(url==='/api/config') value={ok,label:'Lab',scan_dwell_ms:120,error:ok?undefined:'settings_write_failed'};
    if(url==='/api/wifi/channels') value={channels:[{channel:1,overlap_score:100}]};
    if(url==='/api/wifi/devices') value={access_points:[{ssid:'<script>alert(1)</script>'}],local_ap_clients:[]};
    return {ok,status:ok?200:500,json:async()=>value};
  }
};
vm.createContext(context);vm.runInContext(script,context);
(async()=>{
  await context.loadConfig();assert.equal(element('device-label').value,'Lab');
  await context.wifiDetails();assert.match(element('wifi-details').textContent,/overlap_score/);
  assert.match(element('wifi-details').textContent,/<script>/); // rendered via textContent
  element('device-label').value='Kitchen';element('scan-dwell').value='200';
  await context.saveConfig();assert.equal(element('config-result').textContent,'Settings saved');
  const saved=calls.filter(c=>c.options.method==='POST').at(-1);
  assert.deepEqual(JSON.parse(saved.options.body),{label:'Kitchen',scan_dwell_ms:200});
  element('scan-dwell').value='0';const before=calls.length;
  await context.saveConfig();assert.equal(calls.length,before);assert.match(element('config-result').textContent,/100/);
  element('scan-dwell').value='200';configFailure=true;
  await context.saveConfig();assert.match(element('config-result').textContent,/settings_write_failed/);
  console.log('Dashboard: configuration, diagnostics rendering, validation, failure reporting and UI parity passed');
})().catch(e=>{console.error(e);process.exitCode=1});
