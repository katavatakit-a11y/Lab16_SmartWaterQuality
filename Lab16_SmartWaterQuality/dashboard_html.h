#pragma once
/* ============================================================================
 *  หน้าเว็บ Dashboard ของ Smart Water Quality Sensor
 *  เก็บทั้งหน้าไว้ใน PROGMEM (flash) ไม่กิน RAM ที่ต้องเหลือไว้ให้ WiFi และ MQTT
 *
 *  ไม่เรียกอะไรจากอินเทอร์เน็ตเลย ทั้ง CSS JavaScript และฟอนต์ Prompt
 *  อยู่ในตัวบอร์ดทั้งหมด (ฟอนต์อยู่ใน fonts_data.h)
 *  ใช้งานได้เต็มรูปแบบแม้ต่อผ่าน AP ของบอร์ดที่ไม่มีสัญญาณเน็ต
 *
 *  ---------------------------- แนวคิดการออกแบบ ----------------------------
 *  โปรเจกต์นี้ต่างจาก Lab ก่อน ๆ ตรงที่ "ตัวเลขอย่างเดียวไม่พอ"
 *  คนดูหน้านี้ส่วนใหญ่ไม่ได้จำช่วง pH หรือ EC ที่เหมาะสมไว้ในหัว
 *  เห็น pH 8.7 แล้วก็ยังไม่รู้ว่าต้องทำอะไร
 *
 *  หน้านี้จึงแปลงตัวเลขเป็นคำตัดสินและภาพก่อนเสมอ
 *    1. แถบคำตัดสินรวมด้านบน  ดี / เฝ้าระวัง / ผิดปกติ  เห็นตั้งแต่วินาทีแรก
 *    2. เกจของแต่ละค่า แสดงช่วงที่เหมาะสมเป็นแถบเขียว พร้อมเข็มชี้ตำแหน่งปัจจุบัน
 *    3. ตัวเลขดิบยังอยู่ครบ สำหรับคนที่ต้องการค่าจริงไปใช้ต่อ
 *
 *  ช่วงเกณฑ์ทั้งหมดรับมาจาก /api/status ไม่ได้เขียนตายไว้ในหน้าเว็บ
 *  เพื่อให้แก้เกณฑ์ในโค้ดแล้วหน้าเว็บเปลี่ยนตามทันที ไม่หลุดจากกัน
 * ==========================================================================*/

const char DASHBOARD_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="th">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Water Quality (BOI-SUT)</title>
<style>
/* ---- ฟอนต์ Prompt เสิร์ฟจากตัวบอร์ดเอง (ดู fonts_data.h) ---- */
@font-face{font-family:'Prompt';font-style:normal;font-weight:400;font-display:swap;
  src:url(/fonts/prompt-400-thai.woff2) format('woff2');
  unicode-range:U+02D7,U+0303,U+0331,U+0E01-0E5B,U+200C-200D,U+25CC}
@font-face{font-family:'Prompt';font-style:normal;font-weight:400;font-display:swap;
  src:url(/fonts/prompt-400-latin.woff2) format('woff2');
  unicode-range:U+0000-00FF,U+0131,U+0152-0153,U+02BB-02BC,U+02C6,U+02DA,U+02DC,U+0304,U+0308,U+0329,U+2000-206F,U+20AC,U+2122,U+2191,U+2193,U+2212,U+2215,U+FEFF,U+FFFD}
@font-face{font-family:'Prompt';font-style:normal;font-weight:700;font-display:swap;
  src:url(/fonts/prompt-700-thai.woff2) format('woff2');
  unicode-range:U+02D7,U+0303,U+0331,U+0E01-0E5B,U+200C-200D,U+25CC}
@font-face{font-family:'Prompt';font-style:normal;font-weight:700;font-display:swap;
  src:url(/fonts/prompt-700-latin.woff2) format('woff2');
  unicode-range:U+0000-00FF,U+0131,U+0152-0153,U+02BB-02BC,U+02C6,U+02DA,U+02DC,U+0304,U+0308,U+0329,U+2000-206F,U+20AC,U+2122,U+2191,U+2193,U+2212,U+2215,U+FEFF,U+FFFD}

/* =====================================================================
   ธีม Aqua Green (โทนสว่าง) — เขียวน้ำ ให้ความรู้สึกเรื่องน้ำสะอาด
   สีทั้งหมดอยู่ที่ :root จุดเดียว แก้โทนทั้งหน้าได้จากตรงนี้
   ===================================================================== */
:root{
  --bg:#f1f8f7; --bg2:#e0f0ef; --card:#fff; --line:#cfe6e3;
  --txt:#12362f; --dim:#5b7d76;
  --acc:#0d9488; --acc-dk:#0f766e; --acc-lt:#ccfbf1;
  --good:#16a34a; --good-lt:#dcfce7;
  --warn:#d97706; --warn-lt:#fef3c7;
  --bad:#dc2626;  --bad-lt:#fee2e2;
  --ph:#7c3aed; --ec:#0284c7; --wt:#ea580c;
  --mono:#0f766e;
  --shadow:0 2px 10px rgba(13,148,136,.08);
}
*{box-sizing:border-box;margin:0;padding:0}
body{
  font-family:'Prompt','Segoe UI',system-ui,-apple-system,'Sarabun',sans-serif;
  background:
    radial-gradient(900px 420px at 10% -10%,#d3edea 0%,transparent 60%),
    radial-gradient(700px 380px at 95% 0%,#e6f5f3 0%,transparent 55%),
    var(--bg);
  color:var(--txt);min-height:100vh;padding:18px;
}
.wrap{max-width:1080px;margin:0 auto}

/* ---------- หัวเรื่อง ---------- */
header{
  display:flex;flex-wrap:wrap;align-items:center;gap:14px;
  padding:20px 24px;margin-bottom:14px;border-radius:18px;
  background:linear-gradient(120deg,var(--acc-dk) 0%,var(--acc) 55%,#14b8a6 100%);
  color:#fff;box-shadow:0 6px 22px rgba(15,118,110,.22);
}
h1{font-size:1.4rem;font-weight:700;letter-spacing:.2px;line-height:1.2}
h1 .org{
  display:inline-block;margin-left:8px;font-size:.76rem;font-weight:700;
  padding:3px 10px;border-radius:999px;background:rgba(255,255,255,.18);
  border:1px solid rgba(255,255,255,.35);vertical-align:middle;letter-spacing:.6px;
}
.sub{font-size:.79rem;opacity:.9;margin-top:4px}
.clock{margin-left:auto;text-align:right;line-height:1.2}
.clock b{font-size:1.7rem;font-variant-numeric:tabular-nums;letter-spacing:1px}
.clock small{opacity:.85;font-size:.77rem}
.pills{display:flex;gap:8px;flex-wrap:wrap;margin-top:9px}
.pill{
  font-size:.71rem;font-weight:600;padding:5px 12px;border-radius:999px;
  border:1px solid rgba(255,255,255,.35);background:rgba(255,255,255,.14);color:#fff;
}
.pill.on{background:#fff;border-color:#fff;color:var(--acc-dk)}
.pill.no{background:rgba(220,38,38,.85);border-color:rgba(255,255,255,.3);color:#fff}

/* ---------- แถบคำตัดสินรวม : สิ่งแรกที่ตาไปโดน ---------- */
#verdict{
  display:flex;align-items:center;gap:16px;flex-wrap:wrap;
  padding:16px 22px;margin-bottom:16px;border-radius:18px;
  border:2px solid var(--good);background:var(--good-lt);
}
#verdict .vmark{
  width:52px;height:52px;border-radius:50%;flex-shrink:0;
  display:flex;align-items:center;justify-content:center;
  font-size:1.6rem;font-weight:700;color:#fff;background:var(--good);
}
#verdict .vtxt b{display:block;font-size:1.15rem;font-weight:700;color:var(--good)}
#verdict .vtxt span{font-size:.82rem;color:var(--dim)}
#verdict.watch{border-color:var(--warn);background:var(--warn-lt)}
#verdict.watch .vmark{background:var(--warn)}
#verdict.watch .vtxt b{color:#92400e}
#verdict.bad{border-color:var(--bad);background:var(--bad-lt);animation:pulse 1.4s ease-in-out infinite}
#verdict.bad .vmark{background:var(--bad)}
#verdict.bad .vtxt b{color:#991b1b}
@keyframes pulse{0%,100%{box-shadow:0 0 0 0 rgba(220,38,38,.28)}50%{box-shadow:0 0 0 12px rgba(220,38,38,0)}}

/* ---------- แถบเตือนโหมดจำลอง ---------- */
#simbar{
  display:none;align-items:center;gap:10px;margin-bottom:16px;
  padding:11px 18px;border-radius:13px;font-size:.82rem;
  background:#fff7ed;border:1px solid #fdba74;color:#9a3412;
}
#simbar.show{display:flex}

/* ---------- การ์ด ---------- */
.grid{display:grid;gap:16px;margin-bottom:16px}
.g3{grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
.g2{grid-template-columns:repeat(auto-fit,minmax(330px,1fr))}
.gr{grid-template-columns:repeat(auto-fit,minmax(300px,1fr))}
.card{
  background:var(--card);border:1px solid var(--line);border-radius:18px;
  padding:18px 20px;box-shadow:var(--shadow);
}
.card h3{
  display:flex;align-items:center;gap:9px;
  font-size:.79rem;font-weight:700;color:var(--dim);
  text-transform:uppercase;letter-spacing:1.1px;margin-bottom:12px;
}
.src{
  margin-left:auto;font-size:.61rem;font-weight:700;letter-spacing:.7px;
  padding:3px 9px;border-radius:6px;text-transform:none;
}
.src.real{background:var(--good-lt);color:#15803d;border:1px solid #86efac}
.src.sim {background:var(--warn-lt);color:#92400e;border:1px solid #fcd34d}

/* ---------- ค่าหลัก + เกจ ---------- */
.metric{display:flex;align-items:flex-end;gap:9px}
.metric .val{font-size:2.8rem;font-weight:700;line-height:1;font-variant-numeric:tabular-nums}
.metric .unit{font-size:1rem;color:var(--dim);padding-bottom:6px}
.metric .tag{
  margin-left:auto;font-size:.7rem;font-weight:700;padding:4px 10px;border-radius:999px;
  background:var(--good-lt);color:#15803d;
}
.metric .tag.w{background:var(--warn-lt);color:#92400e}
.metric .tag.b{background:var(--bad-lt);color:#991b1b}
.card.ph .val{color:var(--ph)} .card.ec .val{color:var(--ec)} .card.wt .val{color:var(--wt)}
.card.ph{background:#faf7ff;border-color:#e4d9fb}
.card.ec{background:#f2f9fe;border-color:#cfe6f5}
.card.wt{background:#fff7f2;border-color:#f6ddc9}

/* เกจ : แถบโซนสี + เข็มชี้ตำแหน่งค่าปัจจุบัน
   นี่คือส่วนที่ทำให้ตัวเลขมีความหมายโดยไม่ต้องจำเกณฑ์เอง */
.gauge{margin-top:16px;position:relative;height:34px}
.gauge .zones{
  position:absolute;left:0;right:0;top:9px;height:10px;border-radius:99px;
  overflow:hidden;display:flex;background:var(--bg2);
}
.gauge .zones i{display:block;height:100%}
.gauge .zones i.b{background:#fca5a5}
.gauge .zones i.w{background:#fcd34d}
.gauge .zones i.o{background:#4ade80}
.gauge .pin{
  position:absolute;top:0;width:3px;height:28px;border-radius:2px;
  background:var(--txt);transform:translateX(-1.5px);transition:left .5s ease;
}
.gauge .pin::after{
  content:'';position:absolute;top:-4px;left:50%;transform:translateX(-50%);
  border:5px solid transparent;border-top-color:var(--txt);
}
.gauge .ticks{
  position:absolute;left:0;right:0;top:22px;
  display:flex;justify-content:space-between;font-size:.63rem;color:var(--dim);
}

/* ---------- ค่ารอง ---------- */
.mini{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:14px}
.mini div{background:#fff;border:1px solid var(--line);border-radius:10px;padding:8px;text-align:center}
.mini b{display:block;font-size:1rem;font-weight:700;font-variant-numeric:tabular-nums}
.mini span{display:block;font-size:.62rem;color:var(--dim);margin-top:2px}

/* ---------- แถวข้อมูลประกอบ ---------- */
.rows{display:grid;gap:8px}
.row2{
  display:flex;align-items:center;justify-content:space-between;
  padding:9px 12px;background:var(--bg);border:1px solid var(--line);border-radius:10px;
}
.row2 span{font-size:.8rem;color:var(--dim)}
.row2 b{font-size:.92rem;font-variant-numeric:tabular-nums}

/* ---------- การ์ดรีเลย์ ---------- */
.rhead{display:flex;align-items:center;gap:10px;margin-bottom:6px}
.rhead h2{font-size:1.02rem;font-weight:700}
.rhead small{display:block;font-size:.7rem;color:var(--dim);font-weight:400}
.badge{
  margin-left:auto;font-size:.71rem;font-weight:700;letter-spacing:.6px;
  padding:5px 13px;border-radius:999px;background:#d3ddda;color:#3f524a;
}
.badge.on{background:var(--acc);color:#fff;box-shadow:0 0 14px rgba(13,148,136,.4)}
.tabs{display:flex;gap:6px;background:var(--bg2);padding:4px;border-radius:12px;margin:12px 0}
.tabs button{
  flex:1;padding:8px 4px;font-size:.79rem;font-weight:700;cursor:pointer;
  border:0;border-radius:9px;background:transparent;color:var(--dim);
  transition:.18s;font-family:inherit;
}
.tabs button.sel{background:var(--acc);color:#fff;box-shadow:0 2px 8px rgba(13,148,136,.32)}
.tabs button:hover:not(.sel){color:var(--txt);background:rgba(255,255,255,.7)}
.pane{display:none;animation:fade .25s ease}
.pane.show{display:block}
@keyframes fade{from{opacity:0;transform:translateY(-4px)}to{opacity:1}}

.row{display:flex;align-items:center;gap:9px;margin-bottom:10px;flex-wrap:wrap}
.row label{font-size:.79rem;color:var(--dim);min-width:74px;font-weight:600}
input,select{
  background:#fff;border:1px solid var(--line);border-radius:9px;
  color:var(--txt);padding:8px 10px;font-size:.87rem;font-family:inherit;
}
input:focus,select:focus{outline:0;border-color:var(--acc);box-shadow:0 0 0 3px var(--acc-lt)}
input[type=number]{width:88px}
input[type=time]{width:118px}
select{flex:1;min-width:96px}
.btn{
  width:100%;padding:11px;margin-top:6px;border:0;border-radius:11px;cursor:pointer;
  font-size:.87rem;font-weight:700;font-family:inherit;transition:.18s;
  background:var(--acc);color:#fff;box-shadow:0 2px 8px rgba(13,148,136,.3);
}
.btn:hover{background:var(--acc-dk)}
.btn.gray{background:var(--bg2);color:var(--txt);box-shadow:none;border:1px solid var(--line)}
.btn.gray:hover{background:#cfe6e3}
.toggle{display:flex;gap:9px}
.toggle .btn{margin-top:0}
.hint{font-size:.73rem;color:var(--dim);margin-top:9px;line-height:1.5}
.hint b{color:var(--txt);font-family:Consolas,monospace}

/* ---------- MQTT / เครือข่าย ---------- */
.topics{display:grid;gap:7px}
.topic{
  display:flex;align-items:center;gap:10px;font-size:.77rem;
  background:var(--bg);border:1px solid var(--line);border-radius:9px;padding:8px 12px;
}
.topic .k{color:var(--dim);min-width:84px;flex-shrink:0;font-weight:600}
.topic .v{font-family:Consolas,monospace;color:var(--mono);word-break:break-all}
.topic.cmd{background:#fffbeb;border-color:#fde68a}
.topic.cmd .v{color:#b45309}
.meta{display:flex;flex-wrap:wrap;gap:18px;font-size:.77rem;color:var(--dim);margin-bottom:14px}
.meta b{color:var(--txt);font-family:Consolas,monospace}
.netk{font-size:.71rem;color:var(--dim);text-transform:uppercase;letter-spacing:.8px;margin-bottom:5px;font-weight:700}
.netv{font-size:1.02rem;font-weight:700;font-family:Consolas,monospace;color:var(--mono);word-break:break-all}

footer{text-align:center;color:var(--dim);font-size:.74rem;padding:22px 0 8px}
.dead{opacity:.45;pointer-events:none}
</style>
</head>
<body>
<div class="wrap">

  <header>
    <div>
      <h1>Smart Water Quality <span class="org">BOI-SUT</span></h1>
      <div class="sub">ระบบเฝ้าระวังและปรับคุณภาพน้ำอัตโนมัติ &middot; ESP32 Devkit V2</div>
      <div class="pills">
        <span class="pill on" id="pAp">AP</span>
        <span class="pill" id="pWifi">WiFi</span>
        <span class="pill" id="pMqtt">MQTT</span>
      </div>
    </div>
    <div class="clock">
      <b id="clk">--:--:--</b><br>
      <small id="dat">รอเวลาจาก NTP</small>
    </div>
  </header>

  <!-- คำตัดสินรวม : สิ่งแรกที่ผู้ใช้เห็น -->
  <div id="verdict">
    <div class="vmark" id="vMark">?</div>
    <div class="vtxt"><b id="vTitle">กำลังอ่านค่า</b><span id="vDetail">เชื่อมต่อกับบอร์ด...</span></div>
  </div>

  <div id="simbar"><b>โหมดจำลอง</b><span id="simtxt"></span></div>

  <!-- ค่าหลักของคุณภาพน้ำ พร้อมเกจแสดงช่วงที่เหมาะสม -->
  <div class="grid g3">
    <div class="card ph">
      <h3>ค่า pH <span class="src" id="srcQ">-</span></h3>
      <div class="metric">
        <span class="val" id="vPh">--</span><span class="unit">pH</span>
        <span class="tag" id="tPh">-</span>
      </div>
      <div class="gauge" id="gPh"></div>
    </div>
    <div class="card ec">
      <h3>ค่าการนำไฟฟ้า EC</h3>
      <div class="metric">
        <span class="val" id="vEc">--</span><span class="unit">&micro;S/cm</span>
        <span class="tag" id="tEc">-</span>
      </div>
      <div class="gauge" id="gEc"></div>
      <div class="mini">
        <div><b id="vTds">--</b><span>TDS</span></div>
        <div><b id="vSalt">--</b><span>ความเค็ม</span></div>
        <div><b id="vRatio">--</b><span>TDS/EC</span></div>
      </div>
    </div>
    <div class="card wt">
      <h3>อุณหภูมิน้ำ</h3>
      <div class="metric">
        <span class="val" id="vWt">--</span><span class="unit">&deg;C</span>
        <span class="tag" id="tWt">-</span>
      </div>
      <div class="gauge" id="gWt"></div>
    </div>
  </div>

  <!-- ชุดปรับคุณภาพน้ำ -->
  <div class="grid gr" id="relays"></div>

  <!-- ข้อมูลประกอบ + สุขภาพเซนเซอร์ -->
  <div class="grid g2">
    <div class="card">
      <h3>สภาพแวดล้อม</h3>
      <div class="rows">
        <div class="row2"><span>ความชื้นดิน</span><b id="eSoil">--</b></div>
        <div class="row2"><span>ความเข้มแสงอาทิตย์</span><b id="eRad">--</b></div>
        <div class="row2"><span>อุณหภูมิอากาศ</span><b id="eTemp">--</b></div>
        <div class="row2"><span>ความชื้นอากาศ</span><b id="eHumi">--</b></div>
      </div>
    </div>
    <div class="card">
      <h3>สถานะเซนเซอร์</h3>
      <div class="rows" id="sensors"></div>
      <p class="hint">เซนเซอร์ทั้ง 3 ตัวอยู่บนสาย RS-485 เส้นเดียว อ่านทีละตัววนรอบทุก 2 วินาที</p>
    </div>
  </div>

  <!-- เครือข่าย -->
  <div class="card" style="margin-bottom:16px">
    <h3>เครือข่าย</h3>
    <div class="grid g3" style="margin-bottom:0">
      <div>
        <div class="netk">WiFi ของบอร์ด (AP)</div>
        <div class="netv" id="nApSsid">-</div>
        <div class="hint" style="margin-top:4px">IP <b id="nApIp">-</b> &middot; เชื่อมต่ออยู่ <b id="nApCl">0</b> เครื่อง</div>
      </div>
      <div>
        <div class="netk">WiFi บ้าน (STA)</div>
        <div class="netv" id="nStaIp">ไม่ได้เชื่อมต่อ</div>
        <div class="hint" style="margin-top:4px" id="nStaInfo">-</div>
      </div>
      <div>
        <div class="netk">เปิดหน้านี้ได้ที่</div>
        <div class="netv" id="nUrl1">-</div>
        <div class="hint" style="margin-top:4px" id="nUrl2"></div>
      </div>
    </div>
  </div>

  <!-- MQTT -->
  <div class="card">
    <h3>MQTT Topic</h3>
    <div class="meta">
      <span>Broker <b id="mHost">-</b></span>
      <span>Client ID <b id="mCid">-</b></span>
      <span>ส่งสำเร็จ <b id="mTx">0</b> ครั้ง</span>
    </div>
    <div class="topics" id="topics"></div>
    <p class="hint">
      แถบเขียวคือ topic ที่บอร์ดส่งออก &nbsp;|&nbsp; แถบเหลืองคือ topic ที่รับคำสั่ง
      ส่ง <b>on</b> / <b>off</b> / <b>toggle</b> ได้ &nbsp;|&nbsp;
      <b>/verdict</b> ส่งคำตัดสินเป็น GOOD / WATCH / BAD แบบ retained ใช้ตั้งแจ้งเตือนได้เลย
    </p>
  </div>

  <footer>Smart Water Quality (BOI-SUT) &middot; ESP32 Devkit V2 &middot; Lab16 &middot; uptime <span id="up">0</span></footer>
</div>

<script>
const MODE_TH = ['สั่งเอง','ตั้งเวลา','อัตโนมัติ'];
/* ลำดับต้องตรงกับ enum SRC_* ในไฟล์ .ino เป๊ะ ๆ
   เพราะค่า index คือตัวเลขที่ส่งผ่าน /api/config?src= */
const SRC_TH  = ['อุณหภูมิอากาศ','ความชื้นอากาศ','ความชื้นดิน','ความเข้มแสง','pH น้ำ','EC น้ำ'];
const GRADE   = [
  {k:'good', mark:'✓', t:'คุณภาพน้ำดี',   d:'ทุกค่าอยู่ในช่วงที่เหมาะสม'},
  {k:'watch',mark:'!',      t:'ควรเฝ้าระวัง',  d:'มีค่าที่เริ่มออกนอกช่วงที่ต้องการ'},
  {k:'bad',  mark:'✕', t:'คุณภาพน้ำผิดปกติ', d:'ต้องแก้ไขทันที'}
];
let built = false;

/* ---- วาดเกจ : แถบโซนสี + เข็มชี้ค่าปัจจุบัน ----
   โซนแบ่งจากเกณฑ์ที่บอร์ดส่งมา ไม่ได้เขียนตายไว้ในหน้าเว็บ
   แก้เกณฑ์ในโค้ดแล้วเกจจะเปลี่ยนตามเองทันที */
function gauge(id, v, lo, hi, ok, bad, fmt){
  const pc = x => Math.max(0, Math.min(100, (x - lo) / (hi - lo) * 100));
  const b0 = pc(bad[0]), o0 = pc(ok[0]), o1 = pc(ok[1]), b1 = pc(bad[1]);
  const el = document.getElementById(id);
  el.innerHTML =
    `<div class="zones">
       <i class="b" style="width:${b0}%"></i>
       <i class="w" style="width:${o0-b0}%"></i>
       <i class="o" style="width:${o1-o0}%"></i>
       <i class="w" style="width:${b1-o1}%"></i>
       <i class="b" style="width:${100-b1}%"></i>
     </div>
     <div class="pin" style="left:${pc(v)}%"></div>
     <div class="ticks"><span>${fmt(lo)}</span><span>ช่วงที่เหมาะสม ${fmt(ok[0])} - ${fmt(ok[1])}</span><span>${fmt(hi)}</span></div>`;
}

function tag(id, g){
  const el = document.getElementById(id);
  el.className = 'tag' + (g === 1 ? ' w' : (g === 2 ? ' b' : ''));
  el.textContent = g === 0 ? 'ปกติ' : (g === 1 ? 'เฝ้าระวัง' : 'ผิดปกติ');
}

function buildCards(rs, roles){
  document.getElementById('relays').innerHTML = rs.map((r,i)=>`
   <div class="card">
    <div class="rhead">
      <h2>${roles[i]}<small>Relay ${r.id}</small></h2>
      <span class="badge" id="bd${r.id}">OFF</span>
    </div>
    <div class="tabs" id="tb${r.id}">
      ${MODE_TH.map((m,j)=>`<button onclick="pick(${r.id},${j})">${m}</button>`).join('')}
    </div>
    <div class="pane" id="p${r.id}0">
      <div class="toggle">
        <button class="btn" onclick="cmd(${r.id},'on')">เปิด</button>
        <button class="btn gray" onclick="cmd(${r.id},'off')">ปิด</button>
      </div>
      <p class="hint">สั่งงานด้วยมือ ระบบตัดให้อัตโนมัติถ้าเปิดค้างเกิน 2 นาที
        เพราะปั๊มจ่ายสารเคมีจ่ายเกินแล้วแก้ยาก</p>
    </div>
    <div class="pane" id="p${r.id}1">
      <div class="row"><label>เริ่มเวลา</label><input type="time" id="tm${r.id}" value="06:00"></div>
      <div class="row"><label>นานกี่นาที</label><input type="number" id="rn${r.id}" min="1" max="720" value="2"></div>
      <button class="btn" onclick="save(${r.id})">บันทึกตารางเวลา</button>
      <p class="hint">ทำงานซ้ำทุกวันตามเวลาที่ตั้ง ใช้นาฬิกาจริงจาก NTP</p>
    </div>
    <div class="pane" id="p${r.id}2">
      <div class="row"><label>ดูค่าจาก</label>
        <select id="sc${r.id}">${SRC_TH.map((s,j)=>`<option value="${j}">${s}</option>`).join('')}</select></div>
      <div class="row"><label>เงื่อนไข</label>
        <select id="ab${r.id}"><option value="1">มากกว่า</option><option value="0">น้อยกว่า</option></select>
        <input type="number" id="th${r.id}" step="0.1" value="8.5"></div>
      <div class="row"><label>ช่วงหน่วง</label>
        <input type="number" id="hy${r.id}" step="0.1" min="0.1" value="0.3">
        <span class="hint" style="margin:0">กันจ่ายรัวรอบจุดตัด</span></div>
      <button class="btn" onclick="save(${r.id})">บันทึกเงื่อนไข</button>
      <p class="hint" id="au${r.id}"></p>
    </div>
   </div>`).join('');
  built = true;
}

function pick(id,m){
  showPane(id,m);
  fetch('/api/config',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:`id=${id}&mode=${m}`}).then(load);
}
function showPane(id,m){
  document.querySelectorAll(`#tb${id} button`).forEach((b,i)=>b.classList.toggle('sel',i===m));
  for(let i=0;i<3;i++) document.getElementById(`p${id}${i}`).classList.toggle('show',i===m);
}
function cmd(id,s){ fetch(`/api/relay?id=${id}&state=${s}`,{method:'POST'}).then(load); }
function save(id){
  const t = document.getElementById('tm'+id).value.split(':');
  const q = new URLSearchParams({
    id:id, onH:t[0], onM:t[1],
    runMin:document.getElementById('rn'+id).value,
    src:document.getElementById('sc'+id).value,
    above:document.getElementById('ab'+id).value,
    thr:document.getElementById('th'+id).value,
    hyst:document.getElementById('hy'+id).value });
  fetch('/api/config',{method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:q.toString()}).then(load);
}

function pill(el,ok,txt){ el.className='pill '+(ok?'on':'no'); el.textContent=txt; }
function srcTag(el,real){ el.className='src '+(real?'real':'sim'); el.textContent=real?'REAL':'SIM'; }

/* แปลรหัสข้อผิดพลาดของ Modbus เป็นสาเหตุที่ชี้ทางแก้ได้ */
const MB_ERR = {
  0x00:'ปกติ', 0x01:'ไม่รองรับฟังก์ชัน 0x03', 0x02:'ไม่มีรีจิสเตอร์นี้',
  0x03:'จำนวนรีจิสเตอร์ไม่ถูกต้อง', 0x04:'เซนเซอร์แจ้งว่าทำงานผิดพลาด',
  0xE0:'คำตอบมาจาก ID อื่น — มี ID ซ้ำบนสาย', 0xE1:'ฟังก์ชันไม่ถูกต้อง',
  0xE2:'ไม่มีคำตอบ — ตรวจ ID, ไฟเลี้ยง, สาย A/B',
  0xE3:'ข้อมูลเพี้ยน — baud ไม่ตรง หรือสลับ A/B'
};
const mbErr = c => MB_ERR[c] || ('รหัส 0x'+c.toString(16).toUpperCase());

async function load(){
  let d;
  try{ d = await (await fetch('/api/status')).json(); }
  catch(e){ document.body.classList.add('dead'); return; }
  document.body.classList.remove('dead');

  // ----- นาฬิกาและสถานะเชื่อมต่อ -----
  document.getElementById('clk').textContent = d.synced ? d.time.substr(11) : '--:--:--';
  document.getElementById('dat').textContent = d.synced ? d.time.substr(0,10) : 'รอเวลาจาก NTP';
  pill(document.getElementById('pAp'),   true,   'AP ' + d.apClients + ' เครื่อง');
  pill(document.getElementById('pWifi'), d.wifi, 'WiFi บ้าน ' + (d.wifi ? d.rssi+' dBm' : 'ไม่ได้ต่อ'));
  pill(document.getElementById('pMqtt'), d.mqtt, 'MQTT ' + (d.mqtt ? 'ออนไลน์' : 'ออฟไลน์'));

  // ----- คำตัดสินรวม -----
  const g = GRADE[d.grade];
  const vb = document.getElementById('verdict');
  vb.className = d.grade === 0 ? '' : g.k;
  document.getElementById('vMark').textContent  = g.mark;
  document.getElementById('vTitle').textContent = g.t;
  const bad = [];
  if(d.gPh) bad.push('pH');
  if(d.gEc) bad.push('EC');
  if(d.gWt) bad.push('อุณหภูมิน้ำ');
  document.getElementById('vDetail').textContent =
    d.grade === 0 ? g.d : g.d + ' — ค่าที่ต้องดู: ' + bad.join(', ');

  // ----- แถบเตือนโหมดจำลอง -----
  const miss = [];
  if(!d.wqReal)   miss.push('คุณภาพน้ำ (ID3)');
  if(!d.soilReal) miss.push('ความชื้นดิน (ID1)');
  if(!d.radReal)  miss.push('ความเข้มแสง (ID2)');
  if(!d.dhtReal)  miss.push('DHT11');
  document.getElementById('simbar').classList.toggle('show', miss.length>0);
  if(miss.length) document.getElementById('simtxt').textContent =
    'ไม่พบ ' + miss.join(' · ') + ' ระบบใช้ค่าจำลองแทน ค่าที่แสดงไม่ใช่ค่าวัดจริง';

  // ----- ค่าหลัก + เกจ -----
  document.getElementById('vPh').textContent = d.ph.toFixed(2);
  document.getElementById('vEc').textContent = d.ec.toFixed(0);
  document.getElementById('vWt').textContent = d.wtemp.toFixed(1);
  tag('tPh', d.gPh); tag('tEc', d.gEc); tag('tWt', d.gWt);
  srcTag(document.getElementById('srcQ'), d.wqReal);

  gauge('gPh', d.ph,    0,  14,   d.lim.phOk, d.lim.phBad, x=>x.toFixed(1));
  gauge('gEc', d.ec,    0,  2000, d.lim.ecOk, d.lim.ecBad, x=>x.toFixed(0));
  gauge('gWt', d.wtemp, 10, 40,   d.lim.wtOk, d.lim.wtBad, x=>x.toFixed(0));

  document.getElementById('vTds').textContent   = d.tds.toFixed(0);
  document.getElementById('vSalt').textContent  = d.salt.toFixed(0);
  document.getElementById('vRatio').textContent = d.ec > 0 ? (d.tds/d.ec).toFixed(2) : '--';

  // ----- สภาพแวดล้อม -----
  document.getElementById('eSoil').textContent = d.soil.toFixed(0) + ' %';
  document.getElementById('eRad').textContent  = d.rad.toFixed(0) + ' W/m²';
  document.getElementById('eTemp').textContent = d.temp.toFixed(1) + ' °C';
  document.getElementById('eHumi').textContent = d.humi.toFixed(0) + ' %RH';

  // ----- สถานะเซนเซอร์ -----
  const S = [
    ['ID1 ความชื้นดิน', d.soilReal, d.soilOk, d.soilErr, d.soilErrCode],
    ['ID2 ความเข้มแสง', d.radReal,  d.radOk,  d.radErr,  d.radErrCode],
    ['ID3 คุณภาพน้ำ',   d.wqReal,   d.wqOk,   d.wqErr,   d.wqErrCode]
  ];
  document.getElementById('sensors').innerHTML = S.map(([n,ok,o,e,c])=>
    `<div class="row2"><span>${n}</span><b style="color:${ok?'var(--good)':'var(--warn)'}">${
      ok ? `ปกติ · ${o}/${e}` : 'SIM · ' + mbErr(c)}</b></div>`).join('');

  // ----- รีเลย์ -----
  if(!built){
    buildCards(d.relays, d.roles);
    d.relays.forEach(r=>{
      document.getElementById('tm'+r.id).value =
        String(r.onH).padStart(2,'0')+':'+String(r.onM).padStart(2,'0');
      document.getElementById('rn'+r.id).value = r.runMin;
      document.getElementById('sc'+r.id).value = r.src;
      document.getElementById('ab'+r.id).value = r.above?1:0;
      document.getElementById('th'+r.id).value = r.thr;
      document.getElementById('hy'+r.id).value = r.hyst;
      showPane(r.id, r.mode);
    });
  }
  d.relays.forEach(r=>{
    const b = document.getElementById('bd'+r.id);
    b.className = 'badge' + (r.on?' on':'');
    const rm = r.remain>99 ? Math.ceil(r.remain/60)+' น.' : r.remain+' วิ';
    b.textContent = r.on ? (r.remain>0 ? 'ON '+rm : 'ON') : 'OFF';
    const el = document.getElementById('au'+r.id);
    if(el) el.textContent =
      `เปิดเมื่อ ${SRC_TH[r.src]} ${r.above?'ถึง':'ลดถึง'} ${r.thr}`
      + ` และปิดเมื่อ${r.above?'ต่ำกว่า':'สูงกว่า'} ${(r.above?r.thr-r.hyst:r.thr+r.hyst).toFixed(1)}`;
  });

  // ----- เครือข่าย -----
  document.getElementById('nApSsid').textContent = d.apSsid;
  document.getElementById('nApIp').textContent   = d.apIp;
  document.getElementById('nApCl').textContent   = d.apClients;
  if(d.wifi){
    document.getElementById('nStaIp').textContent   = d.ip;
    document.getElementById('nStaInfo').textContent = d.staSsid + ' · ' + d.rssi + ' dBm';
    document.getElementById('nUrl1').textContent    = 'http://' + d.ip;
    document.getElementById('nUrl2').textContent    = d.mdns ? 'หรือ http://' + d.mdns + '.local' : '';
  } else {
    document.getElementById('nStaIp').textContent   = 'ไม่ได้เชื่อมต่อ';
    document.getElementById('nStaInfo').textContent =
      d.staSsid ? 'กำลังหา ' + d.staSsid : 'ปิดใช้งาน STA';
    document.getElementById('nUrl1').textContent    = 'http://' + d.apIp;
    document.getElementById('nUrl2').textContent    = 'ผ่าน WiFi ของบอร์ดเท่านั้น';
  }

  // ----- MQTT -----
  document.getElementById('mHost').textContent = d.mq.host+':'+d.mq.port;
  document.getElementById('mCid').textContent  = d.mq.clientId;
  document.getElementById('mTx').textContent   = d.tx;
  document.getElementById('topics').innerHTML =
    d.mq.pub.map(t=>`<div class="topic"><span class="k">ส่งออก</span><span class="v">${t}</span></div>`).join('')+
    d.mq.sub.map(t=>`<div class="topic cmd"><span class="k">รับคำสั่ง</span><span class="v">${t}</span></div>`).join('');

  const s=d.uptime, hh=Math.floor(s/3600), mm=Math.floor(s%3600/60);
  document.getElementById('up').textContent = hh+' ชม. '+mm+' นาที';
}

load();
setInterval(load, 2000);
</script>
</body>
</html>
)HTMLPAGE";
