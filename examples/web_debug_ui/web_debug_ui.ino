#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Preferences.h>
#include <bq25_bq27.h>

// ================= WIFI =================
const char *WIFI_SSID = "";
const char *WIFI_PASS = "";
const char *AP_SSID = "bq25-bq27-debug";
const char *AP_PASS = "12345678";
const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
const uint32_t CHARGER_SERVICE_INTERVAL_MS = 750;
const uint32_t VBUS_STABLE_REAPPLY_DELAY_MS = 2500;

// ================= PINS =================
#define SDA_PIN 7
#define SCL_PIN 6

#define BQ_CE_PIN 8
#define BQ_OTG_PIN 9
#define BQ_INT_PIN 10

WebServer server(80);
bq25bq27 power;
Preferences prefs;
bool wifiApMode = false;
uint32_t lastChargerServiceMs = 0;
uint32_t vbusGoodSinceMs = 0;

struct ModuleSettings {
  bool ceEnabled = true;
  bool swChargeEnabled = true;
  bool otgEnabled = false;
  bool adcEnabled = true;
  bool autoDpdmEnabled = false;
  bool icoEnabled = false;
  bool hvdcpEnabled = true;
  bool maxChargeEnabled = true;
  uint16_t inputLimitMa = 1500;
  uint16_t chargeCurrentMa = 2048;
  uint16_t chargeVoltageMv = 4208;
};

ModuleSettings settings;

// ================= HELPERS =================
String hex8(uint8_t v) {
  char b[8];
  snprintf(b, sizeof(b), "0x%02X", v);
  return String(b);
}

String hex16(uint16_t v) {
  char b[10];
  snprintf(b, sizeof(b), "0x%04X", v);
  return String(b);
}

String boolJson(bool v) {
  return v ? "true" : "false";
}

String currentIpString() {
  return wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

String scanJson() {
  String j = "[";
  bool first = true;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (!first) {
        j += ",";
      }
      j += "\"" + hex8(addr) + "\"";
      first = false;
    }
  }

  j += "]";
  return j;
}

// ================= SETTINGS =================
void loadSettings() {
  prefs.begin("bqdbg", false);
  settings.ceEnabled = prefs.getBool("ce", settings.ceEnabled);
  settings.swChargeEnabled = prefs.getBool("swchg", settings.swChargeEnabled);
  settings.otgEnabled = prefs.getBool("otg", settings.otgEnabled);
  settings.adcEnabled = prefs.getBool("adc", settings.adcEnabled);
  settings.autoDpdmEnabled = prefs.getBool("adpdm", settings.autoDpdmEnabled);
  settings.icoEnabled = prefs.getBool("ico", settings.icoEnabled);
  settings.hvdcpEnabled = prefs.getBool("hvdcp", settings.hvdcpEnabled);
  settings.maxChargeEnabled = prefs.getBool("maxc", settings.maxChargeEnabled);
  settings.inputLimitMa = prefs.getUInt("iin", settings.inputLimitMa);
  settings.chargeCurrentMa = prefs.getUInt("ichg", settings.chargeCurrentMa);
  settings.chargeVoltageMv = prefs.getUInt("vreg", settings.chargeVoltageMv);
}

void saveSettings() {
  prefs.putBool("ce", settings.ceEnabled);
  prefs.putBool("swchg", settings.swChargeEnabled);
  prefs.putBool("otg", settings.otgEnabled);
  prefs.putBool("adc", settings.adcEnabled);
  prefs.putBool("adpdm", settings.autoDpdmEnabled);
  prefs.putBool("ico", settings.icoEnabled);
  prefs.putBool("hvdcp", settings.hvdcpEnabled);
  prefs.putBool("maxc", settings.maxChargeEnabled);
  prefs.putUInt("iin", settings.inputLimitMa);
  prefs.putUInt("ichg", settings.chargeCurrentMa);
  prefs.putUInt("vreg", settings.chargeVoltageMv);
}

void normalizeSettings() {
  if (settings.inputLimitMa < 100) {
    settings.inputLimitMa = 100;
  }
  if (settings.inputLimitMa > 3250) {
    settings.inputLimitMa = 3250;
  }
  if (settings.chargeCurrentMa > 5056) {
    settings.chargeCurrentMa = 5056;
  }
  if (settings.chargeVoltageMv < 3840) {
    settings.chargeVoltageMv = 3840;
  }
  if (settings.chargeVoltageMv > 4608) {
    settings.chargeVoltageMv = 4608;
  }
}

void applyPinSettings() {
  digitalWrite(BQ_CE_PIN, settings.ceEnabled ? LOW : HIGH);
  digitalWrite(BQ_OTG_PIN, settings.otgEnabled ? HIGH : LOW);
}

void applySettingsToHardware(bool persist) {
  normalizeSettings();
  applyPinSettings();
  power.charger.setWatchdogTimer(bq25::WatchdogOff);
  power.charger.setAutoDpdmEnabled(settings.autoDpdmEnabled);
  power.charger.setIcoEnabled(settings.icoEnabled);
  power.charger.setHvdcpEnabled(settings.hvdcpEnabled);
  power.charger.setMaxChargeEnabled(settings.maxChargeEnabled);
  power.charger.setChargingEnabled(settings.swChargeEnabled);
  power.charger.setOtgEnabled(settings.otgEnabled);
  power.charger.setInputCurrentLimit(settings.inputLimitMa);
  power.charger.setChargeCurrent(settings.chargeCurrentMa);
  power.charger.setChargeVoltage(settings.chargeVoltageMv);
  if (settings.adcEnabled) {
    power.charger.startAdc(true);
  } else {
    power.charger.stopAdc();
  }
  if (persist) {
    saveSettings();
  }
}

void applySafeDefaults(bool persist) {
  settings = ModuleSettings();
  applySettingsToHardware(persist);
}

void serviceChargerAfterVbusChange() {
  uint32_t now = millis();
  if (now - lastChargerServiceMs < CHARGER_SERVICE_INTERVAL_MS) {
    return;
  }
  lastChargerServiceMs = now;

  bq25::Status status = power.charger.status();
  if (!status.powerGood) {
    vbusGoodSinceMs = 0;
    return;
  }

  if (vbusGoodSinceMs == 0) {
    vbusGoodSinceMs = now;
    return;
  }

  if (now - vbusGoodSinceMs < VBUS_STABLE_REAPPLY_DELAY_MS) {
    return;
  }

  if (!settings.autoDpdmEnabled && !settings.icoEnabled) {
    uint16_t actualLimitMa = power.charger.inputCurrentLimit();
    if (actualLimitMa != settings.inputLimitMa) {
      power.charger.setInputCurrentLimit(settings.inputLimitMa);
      Serial.print("Restored saved input limit after VBUS detection: ");
      Serial.print(settings.inputLimitMa);
      Serial.println(" mA");
    }
  }
}

// ================= JSON =================
String statusJson() {
  uint8_t regs[bq25::kRegisterCount];
  bool regsOk = power.charger.readRegisters(regs, bq25::kRegisterCount);
  bool bq25ok = power.charger.isConnected() && regsOk;
  bool bq27ok = power.gauge.isConnected();

  bq25::Status chgStatus = power.charger.status();
  bq25::Faults chgFaults = power.charger.faults();
  bq25::Measurements chgMeas = power.charger.measurements();

  String j = "{";
  j += "\"ip\":\"" + currentIpString() + "\",";
  j += "\"wifi_mode\":\"" + String(wifiApMode ? "AP" : "STA") + "\",";
  j += "\"i2c_scan\":" + scanJson() + ",";

  j += "\"pins\":{";
  j += "\"sda\":" + String(SDA_PIN) + ",";
  j += "\"scl\":" + String(SCL_PIN) + ",";
  j += "\"ce_gpio\":" + String(BQ_CE_PIN) + ",";
  j += "\"otg_gpio\":" + String(BQ_OTG_PIN) + ",";
  j += "\"int_gpio\":" + String(BQ_INT_PIN) + ",";
  j += "\"ce_charge_enabled\":" + boolJson(digitalRead(BQ_CE_PIN) == LOW) + ",";
  j += "\"otg_pin_enabled\":" + boolJson(digitalRead(BQ_OTG_PIN) == HIGH) + ",";
  j += "\"int_pin_low\":" + boolJson(digitalRead(BQ_INT_PIN) == LOW);
  j += "},";

  j += "\"settings\":{";
  j += "\"ce_enabled\":" + boolJson(settings.ceEnabled) + ",";
  j += "\"sw_charge_enabled\":" + boolJson(settings.swChargeEnabled) + ",";
  j += "\"otg_enabled\":" + boolJson(settings.otgEnabled) + ",";
  j += "\"adc_enabled\":" + boolJson(settings.adcEnabled) + ",";
  j += "\"auto_dpdm_enabled\":" + boolJson(settings.autoDpdmEnabled) + ",";
  j += "\"ico_enabled\":" + boolJson(settings.icoEnabled) + ",";
  j += "\"hvdcp_enabled\":" + boolJson(settings.hvdcpEnabled) + ",";
  j += "\"max_charge_enabled\":" + boolJson(settings.maxChargeEnabled) + ",";
  j += "\"input_limit_mA\":" + String(settings.inputLimitMa) + ",";
  j += "\"charge_current_mA\":" + String(settings.chargeCurrentMa) + ",";
  j += "\"charge_voltage_mV\":" + String(settings.chargeVoltageMv);
  j += "},";

  j += "\"bq25895\":{";
  j += "\"present\":" + boolJson(bq25ok);

  if (bq25ok) {
    int inputLimitMa = power.charger.inputCurrentLimit();
    int chargeSetMa = (regs[bq25::RegChargeCurrent] & 0x7F) * 64;
    uint16_t tsWhole = chgMeas.tsPercentX100 / 100;
    uint16_t tsFrac = (chgMeas.tsPercentX100 % 100) / 10;

    j += ",\"vbus_status\":\"" + String(bq25::vbusTypeName(chgStatus.vbusType)) + "\"";
    j += ",\"charge_status\":\"" + String(bq25::chargeStateName(chgStatus.chargeState)) + "\"";
    j += ",\"power_good\":" + boolJson(chgStatus.powerGood);
    j += ",\"vsys_low\":" + boolJson(chgStatus.systemInRegulation);
    j += ",\"vbus_good\":" + boolJson(chgMeas.vbusGood);
    j += ",\"vindpm\":" + boolJson(chgMeas.vindpm);
    j += ",\"iindpm\":" + boolJson(chgMeas.iindpm);
    j += ",\"thermal_regulation\":" + boolJson(chgMeas.thermalRegulation);

    j += ",\"vbus_mV\":" + String(chgMeas.vbusMv);
    j += ",\"vbat_mV\":" + String(chgMeas.batteryMv);
    j += ",\"vsys_mV\":" + String(chgMeas.systemMv);
    j += ",\"charge_adc_mA\":" + String(chgMeas.chargeCurrentMa);
    j += ",\"ts_percent\":\"" + String(tsWhole) + "." + String(tsFrac) + "\"";

    j += ",\"input_limit_set_mA\":" + String(inputLimitMa);
    j += ",\"input_limit_effective_mA\":" + String(chgMeas.effectiveInputCurrentLimitMa);
    j += ",\"charge_current_set_mA\":" + String(chargeSetMa);
    j += ",\"ilim_pin_enabled\":" + boolJson((regs[bq25::RegInputCurrent] & 0x40) != 0);
    j += ",\"auto_dpdm_enabled\":" + boolJson((regs[bq25::RegAdcAndDpdm] & 0x01) != 0);
    j += ",\"ico_enabled\":" + boolJson((regs[bq25::RegAdcAndDpdm] & 0x10) != 0);
    j += ",\"hvdcp_enabled\":" + boolJson((regs[bq25::RegAdcAndDpdm] & 0x08) != 0);
    j += ",\"max_charge_enabled\":" + boolJson((regs[bq25::RegAdcAndDpdm] & 0x04) != 0);
    j += ",\"ico_optimized\":" + boolJson((regs[bq25::RegPartInfo] & 0x40) != 0);

    j += ",\"fault_watchdog\":" + boolJson(chgFaults.watchdog);
    j += ",\"fault_boost\":" + boolJson(chgFaults.boost);
    j += ",\"fault_charge\":\"" + String(bq25::chargeFaultName(chgFaults.charge)) + "\"";
    j += ",\"fault_battery\":" + boolJson(chgFaults.battery);
    j += ",\"fault_ntc\":\"" + String(bq25::ntcFaultName(chgFaults.ntc)) + "\"";
    j += ",\"fault_ntc_code\":" + String(static_cast<uint8_t>(chgFaults.ntc));

    j += ",\"raw_regs\":[";
    for (uint8_t i = 0; i < bq25::kRegisterCount; i++) {
      if (i) {
        j += ",";
      }
      j += "\"" + hex8(regs[i]) + "\"";
    }
    j += "]";
  }

  j += "},";

  j += "\"bq27\":{";
  j += "\"present\":" + boolJson(bq27ok);

  if (bq27ok) {
    bq27::Snapshot fg = power.gauge.snapshot();
    uint16_t flagsRaw = power.gauge.flagsRaw();
    uint16_t sohRaw = power.gauge.readWord(bq27::CmdStateOfHealth);

    j += ",\"voltage_mV\":" + String(fg.voltageMv);
    j += ",\"soc_percent\":" + String(fg.stateOfChargePercent);
    j += ",\"soc_unfiltered_percent\":" + String(fg.stateOfChargeUnfilteredPercent);
    j += ",\"avg_current_mA\":" + String(fg.averageCurrentMa);
    j += ",\"temperature_C_x10\":" + String(fg.temperatureCelsiusX10);
    j += ",\"remaining_capacity_mAh\":" + String(fg.remainingCapacityMah);
    j += ",\"full_capacity_mAh\":" + String(fg.fullChargeCapacityMah);
    j += ",\"soh_percent\":" + String(fg.stateOfHealthPercent);
    j += ",\"soh_raw\":\"" + hex16(sohRaw) + "\"";
    j += ",\"flags\":\"" + hex16(flagsRaw) + "\"";
    j += ",\"flag_full_charge\":" + boolJson(fg.flags.fullCharge);
    j += ",\"flag_charging\":" + boolJson(fg.flags.charging);
    j += ",\"flag_discharging\":" + boolJson(fg.flags.discharging);
    j += ",\"flag_itpor\":" + boolJson(fg.flags.powerOnReset);
  }

  j += "}";
  j += "}";

  return j;
}

// ================= WEB UI =================
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>BQ Debug UI</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{margin:0;font-family:Arial;background:#0b0d10;color:#eef2f7}
header{padding:18px;background:#111722;border-bottom:1px solid #2b3340}
h1{margin:0;font-size:22px}.small{color:#9aa4b2;font-size:13px;margin-top:6px}
.wrap{padding:16px;display:grid;gap:14px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:14px}
.card{background:#151922;border:1px solid #2b3340;border-radius:10px;padding:14px}.card h2{margin:0 0 12px 0;font-size:17px}
.big{font-size:28px;font-weight:bold}.ok{color:#26d07c}.bad{color:#ff5c5c}.warn{color:#ffcc4d}.blue{color:#4da3ff}
.row{display:flex;justify-content:space-between;border-bottom:1px solid #222a36;padding:8px 0;gap:12px}.row:last-child{border-bottom:0}
button{background:#222b38;color:white;border:1px solid #354154;border-radius:8px;padding:10px 12px;margin:4px;cursor:pointer}
.togglebar{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:10px;margin-bottom:10px}
.toggle{display:flex;align-items:center;justify-content:space-between;margin:0;padding:13px 14px;font-weight:bold}
.toggle b{font-size:13px;border-radius:999px;padding:4px 8px;background:rgba(0,0,0,.28)}
.toggle.on{background:#123d2a;border-color:#26d07c;color:#c9ffe2}
.toggle.off{background:#421a1a;border-color:#ff5c5c;color:#ffd5d5}
input{width:90px;background:#0f131a;color:white;border:1px solid #354154;border-radius:8px;padding:10px}
pre{background:#05070a;color:#d5dbe5;border:1px solid #2b3340;border-radius:8px;padding:12px;overflow-x:auto;white-space:pre-wrap}
.badge{display:inline-block;padding:5px 9px;border-radius:999px;font-size:13px;font-weight:bold;background:#222b38}
.badge.ok{background:#113d2a;color:#26d07c}.badge.bad{background:#421a1a;color:#ff5c5c}.badge.warn{background:#423818;color:#ffcc4d}
table{width:100%;border-collapse:collapse}td,th{padding:8px;border-bottom:1px solid #222a36;text-align:left}th{color:#9aa4b2;font-weight:normal}
</style>
</head>
<body>
<header>
  <h1>BQ25895 + BQ27441 Debug UI</h1>
  <div class="small" id="topline">Connecting...</div>
</header>

<div class="wrap">
<div class="grid">
  <div class="card"><h2>Charge Status</h2><div class="big" id="chargeStatus">---</div><div class="small" id="chargeInfo">---</div></div>
  <div class="card"><h2>Charge Current</h2><div class="big blue" id="chargeCurrent">---</div><div class="small">BQ25895 ADC</div></div>
  <div class="card"><h2>USB / VBUS</h2><div class="big" id="vbus">---</div><div class="small" id="vbusInfo">---</div></div>
  <div class="card"><h2>Battery</h2><div class="big" id="battery">---</div><div class="small" id="batteryInfo">---</div></div>
</div>

<div class="grid">
  <div class="card">
    <h2>BQ25895 Values</h2>
    <div class="row"><span>Power Good</span><b id="pg">---</b></div>
    <div class="row"><span>VBUS Status</span><b id="vbusStatus">---</b></div>
    <div class="row"><span>VBUS</span><b id="vbus2">---</b></div>
    <div class="row"><span>VBAT</span><b id="vbat">---</b></div>
    <div class="row"><span>VSYS</span><b id="vsys">---</b></div>
    <div class="row"><span>TS / NTC</span><b id="ts">---</b></div>
    <div class="row"><span>Input Limit</span><b id="iinSet">---</b></div>
    <div class="row"><span>Effective Input Limit</span><b id="iinEffective">---</b></div>
    <div class="row"><span>IINDPM / VINDPM</span><b id="dpmState">---</b></div>
    <div class="row"><span>Charge Current Set</span><b id="ichgSet">---</b></div>
    <div class="row"><span>DPDM / HVDCP</span><b id="dpdmHvdcp">---</b></div>
    <div class="row"><span>ICO / MaxCharge</span><b id="icoMaxc">---</b></div>
  </div>

  <div class="card">
    <h2>BQ27441 Fuel Gauge</h2>
    <div class="row"><span>SOC</span><b id="soc">---</b></div>
    <div class="row"><span>Voltage</span><b id="fgVolt">---</b></div>
    <div class="row"><span>Avg Current</span><b id="fgCur">---</b></div>
    <div class="row"><span>Temperature</span><b id="fgTemp">---</b></div>
    <div class="row"><span>Remaining Cap</span><b id="remCap">---</b></div>
    <div class="row"><span>Full Cap</span><b id="fullCap">---</b></div>
    <div class="row"><span>Flags</span><b id="fgFlags">---</b></div>
  </div>

  <div class="card">
    <h2>Faults</h2>
    <div class="row"><span>Charge Fault</span><b id="faultCharge">---</b></div>
    <div class="row"><span>Watchdog Fault</span><b id="faultWd">---</b></div>
    <div class="row"><span>Boost Fault</span><b id="faultBoost">---</b></div>
    <div class="row"><span>Battery Fault</span><b id="faultBat">---</b></div>
    <div class="row"><span>NTC Fault</span><b id="faultNtc">---</b></div>
    <div class="row"><span>INT Pin LOW</span><b id="intPin">---</b></div>
  </div>
</div>

<div class="card">
  <h2>Controls</h2>
  <div class="togglebar">
    <button id="toggleCe" class="toggle off" onclick="toggleCmd('ce')"><span>CE Pin</span><b>OFF</b></button>
    <button id="toggleSw" class="toggle off" onclick="toggleCmd('sw')"><span>SW Charge</span><b>OFF</b></button>
    <button id="toggleOtg" class="toggle off" onclick="toggleCmd('otg')"><span>OTG</span><b>OFF</b></button>
    <button id="toggleAdc" class="toggle off" onclick="toggleCmd('adc')"><span>ADC</span><b>OFF</b></button>
    <button id="toggleAutoDpdm" class="toggle off" onclick="toggleCmd('auto_dpdm')"><span>Auto DPDM</span><b>OFF</b></button>
    <button id="toggleIco" class="toggle off" onclick="toggleCmd('ico')"><span>ICO</span><b>OFF</b></button>
    <button id="toggleHvdcp" class="toggle off" onclick="toggleCmd('hvdcp')"><span>HVDCP</span><b>OFF</b></button>
    <button id="toggleMaxc" class="toggle off" onclick="toggleCmd('maxc')"><span>MaxCharge</span><b>OFF</b></button>
  </div>
  <button onclick="cmd('wd_off')">Watchdog OFF</button>
  <button onclick="cmd('dpdm')">Force DPDM</button>
  <button onclick="cmd('qc_detect')">QC/HVDCP Detect</button>
  <button onclick="cmd('safe')">Save Safe Defaults</button>
  <button onclick="cmd('reset_bq25')">Reset BQ25895</button>
</div>

<div class="card">
  <h2>Set Limits</h2>
  Input Limit: <input id="iin" value="1500"><button onclick="setIin()">set mA</button><br><br>
  Charge Current: <input id="ichg" value="2048"><button onclick="setIchg()">set mA</button><br><br>
  Charge Voltage: <input id="vreg" value="4208"><button onclick="setVreg()">set mV</button>
</div>

<div class="card"><h2>I2C Scan</h2><pre id="scan">---</pre></div>
<div class="card"><h2>BQ25895 Raw Register</h2><table><thead><tr><th>Register</th><th>Value</th></tr></thead><tbody id="regs"></tbody></table></div>
<div class="card"><h2>Full JSON Dump</h2><pre id="json">---</pre></div>
</div>

<script>
function setText(id, txt, cls="") {
  let e = document.getElementById(id);
  e.textContent = txt;
  e.className = cls;
}
function badge(txt, type) { return '<span class="badge ' + type + '">' + txt + '</span>'; }
async function cmd(x) { await fetch('/cmd?x=' + x); update(); }
async function toggleCmd(x) { await fetch('/cmd?x=toggle_' + x); update(); }
async function setIin() { await fetch('/cmd?x=set_iin&ma=' + document.getElementById('iin').value); update(); }
async function setIchg() { await fetch('/cmd?x=set_ichg&ma=' + document.getElementById('ichg').value); update(); }
async function setVreg() { await fetch('/cmd?x=set_vreg&mv=' + document.getElementById('vreg').value); update(); }

function setToggle(id, on) {
  let e = document.getElementById(id);
  e.className = "toggle " + (on ? "on" : "off");
  e.querySelector("b").textContent = on ? "ON" : "OFF";
}

async function update() {
  try {
    let r = await fetch('/api/status');
    let j = await r.json();
    document.getElementById('json').textContent = JSON.stringify(j, null, 2);
    document.getElementById('topline').textContent = "IP: " + j.ip + " | I2C: " + j.i2c_scan.join(", ");
    document.getElementById('scan').textContent = j.i2c_scan.join("\\n");

    let bq = j.bq25895;
    let fg = j.bq27;
    let st = j.settings;

    setToggle("toggleCe", st.ce_enabled);
    setToggle("toggleSw", st.sw_charge_enabled);
    setToggle("toggleOtg", st.otg_enabled);
    setToggle("toggleAdc", st.adc_enabled);
    setToggle("toggleAutoDpdm", st.auto_dpdm_enabled);
    setToggle("toggleIco", st.ico_enabled);
    setToggle("toggleHvdcp", st.hvdcp_enabled);
    setToggle("toggleMaxc", st.max_charge_enabled);
    if (document.activeElement.id !== "iin") document.getElementById("iin").value = st.input_limit_mA;
    if (document.activeElement.id !== "ichg") document.getElementById("ichg").value = st.charge_current_mA;
    if (document.activeElement.id !== "vreg") document.getElementById("vreg").value = st.charge_voltage_mV;

    if (bq.present) {
      let chargeCls = "bad";
      if (bq.charge_status == "fast charge") chargeCls = "ok";
      if (bq.charge_status == "precharge") chargeCls = "warn";
      if (bq.charge_status == "done") chargeCls = "blue";

      setText("chargeStatus", bq.charge_status, chargeCls);
      setText("chargeInfo", "Fault: " + bq.fault_charge);
      setText("chargeCurrent", bq.charge_adc_mA + " mA", bq.charge_adc_mA > 100 ? "blue" : "warn");

      let vbusCls = "ok";
      let vbusMsg = "OK";
      if (bq.vbus_mV < 4400) { vbusCls = "bad"; vbusMsg = "too low"; }
      else if (bq.vbus_mV < 4700) { vbusCls = "warn"; vbusMsg = "a little low"; }

      setText("vbus", bq.vbus_mV + " mV", vbusCls);
      setText("vbusInfo", bq.vbus_status + " | " + vbusMsg);
      setText("battery", bq.vbat_mV + " mV", "ok");
      setText("batteryInfo", "VSYS: " + bq.vsys_mV + " mV");

      document.getElementById("pg").innerHTML = bq.power_good ? badge("YES", "ok") : badge("NO", "bad");
      setText("vbusStatus", bq.vbus_status);
      setText("vbus2", bq.vbus_mV + " mV", vbusCls);
      setText("vbat", bq.vbat_mV + " mV");
      setText("vsys", bq.vsys_mV + " mV");
      setText("ts", bq.ts_percent + " %");
      setText("iinSet", bq.input_limit_set_mA + " mA");
      setText("iinEffective", bq.input_limit_effective_mA + " mA", bq.input_limit_effective_mA < st.input_limit_mA ? "warn" : "ok");
      setText("dpmState", "IIN " + (bq.iindpm ? "ON" : "OFF") + " / VIN " + (bq.vindpm ? "ON" : "OFF"), (bq.iindpm || bq.vindpm) ? "warn" : "ok");
      setText("ichgSet", bq.charge_current_set_mA + " mA");
      setText("dpdmHvdcp", (bq.auto_dpdm_enabled ? "DPDM ON" : "DPDM OFF") + " / " + (bq.hvdcp_enabled ? "HVDCP ON" : "HVDCP OFF"));
      setText("icoMaxc", (bq.ico_enabled ? "ICO ON" : "ICO OFF") + " / " + (bq.max_charge_enabled ? "MaxC ON" : "MaxC OFF"));

      setText("faultCharge", bq.fault_charge, bq.fault_charge == "normal" ? "ok" : "bad");
      setText("faultWd", bq.fault_watchdog ? "YES" : "NO", bq.fault_watchdog ? "bad" : "ok");
      setText("faultBoost", bq.fault_boost ? "YES" : "NO", bq.fault_boost ? "bad" : "ok");
      setText("faultBat", bq.fault_battery ? "YES" : "NO", bq.fault_battery ? "bad" : "ok");
      setText("faultNtc", bq.fault_ntc, bq.fault_ntc_code == 0 ? "ok" : "bad");

      let html = "";
      for (let i = 0; i < bq.raw_regs.length; i++) {
        let reg = "REG" + i.toString(16).toUpperCase().padStart(2, "0");
        html += "<tr><td>" + reg + "</td><td><b>" + bq.raw_regs[i] + "</b></td></tr>";
      }
      document.getElementById("regs").innerHTML = html;
    } else {
      setText("chargeStatus", "BQ25895 missing", "bad");
    }

    if (fg.present) {
      setText("soc", fg.soc_percent + " %");
      setText("fgVolt", fg.voltage_mV + " mV");
      setText("fgCur", fg.avg_current_mA + " mA", fg.avg_current_mA > 0 ? "ok" : "warn");
      setText("fgTemp", (fg.temperature_C_x10 / 10).toFixed(1) + " C");
      setText("remCap", fg.remaining_capacity_mAh + " mAh");
      setText("fullCap", fg.full_capacity_mAh + " mAh");
      setText("fgFlags", fg.flags);
    } else {
      setText("soc", "BQ27441 missing", "bad");
    }

    document.getElementById("intPin").innerHTML = j.pins.int_pin_low ? badge("LOW", "warn") : badge("HIGH", "ok");
  } catch(e) {
    document.getElementById('json').textContent = "ERROR: " + e;
  }
}

setInterval(update, 1500);
update();
</script>
</body>
</html>
)rawliteral";
}

// ================= WEB HANDLERS =================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleStatus() {
  server.send(200, "application/json", statusJson());
}

void handleCmd() {
  String x = server.arg("x");

  if (x == "toggle_ce") {
    settings.ceEnabled = !settings.ceEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_sw") {
    settings.swChargeEnabled = !settings.swChargeEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_otg") {
    settings.otgEnabled = !settings.otgEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_adc") {
    settings.adcEnabled = !settings.adcEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_auto_dpdm") {
    settings.autoDpdmEnabled = !settings.autoDpdmEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_ico") {
    settings.icoEnabled = !settings.icoEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_hvdcp") {
    settings.hvdcpEnabled = !settings.hvdcpEnabled;
    applySettingsToHardware(true);
  } else if (x == "toggle_maxc") {
    settings.maxChargeEnabled = !settings.maxChargeEnabled;
    applySettingsToHardware(true);
  } else if (x == "wd_off") {
    power.charger.setWatchdogTimer(bq25::WatchdogOff);
  } else if (x == "dpdm") {
    power.charger.forceDpdmDetection();
  } else if (x == "qc_detect") {
    settings.autoDpdmEnabled = true;
    settings.hvdcpEnabled = true;
    settings.maxChargeEnabled = true;
    applySettingsToHardware(true);
    power.charger.forceDpdmDetection();
  } else if (x == "ce_on") {
    settings.ceEnabled = true;
    applySettingsToHardware(true);
  } else if (x == "ce_off") {
    settings.ceEnabled = false;
    applySettingsToHardware(true);
  } else if (x == "otg_on") {
    settings.otgEnabled = true;
    applySettingsToHardware(true);
  } else if (x == "otg_off") {
    settings.otgEnabled = false;
    applySettingsToHardware(true);
  } else if (x == "adc_on") {
    settings.adcEnabled = true;
    applySettingsToHardware(true);
  } else if (x == "chg_sw_on") {
    settings.swChargeEnabled = true;
    applySettingsToHardware(true);
  } else if (x == "chg_sw_off") {
    settings.swChargeEnabled = false;
    applySettingsToHardware(true);
  } else if (x == "reset_bq25") {
    power.charger.resetRegisters();
    delay(100);
    applySettingsToHardware(false);
  } else if (x == "safe") {
    applySafeDefaults(true);
  } else if (x == "set_iin") {
    int ma = server.arg("ma").toInt();
    if (ma < 100) ma = 100;
    if (ma > 3250) ma = 3250;
    settings.inputLimitMa = ma;
    applySettingsToHardware(true);
  } else if (x == "set_ichg") {
    int ma = server.arg("ma").toInt();
    if (ma < 0) ma = 0;
    if (ma > 5056) ma = 5056;
    settings.chargeCurrentMa = ma;
    applySettingsToHardware(true);
  } else if (x == "set_vreg") {
    int mv = server.arg("mv").toInt();
    if (mv < 3840) mv = 3840;
    if (mv > 4608) mv = 4608;
    settings.chargeVoltageMv = mv;
    applySettingsToHardware(true);
  }

  server.send(200, "text/plain", "OK");
}

// ================= WIFI =================
void setupWifi() {
  wifiApMode = false;

  if (strlen(WIFI_SSID) == 0) {
    WiFi.mode(WIFI_AP);
    wifiApMode = WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("No STA WiFi configured. AP mode ");
    Serial.println(wifiApMode ? "started." : "failed.");
    Serial.print("AP SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting WiFi");
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
    return;
  }

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  wifiApMode = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("WiFi STA failed. AP mode ");
  Serial.println(wifiApMode ? "started." : "failed.");
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BQ_CE_PIN, OUTPUT);
  pinMode(BQ_OTG_PIN, OUTPUT);
  pinMode(BQ_INT_PIN, INPUT_PULLUP);

  loadSettings();
  applyPinSettings();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  delay(300);

  bq25bq27::BeginOptions options;
  options.startWire = false;
  power.begin(Wire, options);
  applySettingsToHardware(false);

  setupWifi();

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/cmd", handleCmd);
  server.begin();

  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
  serviceChargerAfterVbusChange();
}
