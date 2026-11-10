#pragma once
// Mobile-first Mambo OS theme. Loaded as /mambo.css so it remains editable
// without changing the existing control and routing JavaScript.
static const char MAMBO_MOBILE_CSS[] PROGMEM = R"CSS(
:root {--ink:#e8f7ff;--muted:#9bb7ca;--teal:#43e2ff;--pink:#e95cf9;--panel:#111b2b;--border:#2b5474;--blue:#36cfff;--silver:#ecfaff;color-scheme:dark}
html,body {background:radial-gradient(ellipse at 22% 0%,#0c3949 0,transparent 42%),radial-gradient(ellipse at 100% 20%,#38183f 0,transparent 45%),#060a12;color:var(--ink);font-family:system-ui,-apple-system,Roboto,sans-serif}
main {max-width:890px;padding:8px 10px 10px;gap:2px}
.hero,.panel,.skb-page article {background:linear-gradient(153deg,rgba(22,43,62,.95),rgba(11,17,31,.98));border:1px solid rgba(88,183,231,.35);border-radius:19px;padding:16px;box-shadow:0 12px 28px #0006,inset 0 1px 0 #ffffff16}
.hero {text-align:left;border-top:1px solid #3de0ff;display:flex;align-items:center;gap:14px;position:relative;overflow:hidden}
.hero img {height:90px;width:90px;object-fit:cover;border-radius:20px;border:1px solid #57e8ff}
.hero h1 {font-size:clamp(20px,5vw,29px);letter-spacing:.5px;line-height:1.1}
.hero:after {display:none}
h1,h2,h3 {color:#f1faff;letter-spacing:.25px}
.tag,.skb-page-title span {color:var(--teal)}
small,.muted {color:var(--muted)}
button {background:linear-gradient(140deg,#1f4863,#15253c 75%);border:1px solid #4182a0;border-bottom:2px solid #43cafa;border-radius:13px;color:#f8fcff;min-height:46px;box-shadow:0 3px 12px #07111b;font-size:13px;font-weight:750}
button:active {background:#215a68;transform:translateY(1px)}
button:focus-visible,input:focus-visible,select:focus-visible {outline:3px solid #dd66fa;outline-offset:2px}
button:disabled {opacity:.48}
input,select,textarea {background:#091524!important;border:1px solid #355b77!important;color:#f8fcff!important;border-radius:11px!important;min-height:42px;padding:10px!important}
input[type=file]{padding:10px!important}
pre {background:#060d19;border:1px solid #26465f;border-radius:12px;color:#bce9ff;max-height:290px}
a {color:var(--teal)}
.skb-tabs {gap:8px;padding:7px 2px 12px}
.skb-tabs button {background:#111e30;border-color:#294861;border-radius:15px;min-height:46px}
.skb-tabs button[aria-selected=true] {background:linear-gradient(115deg,#12728e,#68418a);border:1px solid #4de4fc;box-shadow:0 0 16px #2ed4f128}
.skb-page-title {background:linear-gradient(90deg,#11273c,#151629);border:1px solid #2c5a7a;border-radius:12px;top:0;padding:12px}
.skb-page-title strong {font-size:15px}
.skb-pager {background:#080e1a;border-top:1px solid #375472;border-radius:12px}
.skb-page {padding-bottom:24px}
.skb-page article {margin-bottom:12px}
.mambo-brand-strip {display:grid;grid-template-columns:1fr 1.4fr .9fr;align-items:center;gap:9px;background:linear-gradient(120deg,#0c2036,#151123);padding:10px;border-radius:18px;border:1px solid #2a6585;margin:0 0 12px}
.mambo-brand-strip img {width:100%;height:90px;object-fit:contain;border-radius:12px;background:#060b14}
.mambo-brand-strip img:first-child{object-fit:cover}
.mambo-brand-strip small{display:block;text-align:center;font-size:10px;color:#8eaac0;margin-top:3px}
.mambo-button-grid {display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;margin:10px 0}
.mambo-button-grid button {width:100%}
.mambo-data-actions {display:flex;gap:8px;flex-wrap:wrap}
.mambo-data-actions button {flex:1}
.mambo-status-text{font-size:12px;color:#7bdffa;min-height:20px}
.mambo-radio-bars{display:grid;grid-template-columns:repeat(8,minmax(0,1fr));gap:5px;align-items:end;height:102px;padding:10px;background:#080f1c;border:1px solid #294763;border-radius:12px;margin:10px 0}
.mambo-radio-bars span{display:block;background:linear-gradient(0deg,#3cbaff,#e95af9);min-height:6px;border-radius:5px 5px 2px 2px}
@media(max-width:420px){.hero img{width:70px;height:70px}.mambo-brand-strip img{height:66px}.mambo-button-grid{grid-template-columns:1fr 1fr}.skb-tabs button{font-size:11px;padding:8px 10px}.grid{grid-template-columns:1fr 1fr}}
)CSS";
