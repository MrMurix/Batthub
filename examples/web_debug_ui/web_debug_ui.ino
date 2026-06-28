#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Preferences.h>
#include <Batthub.h>

// ================= WIFI =================
const char *WIFI_SSID = "";
const char *WIFI_PASS = "";
const char *AP_SSID = "batthub-debug";
const char *AP_PASS = "batthub123";
const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
const uint32_t CHARGER_SERVICE_INTERVAL_MS = 750;
const uint32_t VBUS_STABLE_REAPPLY_DELAY_MS = 2500;
const uint32_t WATCHDOG_SERVICE_INTERVAL_MS = 10000;

// ================= HOST WIRING =================
// This ESP32 example needs to know how your microcontroller is wired to the
// Batthub module. Set optional pins to -1 when they are not connected.
const int SDA_PIN = 7;
const int SCL_PIN = 6;
const int8_t CE_PIN = 8;
const int8_t OTG_PIN = 9;
const int8_t INT_PIN = 10;

WebServer server(80);
Batthub power;
Preferences prefs;
bool wifiApMode = false;
uint32_t lastChargerServiceMs = 0;
uint32_t lastWatchdogServiceMs = 0;
uint32_t vbusGoodSinceMs = 0;

struct WebSettings {
  bool cePinEnabled = true;
  Batthub::ChargerSettings charger;
  Batthub::BatteryProfile batteryProfile;
  Batthub::GaugeSettings gauge;
};

WebSettings settings;
Batthub::BatteryProfile lastReadBatteryProfile;
bool lastReadBatteryProfileValid = false;

// ================= HELPERS =================
String boolJson(bool v) {
  return v ? "true" : "false";
}

String quoted(const String &value) {
  String out = "\"";
  for (uint16_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    if (c == '"' || c == '\\') {
      out += '\\';
    }
    out += c;
  }
  out += "\"";
  return out;
}

String currentIpString() {
  return wifiApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

uint16_t clampU16(int value, uint16_t minValue, uint16_t maxValue) {
  if (value < static_cast<int>(minValue)) {
    return minValue;
  }
  if (value > static_cast<int>(maxValue)) {
    return maxValue;
  }
  return static_cast<uint16_t>(value);
}

uint8_t clampU8(int value, uint8_t minValue, uint8_t maxValue) {
  if (value < static_cast<int>(minValue)) {
    return minValue;
  }
  if (value > static_cast<int>(maxValue)) {
    return maxValue;
  }
  return static_cast<uint8_t>(value);
}

uint16_t argU16(const char *name, uint16_t current, uint16_t minValue, uint16_t maxValue) {
  if (!server.hasArg(name)) {
    return current;
  }
  return clampU16(server.arg(name).toInt(), minValue, maxValue);
}

uint8_t argU8(const char *name, uint8_t current, uint8_t minValue, uint8_t maxValue) {
  if (!server.hasArg(name)) {
    return current;
  }
  return clampU8(server.arg(name).toInt(), minValue, maxValue);
}

bool argBool(const char *name, bool current) {
  if (!server.hasArg(name)) {
    return current;
  }
  String value = server.arg(name);
  return value == "1" || value == "true" || value == "on";
}

int argInt(const char *name, int current, int minValue, int maxValue) {
  if (!server.hasArg(name)) {
    return current;
  }
  int value = server.arg(name).toInt();
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

uint8_t field(uint8_t reg, uint8_t mask, uint8_t shift) {
  return static_cast<uint8_t>((reg & mask) >> shift);
}

uint16_t decodeLinear(uint8_t code, uint16_t base, uint16_t step) {
  return static_cast<uint16_t>(base + static_cast<uint16_t>(code) * step);
}

String yesNo(bool value) {
  return value ? "Yes" : "No";
}

String onOff(bool value) {
  return value ? "On" : "Off";
}

String gpoutModeName(bq27::GpoutMode mode) {
  return mode == bq27::GpoutBatLow ? "Battery low" : "SOC interrupt";
}

String temperatureSourceName(bq27::TemperatureSource source) {
  return source == bq27::TemperatureFromHost ? "Host provided" : "Internal sensor";
}

String watchdogName(uint8_t code) {
  switch (code) {
    case bq25::WatchdogOff: return "Off";
    case bq25::Watchdog40s: return "40 seconds";
    case bq25::Watchdog80s: return "80 seconds";
    case bq25::Watchdog160s: return "160 seconds";
  }
  return "Unknown";
}

String safetyTimerName(uint8_t code) {
  switch (code) {
    case bq25::SafetyTimer5h: return "5 hours";
    case bq25::SafetyTimer8h: return "8 hours";
    case bq25::SafetyTimer12h: return "12 hours";
    case bq25::SafetyTimer20h: return "20 hours";
  }
  return "Unknown";
}

String thermalRegulationName(uint8_t code) {
  switch (code) {
    case bq25::Thermal60C: return "60 C";
    case bq25::Thermal80C: return "80 C";
    case bq25::Thermal100C: return "100 C";
    case bq25::Thermal120C: return "120 C";
  }
  return "Unknown";
}

String boostFrequencyName(uint8_t code) {
  return code == bq25::Boost500kHz ? "500 kHz" : "1500 kHz";
}

String boostHotName(uint8_t code) {
  switch (code) {
    case bq25::BoostHot3475: return "34.75 percent";
    case bq25::BoostHot3775: return "37.75 percent";
    case bq25::BoostHot3125: return "31.25 percent";
    case bq25::BoostHotDisabled: return "Disabled";
  }
  return "Unknown";
}

String boostColdName(uint8_t code) {
  return code == bq25::BoostCold80 ? "80 percent" : "77 percent";
}

String batteryLowName(uint8_t code) {
  return code == bq25::BatteryLow3000mV ? "3000 mV" : "2800 mV";
}

String rechargeName(uint8_t code) {
  return code == bq25::Recharge200mV ? "200 mV below full" : "100 mV below full";
}

void appendBoolField(String &j, const char *name, bool value, bool comma = true) {
  j += "\"";
  j += name;
  j += "\":";
  j += boolJson(value);
  if (comma) {
    j += ",";
  }
}

void appendU16Field(String &j, const char *name, uint16_t value, bool comma = true) {
  j += "\"";
  j += name;
  j += "\":";
  j += String(value);
  if (comma) {
    j += ",";
  }
}

void appendI16Field(String &j, const char *name, int16_t value, bool comma = true) {
  j += "\"";
  j += name;
  j += "\":";
  j += String(value);
  if (comma) {
    j += ",";
  }
}

void appendStringField(String &j, const char *name, const String &value, bool comma = true) {
  j += "\"";
  j += name;
  j += "\":";
  j += quoted(value);
  if (comma) {
    j += ",";
  }
}

// ================= SETTINGS =================
void loadSettings() {
  prefs.begin("bqdbg", false);
  settings.cePinEnabled = prefs.getBool("cePinEnabled", settings.cePinEnabled);

  settings.charger.highImpedanceMode = prefs.getBool("highImpedanceMode", settings.charger.highImpedanceMode);
  settings.charger.ilimPinEnabled = prefs.getBool("ilimPinEnabled", settings.charger.ilimPinEnabled);
  settings.charger.inputCurrentLimitMa = prefs.getUInt("inputCurrentLimitMa", settings.charger.inputCurrentLimitMa);
  settings.charger.inputVoltageLimitMv = prefs.getUInt("inputVoltageLimitMv", settings.charger.inputVoltageLimitMv);
  settings.charger.vindpmOffsetMv = prefs.getUInt("vindpmOffsetMv", settings.charger.vindpmOffsetMv);
  settings.charger.chargeCurrentMa = prefs.getUInt("chargeCurrentMa", settings.charger.chargeCurrentMa);
  settings.charger.prechargeCurrentMa = prefs.getUInt("prechargeCurrentMa", settings.charger.prechargeCurrentMa);
  settings.charger.terminationCurrentMa = prefs.getUInt("terminationCurrentMa", settings.charger.terminationCurrentMa);
  settings.charger.chargeVoltageMv = prefs.getUInt("chargeVoltageMv", settings.charger.chargeVoltageMv);
  settings.charger.minSystemVoltageMv = prefs.getUInt("minSystemVoltageMv", settings.charger.minSystemVoltageMv);
  settings.charger.boostVoltageMv = prefs.getUInt("boostVoltageMv", settings.charger.boostVoltageMv);
  settings.charger.irCompResistanceMohm = prefs.getUInt("irCompResistanceMohm", settings.charger.irCompResistanceMohm);
  settings.charger.irCompVoltageClampMv = prefs.getUInt("irCompVoltageClampMv", settings.charger.irCompVoltageClampMv);
  settings.charger.watchdog = static_cast<bq25::WatchdogTimer>(prefs.getUInt("watchdog", settings.charger.watchdog));
  settings.charger.safetyTimer = static_cast<bq25::SafetyTimer>(prefs.getUInt("safetyTimer", settings.charger.safetyTimer));
  settings.charger.thermalRegulation = static_cast<bq25::ThermalRegulation>(prefs.getUInt("thermalRegulation", settings.charger.thermalRegulation));
  settings.charger.boostFrequency = static_cast<bq25::BoostFrequency>(prefs.getUInt("boostFrequency", settings.charger.boostFrequency));
  settings.charger.boostHotThreshold = static_cast<bq25::BoostHotThreshold>(prefs.getUInt("boostHotThreshold", settings.charger.boostHotThreshold));
  settings.charger.boostColdThreshold = static_cast<bq25::BoostColdThreshold>(prefs.getUInt("boostColdThreshold", settings.charger.boostColdThreshold));
  settings.charger.batteryLowThreshold = static_cast<bq25::BatteryLowThreshold>(prefs.getUInt("batteryLowThreshold", settings.charger.batteryLowThreshold));
  settings.charger.rechargeThreshold = static_cast<bq25::RechargeThreshold>(prefs.getUInt("rechargeThreshold", settings.charger.rechargeThreshold));
  settings.charger.chargingEnabled = prefs.getBool("chargingEnabled", settings.charger.chargingEnabled);
  settings.charger.otgEnabled = prefs.getBool("otgEnabled", settings.charger.otgEnabled);
  settings.charger.batteryLoadEnabled = prefs.getBool("batteryLoadEnabled", settings.charger.batteryLoadEnabled);
  settings.charger.pumpXEnabled = prefs.getBool("pumpXEnabled", settings.charger.pumpXEnabled);
  settings.charger.terminationEnabled = prefs.getBool("terminationEnabled", settings.charger.terminationEnabled);
  settings.charger.safetyTimerEnabled = prefs.getBool("safetyTimerEnabled", settings.charger.safetyTimerEnabled);
  settings.charger.safetyTimerSlowedInDpm = prefs.getBool("safetyTimerSlowedInDpm", settings.charger.safetyTimerSlowedInDpm);
  settings.charger.icoEnabled = prefs.getBool("icoEnabled", settings.charger.icoEnabled);
  settings.charger.autoDpdmEnabled = prefs.getBool("autoDpdmEnabled", settings.charger.autoDpdmEnabled);
  settings.charger.hvdcpEnabled = prefs.getBool("hvdcpEnabled", settings.charger.hvdcpEnabled);
  settings.charger.maxChargeEnabled = prefs.getBool("maxChargeEnabled", settings.charger.maxChargeEnabled);
  settings.charger.adcContinuous = prefs.getBool("adcContinuous", settings.charger.adcContinuous);
  settings.charger.absoluteVindpm = prefs.getBool("absoluteVindpm", settings.charger.absoluteVindpm);
  settings.charger.batfetResetEnabled = prefs.getBool("batfetResetEnabled", settings.charger.batfetResetEnabled);

  settings.batteryProfile.designCapacityMah = prefs.getUInt("designCapacityMah", settings.batteryProfile.designCapacityMah);
  settings.batteryProfile.designEnergyMwh = prefs.getUInt("designEnergyMwh", settings.batteryProfile.designEnergyMwh);
  settings.batteryProfile.terminateVoltageMv = prefs.getUInt("terminateVoltageMv", settings.batteryProfile.terminateVoltageMv);
  settings.batteryProfile.taperRate = prefs.getUInt("taperRate", settings.batteryProfile.taperRate);
  settings.batteryProfile.soc1SetPercent = prefs.getUInt("soc1SetPercent", settings.batteryProfile.soc1SetPercent);
  settings.batteryProfile.soc1ClearPercent = prefs.getUInt("soc1ClearPercent", settings.batteryProfile.soc1ClearPercent);
  settings.batteryProfile.socfSetPercent = prefs.getUInt("socfSetPercent", settings.batteryProfile.socfSetPercent);
  settings.batteryProfile.socfClearPercent = prefs.getUInt("socfClearPercent", settings.batteryProfile.socfClearPercent);
  settings.batteryProfile.socIntDeltaPercent = prefs.getUInt("socIntDeltaPercent", settings.batteryProfile.socIntDeltaPercent);
  settings.gauge.batteryProfile = settings.batteryProfile;

  settings.gauge.gpoutMode = static_cast<bq27::GpoutMode>(prefs.getUInt("gpoutMode", settings.gauge.gpoutMode));
  settings.gauge.temperatureSource = static_cast<bq27::TemperatureSource>(prefs.getUInt("temperatureSource", settings.gauge.temperatureSource));
  settings.gauge.gpoutActiveHigh = prefs.getBool("gpoutActiveHigh", settings.gauge.gpoutActiveHigh);
  settings.gauge.batteryInsertionEnabled = prefs.getBool("batteryInsertionEnabled", settings.gauge.batteryInsertionEnabled);
  settings.gauge.binPullupEnabled = prefs.getBool("binPullupEnabled", settings.gauge.binPullupEnabled);
  settings.gauge.sleepEnabled = prefs.getBool("sleepEnabled", settings.gauge.sleepEnabled);
  settings.gauge.rmFccSmoothingEnabled = prefs.getBool("rmFccSmoothingEnabled", settings.gauge.rmFccSmoothingEnabled);
}

void saveChargerSettings() {
  prefs.putBool("cePinEnabled", settings.cePinEnabled);
  prefs.putBool("highImpedanceMode", settings.charger.highImpedanceMode);
  prefs.putBool("ilimPinEnabled", settings.charger.ilimPinEnabled);
  prefs.putUInt("inputCurrentLimitMa", settings.charger.inputCurrentLimitMa);
  prefs.putUInt("inputVoltageLimitMv", settings.charger.inputVoltageLimitMv);
  prefs.putUInt("vindpmOffsetMv", settings.charger.vindpmOffsetMv);
  prefs.putUInt("chargeCurrentMa", settings.charger.chargeCurrentMa);
  prefs.putUInt("prechargeCurrentMa", settings.charger.prechargeCurrentMa);
  prefs.putUInt("terminationCurrentMa", settings.charger.terminationCurrentMa);
  prefs.putUInt("chargeVoltageMv", settings.charger.chargeVoltageMv);
  prefs.putUInt("minSystemVoltageMv", settings.charger.minSystemVoltageMv);
  prefs.putUInt("boostVoltageMv", settings.charger.boostVoltageMv);
  prefs.putUInt("irCompResistanceMohm", settings.charger.irCompResistanceMohm);
  prefs.putUInt("irCompVoltageClampMv", settings.charger.irCompVoltageClampMv);
  prefs.putUInt("watchdog", settings.charger.watchdog);
  prefs.putUInt("safetyTimer", settings.charger.safetyTimer);
  prefs.putUInt("thermalRegulation", settings.charger.thermalRegulation);
  prefs.putUInt("boostFrequency", settings.charger.boostFrequency);
  prefs.putUInt("boostHotThreshold", settings.charger.boostHotThreshold);
  prefs.putUInt("boostColdThreshold", settings.charger.boostColdThreshold);
  prefs.putUInt("batteryLowThreshold", settings.charger.batteryLowThreshold);
  prefs.putUInt("rechargeThreshold", settings.charger.rechargeThreshold);
  prefs.putBool("chargingEnabled", settings.charger.chargingEnabled);
  prefs.putBool("otgEnabled", settings.charger.otgEnabled);
  prefs.putBool("batteryLoadEnabled", settings.charger.batteryLoadEnabled);
  prefs.putBool("pumpXEnabled", settings.charger.pumpXEnabled);
  prefs.putBool("terminationEnabled", settings.charger.terminationEnabled);
  prefs.putBool("safetyTimerEnabled", settings.charger.safetyTimerEnabled);
  prefs.putBool("safetyTimerSlowedInDpm", settings.charger.safetyTimerSlowedInDpm);
  prefs.putBool("icoEnabled", settings.charger.icoEnabled);
  prefs.putBool("autoDpdmEnabled", settings.charger.autoDpdmEnabled);
  prefs.putBool("hvdcpEnabled", settings.charger.hvdcpEnabled);
  prefs.putBool("maxChargeEnabled", settings.charger.maxChargeEnabled);
  prefs.putBool("adcContinuous", settings.charger.adcContinuous);
  prefs.putBool("absoluteVindpm", settings.charger.absoluteVindpm);
  prefs.putBool("batfetResetEnabled", settings.charger.batfetResetEnabled);
}

void saveGaugeSettings() {
  prefs.putUInt("designCapacityMah", settings.batteryProfile.designCapacityMah);
  prefs.putUInt("designEnergyMwh", settings.batteryProfile.designEnergyMwh);
  prefs.putUInt("terminateVoltageMv", settings.batteryProfile.terminateVoltageMv);
  prefs.putUInt("taperRate", settings.batteryProfile.taperRate);
  prefs.putUInt("soc1SetPercent", settings.batteryProfile.soc1SetPercent);
  prefs.putUInt("soc1ClearPercent", settings.batteryProfile.soc1ClearPercent);
  prefs.putUInt("socfSetPercent", settings.batteryProfile.socfSetPercent);
  prefs.putUInt("socfClearPercent", settings.batteryProfile.socfClearPercent);
  prefs.putUInt("socIntDeltaPercent", settings.batteryProfile.socIntDeltaPercent);
  prefs.putUInt("gpoutMode", settings.gauge.gpoutMode);
  prefs.putUInt("temperatureSource", settings.gauge.temperatureSource);
  prefs.putBool("gpoutActiveHigh", settings.gauge.gpoutActiveHigh);
  prefs.putBool("batteryInsertionEnabled", settings.gauge.batteryInsertionEnabled);
  prefs.putBool("binPullupEnabled", settings.gauge.binPullupEnabled);
  prefs.putBool("sleepEnabled", settings.gauge.sleepEnabled);
  prefs.putBool("rmFccSmoothingEnabled", settings.gauge.rmFccSmoothingEnabled);
}

void normalizeChargerSettings() {
  settings.charger.inputCurrentLimitMa = clampU16(settings.charger.inputCurrentLimitMa, 100, 3250);
  settings.charger.inputVoltageLimitMv = clampU16(settings.charger.inputVoltageLimitMv, 2600, 15300);
  settings.charger.vindpmOffsetMv = clampU16(settings.charger.vindpmOffsetMv, 0, 3100);
  settings.charger.chargeCurrentMa = clampU16(settings.charger.chargeCurrentMa, 0, 5056);
  settings.charger.prechargeCurrentMa = clampU16(settings.charger.prechargeCurrentMa, 64, 1024);
  settings.charger.terminationCurrentMa = clampU16(settings.charger.terminationCurrentMa, 64, 1024);
  settings.charger.chargeVoltageMv = clampU16(settings.charger.chargeVoltageMv, 3840, 4608);
  settings.charger.minSystemVoltageMv = clampU16(settings.charger.minSystemVoltageMv, 3000, 3700);
  settings.charger.boostVoltageMv = clampU16(settings.charger.boostVoltageMv, 4550, 5510);
  settings.charger.irCompResistanceMohm = clampU16(settings.charger.irCompResistanceMohm, 0, 140);
  settings.charger.irCompVoltageClampMv = clampU16(settings.charger.irCompVoltageClampMv, 0, 224);
}

void normalizeBatteryProfile() {
  settings.batteryProfile.designCapacityMah = clampU16(settings.batteryProfile.designCapacityMah, 1, 30000);
  settings.batteryProfile.designEnergyMwh = clampU16(settings.batteryProfile.designEnergyMwh, 1, 60000);
  settings.batteryProfile.terminateVoltageMv = clampU16(settings.batteryProfile.terminateVoltageMv, 2500, 3700);
  settings.batteryProfile.taperRate = clampU16(settings.batteryProfile.taperRate, 1, 2000);
  settings.batteryProfile.soc1SetPercent = clampU8(settings.batteryProfile.soc1SetPercent, 0, 100);
  settings.batteryProfile.soc1ClearPercent = clampU8(settings.batteryProfile.soc1ClearPercent, 0, 100);
  settings.batteryProfile.socfSetPercent = clampU8(settings.batteryProfile.socfSetPercent, 0, 100);
  settings.batteryProfile.socfClearPercent = clampU8(settings.batteryProfile.socfClearPercent, 0, 100);
  settings.batteryProfile.socIntDeltaPercent = clampU8(settings.batteryProfile.socIntDeltaPercent, 1, 100);
  settings.gauge.batteryProfile = settings.batteryProfile;
}

bool applyChargerSettingsToHardware(bool persist) {
  normalizeChargerSettings();
  power.setCePinEnabled(settings.cePinEnabled);
  bool ok = power.applyChargerSettings(settings.charger);
  if (persist) {
    saveChargerSettings();
  }
  return ok;
}

bool applyChargerSettingToHardware(const String &name, bool persist) {
  normalizeChargerSettings();
  bool ok = false;

  if (name == "cePinEnabled") ok = power.setCePinEnabled(settings.cePinEnabled);
  else if (name == "highImpedanceMode") ok = power.charger.setHighImpedanceMode(settings.charger.highImpedanceMode);
  else if (name == "ilimPinEnabled") ok = power.charger.setIlimPinEnabled(settings.charger.ilimPinEnabled);
  else if (name == "inputCurrentLimitMa") ok = power.restoreInputCurrentLimit(settings.charger.inputCurrentLimitMa);
  else if (name == "inputVoltageLimitMv") {
    ok = true;
    if (settings.charger.absoluteVindpm) {
      ok &= power.charger.setAbsoluteVindpm(true);
    }
    ok &= power.setInputVoltageLimit(settings.charger.inputVoltageLimitMv);
  }
  else if (name == "vindpmOffsetMv") ok = power.charger.setVindpmOffset(settings.charger.vindpmOffsetMv);
  else if (name == "chargeCurrentMa") ok = power.setChargeCurrent(settings.charger.chargeCurrentMa);
  else if (name == "prechargeCurrentMa") ok = power.charger.setPrechargeCurrent(settings.charger.prechargeCurrentMa);
  else if (name == "terminationCurrentMa") ok = power.charger.setTerminationCurrent(settings.charger.terminationCurrentMa);
  else if (name == "chargeVoltageMv") ok = power.setChargeVoltage(settings.charger.chargeVoltageMv);
  else if (name == "minSystemVoltageMv") ok = power.charger.setMinSystemVoltage(settings.charger.minSystemVoltageMv);
  else if (name == "boostVoltageMv") ok = power.setBoostVoltage(settings.charger.boostVoltageMv);
  else if (name == "irCompResistanceMohm") ok = power.charger.setIrCompResistance(settings.charger.irCompResistanceMohm);
  else if (name == "irCompVoltageClampMv") ok = power.charger.setIrCompVoltageClamp(settings.charger.irCompVoltageClampMv);
  else if (name == "watchdog") {
    ok = power.charger.setWatchdogTimer(settings.charger.watchdog);
    if (ok && settings.charger.watchdog != bq25::WatchdogOff) {
      ok &= power.charger.resetWatchdog();
      lastWatchdogServiceMs = millis();
    }
  }
  else if (name == "safetyTimer") ok = power.charger.setSafetyTimer(settings.charger.safetyTimer);
  else if (name == "thermalRegulation") ok = power.charger.setThermalRegulation(settings.charger.thermalRegulation);
  else if (name == "boostFrequency") ok = power.charger.setBoostFrequency(settings.charger.boostFrequency);
  else if (name == "boostHotThreshold") ok = power.charger.setBoostHotThreshold(settings.charger.boostHotThreshold);
  else if (name == "boostColdThreshold") ok = power.charger.setBoostColdThreshold(settings.charger.boostColdThreshold);
  else if (name == "batteryLowThreshold") ok = power.charger.setBatteryLowThreshold(settings.charger.batteryLowThreshold);
  else if (name == "rechargeThreshold") ok = power.charger.setRechargeThreshold(settings.charger.rechargeThreshold);
  else if (name == "chargingEnabled") ok = power.charger.setChargingEnabled(settings.charger.chargingEnabled);
  else if (name == "otgEnabled") {
    ok = power.charger.setOtgEnabled(settings.charger.otgEnabled);
    ok &= power.setOtgPinEnabled(settings.charger.otgEnabled);
  } else if (name == "batteryLoadEnabled") ok = power.charger.setBatteryLoadEnabled(settings.charger.batteryLoadEnabled);
  else if (name == "pumpXEnabled") ok = power.charger.setPumpXEnabled(settings.charger.pumpXEnabled);
  else if (name == "terminationEnabled") ok = power.charger.setTerminationEnabled(settings.charger.terminationEnabled);
  else if (name == "safetyTimerEnabled") ok = power.charger.setSafetyTimerEnabled(settings.charger.safetyTimerEnabled);
  else if (name == "safetyTimerSlowedInDpm") ok = power.charger.setSafetyTimerSlowedInDpm(settings.charger.safetyTimerSlowedInDpm);
  else if (name == "autoDpdmEnabled") ok = power.charger.setAutoDpdmEnabled(settings.charger.autoDpdmEnabled);
  else if (name == "hvdcpEnabled") ok = power.charger.setHvdcpEnabled(settings.charger.hvdcpEnabled);
  else if (name == "maxChargeEnabled") ok = power.charger.setMaxChargeEnabled(settings.charger.maxChargeEnabled);
  else if (name == "icoEnabled") ok = power.charger.setIcoEnabled(settings.charger.icoEnabled);
  else if (name == "adcContinuous") ok = settings.charger.adcContinuous ? power.charger.startAdc(true) : power.charger.stopAdc();
  else if (name == "absoluteVindpm") {
    ok = power.charger.setAbsoluteVindpm(settings.charger.absoluteVindpm);
    if (ok && settings.charger.absoluteVindpm) {
      ok &= power.setInputVoltageLimit(settings.charger.inputVoltageLimitMv);
    }
  }
  else if (name == "batfetResetEnabled") ok = power.charger.setBatfetResetEnabled(settings.charger.batfetResetEnabled);

  if (ok && !settings.charger.autoDpdmEnabled && !settings.charger.icoEnabled) {
    ok &= power.restoreInputCurrentLimit(settings.charger.inputCurrentLimitMa);
  }

  if (persist && ok) {
    saveChargerSettings();
  }
  return ok;
}

bool applyChangedChargerSettingsToHardware(bool oldCePinEnabled,
                                           const Batthub::ChargerSettings &old,
                                           bool persist) {
  normalizeChargerSettings();
  bool ok = true;

  if (settings.cePinEnabled != oldCePinEnabled) ok &= applyChargerSettingToHardware("cePinEnabled", false);
  if (settings.charger.highImpedanceMode != old.highImpedanceMode) ok &= applyChargerSettingToHardware("highImpedanceMode", false);
  if (settings.charger.ilimPinEnabled != old.ilimPinEnabled) ok &= applyChargerSettingToHardware("ilimPinEnabled", false);
  if (settings.charger.inputCurrentLimitMa != old.inputCurrentLimitMa) ok &= applyChargerSettingToHardware("inputCurrentLimitMa", false);
  if (settings.charger.inputVoltageLimitMv != old.inputVoltageLimitMv) ok &= applyChargerSettingToHardware("inputVoltageLimitMv", false);
  if (settings.charger.vindpmOffsetMv != old.vindpmOffsetMv) ok &= applyChargerSettingToHardware("vindpmOffsetMv", false);
  if (settings.charger.chargeCurrentMa != old.chargeCurrentMa) ok &= applyChargerSettingToHardware("chargeCurrentMa", false);
  if (settings.charger.prechargeCurrentMa != old.prechargeCurrentMa) ok &= applyChargerSettingToHardware("prechargeCurrentMa", false);
  if (settings.charger.terminationCurrentMa != old.terminationCurrentMa) ok &= applyChargerSettingToHardware("terminationCurrentMa", false);
  if (settings.charger.chargeVoltageMv != old.chargeVoltageMv) ok &= applyChargerSettingToHardware("chargeVoltageMv", false);
  if (settings.charger.minSystemVoltageMv != old.minSystemVoltageMv) ok &= applyChargerSettingToHardware("minSystemVoltageMv", false);
  if (settings.charger.boostVoltageMv != old.boostVoltageMv) ok &= applyChargerSettingToHardware("boostVoltageMv", false);
  if (settings.charger.irCompResistanceMohm != old.irCompResistanceMohm) ok &= applyChargerSettingToHardware("irCompResistanceMohm", false);
  if (settings.charger.irCompVoltageClampMv != old.irCompVoltageClampMv) ok &= applyChargerSettingToHardware("irCompVoltageClampMv", false);
  if (settings.charger.watchdog != old.watchdog) ok &= applyChargerSettingToHardware("watchdog", false);
  if (settings.charger.safetyTimer != old.safetyTimer) ok &= applyChargerSettingToHardware("safetyTimer", false);
  if (settings.charger.thermalRegulation != old.thermalRegulation) ok &= applyChargerSettingToHardware("thermalRegulation", false);
  if (settings.charger.boostFrequency != old.boostFrequency) ok &= applyChargerSettingToHardware("boostFrequency", false);
  if (settings.charger.boostHotThreshold != old.boostHotThreshold) ok &= applyChargerSettingToHardware("boostHotThreshold", false);
  if (settings.charger.boostColdThreshold != old.boostColdThreshold) ok &= applyChargerSettingToHardware("boostColdThreshold", false);
  if (settings.charger.batteryLowThreshold != old.batteryLowThreshold) ok &= applyChargerSettingToHardware("batteryLowThreshold", false);
  if (settings.charger.rechargeThreshold != old.rechargeThreshold) ok &= applyChargerSettingToHardware("rechargeThreshold", false);
  if (settings.charger.chargingEnabled != old.chargingEnabled) ok &= applyChargerSettingToHardware("chargingEnabled", false);
  if (settings.charger.otgEnabled != old.otgEnabled) ok &= applyChargerSettingToHardware("otgEnabled", false);
  if (settings.charger.batteryLoadEnabled != old.batteryLoadEnabled) ok &= applyChargerSettingToHardware("batteryLoadEnabled", false);
  if (settings.charger.pumpXEnabled != old.pumpXEnabled) ok &= applyChargerSettingToHardware("pumpXEnabled", false);
  if (settings.charger.terminationEnabled != old.terminationEnabled) ok &= applyChargerSettingToHardware("terminationEnabled", false);
  if (settings.charger.safetyTimerEnabled != old.safetyTimerEnabled) ok &= applyChargerSettingToHardware("safetyTimerEnabled", false);
  if (settings.charger.safetyTimerSlowedInDpm != old.safetyTimerSlowedInDpm) ok &= applyChargerSettingToHardware("safetyTimerSlowedInDpm", false);
  if (settings.charger.autoDpdmEnabled != old.autoDpdmEnabled) ok &= applyChargerSettingToHardware("autoDpdmEnabled", false);
  if (settings.charger.hvdcpEnabled != old.hvdcpEnabled) ok &= applyChargerSettingToHardware("hvdcpEnabled", false);
  if (settings.charger.maxChargeEnabled != old.maxChargeEnabled) ok &= applyChargerSettingToHardware("maxChargeEnabled", false);
  if (settings.charger.icoEnabled != old.icoEnabled) ok &= applyChargerSettingToHardware("icoEnabled", false);
  if (settings.charger.adcContinuous != old.adcContinuous) ok &= applyChargerSettingToHardware("adcContinuous", false);
  if (settings.charger.absoluteVindpm != old.absoluteVindpm) ok &= applyChargerSettingToHardware("absoluteVindpm", false);
  if (settings.charger.batfetResetEnabled != old.batfetResetEnabled) ok &= applyChargerSettingToHardware("batfetResetEnabled", false);

  if (persist && ok) {
    saveChargerSettings();
  }
  return ok;
}

bool applyGaugeProfileToHardware(bool persist) {
  normalizeBatteryProfile();
  bool ok = power.configureBattery(settings.batteryProfile);
  settings.gauge.batteryProfile = settings.batteryProfile;
  if (persist) {
    saveGaugeSettings();
  }
  return ok;
}

bool applyGaugeOptionsToHardware(bool persist) {
  bool ok = power.applyGaugeSettings(settings.gauge, false);
  if (persist) {
    saveGaugeSettings();
  }
  return ok;
}

bool applySafeDefaults(bool persist) {
  settings.cePinEnabled = true;
  settings.charger = Batthub::ChargerSettings();
  return applyChargerSettingsToHardware(persist);
}

bool applySafeChargeMode(bool persist) {
  settings.cePinEnabled = true;
  settings.charger.highImpedanceMode = false;
  settings.charger.ilimPinEnabled = false;
  settings.charger.inputCurrentLimitMa = 1000;
  settings.charger.inputVoltageLimitMv = 4400;
  settings.charger.vindpmOffsetMv = 0;
  settings.charger.chargeCurrentMa = 500;
  settings.charger.prechargeCurrentMa = 128;
  settings.charger.terminationCurrentMa = 128;
  settings.charger.chargeVoltageMv = 4208;
  settings.charger.minSystemVoltageMv = 3500;
  settings.charger.chargingEnabled = true;
  settings.charger.otgEnabled = false;
  settings.charger.batteryLoadEnabled = false;
  settings.charger.pumpXEnabled = false;
  settings.charger.terminationEnabled = true;
  settings.charger.safetyTimerEnabled = true;
  settings.charger.safetyTimerSlowedInDpm = true;
  settings.charger.adcContinuous = true;
  settings.charger.watchdog = bq25::WatchdogOff;
  settings.charger.autoDpdmEnabled = false;
  settings.charger.hvdcpEnabled = false;
  settings.charger.maxChargeEnabled = false;
  settings.charger.icoEnabled = false;
  settings.charger.absoluteVindpm = true;
  settings.charger.batfetResetEnabled = true;
  normalizeChargerSettings();
  return applyChargerSettingsToHardware(persist);
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

  if (!settings.charger.autoDpdmEnabled && !settings.charger.icoEnabled) {
    power.restoreInputCurrentLimit(settings.charger.inputCurrentLimitMa);
  }
}

void serviceChargerWatchdog() {
  if (settings.charger.watchdog == bq25::WatchdogOff) {
    return;
  }

  uint32_t now = millis();
  if (now - lastWatchdogServiceMs < WATCHDOG_SERVICE_INTERVAL_MS) {
    return;
  }

  if (power.charger.resetWatchdog()) {
    lastWatchdogServiceMs = now;
  }
}

// ================= JSON =================
String chargerActualJson(const uint8_t *regs) {
  String j = "{";
  appendBoolField(j, "highImpedanceMode", (regs[bq25::RegInputCurrent] & 0x80) != 0);
  appendBoolField(j, "ilimPinEnabled", (regs[bq25::RegInputCurrent] & 0x40) != 0);
  appendU16Field(j, "inputCurrentLimitMa", decodeLinear(field(regs[bq25::RegInputCurrent], 0x3F, 0), 100, 50));
  appendU16Field(j, "vindpmOffsetMv", decodeLinear(field(regs[bq25::RegVindpmOffset], 0x1F, 0), 0, 100));
  appendU16Field(j, "inputVoltageLimitMv", decodeLinear(field(regs[bq25::RegVindpm], 0x7F, 0), 2600, 100));
  appendU16Field(j, "chargeCurrentMa", decodeLinear(field(regs[bq25::RegChargeCurrent], 0x7F, 0), 0, 64));
  appendU16Field(j, "prechargeCurrentMa", decodeLinear(field(regs[bq25::RegPrechargeTermination], 0xF0, 4), 64, 64));
  appendU16Field(j, "terminationCurrentMa", decodeLinear(field(regs[bq25::RegPrechargeTermination], 0x0F, 0), 64, 64));
  appendU16Field(j, "chargeVoltageMv", decodeLinear(field(regs[bq25::RegChargeVoltage], 0xFC, 2), 3840, 16));
  appendU16Field(j, "minSystemVoltageMv", decodeLinear(field(regs[bq25::RegPowerPath], 0x0E, 1), 3000, 100));
  appendU16Field(j, "boostVoltageMv", decodeLinear(field(regs[bq25::RegBoostVoltage], 0xF0, 4), 4550, 64));
  appendU16Field(j, "irCompResistanceMohm", decodeLinear(field(regs[bq25::RegIrCompThermal], 0xE0, 5), 0, 20));
  appendU16Field(j, "irCompVoltageClampMv", decodeLinear(field(regs[bq25::RegIrCompThermal], 0x1C, 2), 0, 32));
  appendStringField(j, "watchdog", watchdogName(field(regs[bq25::RegChargeControl], 0x30, 4)));
  appendStringField(j, "safetyTimer", safetyTimerName(field(regs[bq25::RegChargeControl], 0x06, 1)));
  appendStringField(j, "thermalRegulation", thermalRegulationName(field(regs[bq25::RegIrCompThermal], 0x03, 0)));
  appendStringField(j, "boostFrequency", boostFrequencyName(field(regs[bq25::RegAdcAndDpdm], 0x20, 5)));
  appendStringField(j, "boostHotThreshold", boostHotName(field(regs[bq25::RegVindpmOffset], 0xC0, 6)));
  appendStringField(j, "boostColdThreshold", boostColdName(field(regs[bq25::RegVindpmOffset], 0x20, 5)));
  appendStringField(j, "batteryLowThreshold", batteryLowName(field(regs[bq25::RegChargeVoltage], 0x02, 1)));
  appendStringField(j, "rechargeThreshold", rechargeName(field(regs[bq25::RegChargeVoltage], 0x01, 0)));
  appendBoolField(j, "chargingEnabled", (regs[bq25::RegPowerPath] & 0x10) != 0);
  appendBoolField(j, "otgEnabled", (regs[bq25::RegPowerPath] & 0x20) != 0);
  appendBoolField(j, "batteryLoadEnabled", (regs[bq25::RegPowerPath] & 0x80) != 0);
  appendBoolField(j, "pumpXEnabled", (regs[bq25::RegChargeCurrent] & 0x80) != 0);
  appendBoolField(j, "terminationEnabled", (regs[bq25::RegChargeControl] & 0x80) != 0);
  appendBoolField(j, "safetyTimerEnabled", (regs[bq25::RegChargeControl] & 0x08) != 0);
  appendBoolField(j, "safetyTimerSlowedInDpm", (regs[bq25::RegMiscControl] & 0x40) != 0);
  appendBoolField(j, "icoEnabled", (regs[bq25::RegAdcAndDpdm] & 0x10) != 0);
  appendBoolField(j, "autoDpdmEnabled", (regs[bq25::RegAdcAndDpdm] & 0x01) != 0);
  appendBoolField(j, "hvdcpEnabled", (regs[bq25::RegAdcAndDpdm] & 0x08) != 0);
  appendBoolField(j, "maxChargeEnabled", (regs[bq25::RegAdcAndDpdm] & 0x04) != 0);
  appendBoolField(j, "adcContinuous", (regs[bq25::RegAdcAndDpdm] & 0x40) != 0);
  appendBoolField(j, "absoluteVindpm", (regs[bq25::RegVindpm] & 0x80) != 0);
  appendBoolField(j, "batfetResetEnabled", (regs[bq25::RegMiscControl] & 0x04) != 0, false);
  j += "}";
  return j;
}

String chargerSettingsJson() {
  String j = "{";
  appendBoolField(j, "cePinEnabled", settings.cePinEnabled);
  appendBoolField(j, "highImpedanceMode", settings.charger.highImpedanceMode);
  appendBoolField(j, "ilimPinEnabled", settings.charger.ilimPinEnabled);
  appendU16Field(j, "inputCurrentLimitMa", settings.charger.inputCurrentLimitMa);
  appendU16Field(j, "inputVoltageLimitMv", settings.charger.inputVoltageLimitMv);
  appendU16Field(j, "vindpmOffsetMv", settings.charger.vindpmOffsetMv);
  appendU16Field(j, "chargeCurrentMa", settings.charger.chargeCurrentMa);
  appendU16Field(j, "prechargeCurrentMa", settings.charger.prechargeCurrentMa);
  appendU16Field(j, "terminationCurrentMa", settings.charger.terminationCurrentMa);
  appendU16Field(j, "chargeVoltageMv", settings.charger.chargeVoltageMv);
  appendU16Field(j, "minSystemVoltageMv", settings.charger.minSystemVoltageMv);
  appendU16Field(j, "boostVoltageMv", settings.charger.boostVoltageMv);
  appendU16Field(j, "irCompResistanceMohm", settings.charger.irCompResistanceMohm);
  appendU16Field(j, "irCompVoltageClampMv", settings.charger.irCompVoltageClampMv);
  appendU16Field(j, "watchdog", settings.charger.watchdog);
  appendU16Field(j, "safetyTimer", settings.charger.safetyTimer);
  appendU16Field(j, "thermalRegulation", settings.charger.thermalRegulation);
  appendU16Field(j, "boostFrequency", settings.charger.boostFrequency);
  appendU16Field(j, "boostHotThreshold", settings.charger.boostHotThreshold);
  appendU16Field(j, "boostColdThreshold", settings.charger.boostColdThreshold);
  appendU16Field(j, "batteryLowThreshold", settings.charger.batteryLowThreshold);
  appendU16Field(j, "rechargeThreshold", settings.charger.rechargeThreshold);
  appendBoolField(j, "chargingEnabled", settings.charger.chargingEnabled);
  appendBoolField(j, "otgEnabled", settings.charger.otgEnabled);
  appendBoolField(j, "batteryLoadEnabled", settings.charger.batteryLoadEnabled);
  appendBoolField(j, "pumpXEnabled", settings.charger.pumpXEnabled);
  appendBoolField(j, "terminationEnabled", settings.charger.terminationEnabled);
  appendBoolField(j, "safetyTimerEnabled", settings.charger.safetyTimerEnabled);
  appendBoolField(j, "safetyTimerSlowedInDpm", settings.charger.safetyTimerSlowedInDpm);
  appendBoolField(j, "icoEnabled", settings.charger.icoEnabled);
  appendBoolField(j, "autoDpdmEnabled", settings.charger.autoDpdmEnabled);
  appendBoolField(j, "hvdcpEnabled", settings.charger.hvdcpEnabled);
  appendBoolField(j, "maxChargeEnabled", settings.charger.maxChargeEnabled);
  appendBoolField(j, "adcContinuous", settings.charger.adcContinuous);
  appendBoolField(j, "absoluteVindpm", settings.charger.absoluteVindpm);
  appendBoolField(j, "batfetResetEnabled", settings.charger.batfetResetEnabled, false);
  j += "}";
  return j;
}

String batteryProfileJson(const Batthub::BatteryProfile &profile) {
  String j = "{";
  appendU16Field(j, "designCapacityMah", profile.designCapacityMah);
  appendU16Field(j, "designEnergyMwh", profile.designEnergyMwh);
  appendU16Field(j, "terminateVoltageMv", profile.terminateVoltageMv);
  appendU16Field(j, "taperRate", profile.taperRate);
  appendU16Field(j, "soc1SetPercent", profile.soc1SetPercent);
  appendU16Field(j, "soc1ClearPercent", profile.soc1ClearPercent);
  appendU16Field(j, "socfSetPercent", profile.socfSetPercent);
  appendU16Field(j, "socfClearPercent", profile.socfClearPercent);
  appendU16Field(j, "socIntDeltaPercent", profile.socIntDeltaPercent, false);
  j += "}";
  return j;
}

String gaugeSettingsJson() {
  String j = "{";
  j += "\"batteryProfile\":";
  j += batteryProfileJson(settings.batteryProfile);
  j += ",";
  appendU16Field(j, "gpoutMode", settings.gauge.gpoutMode);
  appendU16Field(j, "temperatureSource", settings.gauge.temperatureSource);
  appendBoolField(j, "gpoutActiveHigh", settings.gauge.gpoutActiveHigh);
  appendBoolField(j, "batteryInsertionEnabled", settings.gauge.batteryInsertionEnabled);
  appendBoolField(j, "binPullupEnabled", settings.gauge.binPullupEnabled);
  appendBoolField(j, "sleepEnabled", settings.gauge.sleepEnabled);
  appendBoolField(j, "rmFccSmoothingEnabled", settings.gauge.rmFccSmoothingEnabled, false);
  j += "}";
  return j;
}

String gaugeOptionsActualJson() {
  uint16_t opConfig = power.gauge.opConfig();
  String j = "{";
  appendStringField(j, "gpoutMode", (opConfig & (1u << 2)) ? "Battery low" : "SOC interrupt");
  appendStringField(j, "temperatureSource", (opConfig & 1u) ? "Host provided" : "Internal sensor");
  appendBoolField(j, "gpoutActiveHigh", (opConfig & (1u << 11)) != 0);
  appendBoolField(j, "batteryInsertionEnabled", (opConfig & (1u << 13)) != 0);
  appendBoolField(j, "binPullupEnabled", (opConfig & (1u << 12)) != 0);
  appendBoolField(j, "sleepEnabled", (opConfig & (1u << 5)) != 0);
  appendBoolField(j, "rmFccSmoothingEnabled", (opConfig & (1u << 4)) != 0, false);
  j += "}";
  return j;
}

String statusJson() {
  uint8_t regs[bq25::kRegisterCount];
  bool regsOk = power.charger.readRegisters(regs, bq25::kRegisterCount);
  bool bq25ok = power.charger.isConnected() && regsOk;
  bool bq27ok = power.gauge.isConnected();

  bq25::Status chargerStatus = power.charger.status();
  bq25::Faults chargerFaults = power.charger.faults();
  bq25::Measurements chargerMeasurements = power.charger.measurements();

  String j = "{";
  appendStringField(j, "ip", currentIpString());
  appendStringField(j, "wifiMode", wifiApMode ? "Access Point" : "WiFi Client");

  const Batthub::Pins &pins = power.pins();
  j += "\"hostPins\":{";
  appendU16Field(j, "sda", SDA_PIN);
  appendU16Field(j, "scl", SCL_PIN);
  appendI16Field(j, "ce", pins.ce);
  appendI16Field(j, "otg", pins.otg);
  appendI16Field(j, "intPin", pins.intPin);
  appendBoolField(j, "cePinEnabled", power.cePinEnabled());
  appendBoolField(j, "otgPinEnabled", power.otgPinEnabled());
  appendBoolField(j, "interruptAsserted", power.interruptAsserted(), false);
  j += "},";

  j += "\"settings\":{";
  j += "\"charger\":";
  j += chargerSettingsJson();
  j += ",\"batteryProfile\":";
  j += batteryProfileJson(settings.batteryProfile);
  j += ",\"gauge\":";
  j += gaugeSettingsJson();
  j += "},";

  j += "\"bq25\":{";
  appendBoolField(j, "present", bq25ok);
  if (bq25ok) {
    j += "\"live\":{";
    appendStringField(j, "inputType", bq25::vbusTypeName(chargerStatus.vbusType));
    appendStringField(j, "chargeState", bq25::chargeStateName(chargerStatus.chargeState));
    appendBoolField(j, "powerGood", chargerStatus.powerGood);
    appendBoolField(j, "systemLow", chargerStatus.systemInRegulation);
    appendBoolField(j, "vbusGood", chargerMeasurements.vbusGood);
    appendBoolField(j, "inputCurrentLimited", chargerMeasurements.iindpm);
    appendBoolField(j, "inputVoltageLimited", chargerMeasurements.vindpm);
    appendBoolField(j, "thermalRegulation", chargerMeasurements.thermalRegulation);
    appendU16Field(j, "inputVoltageMv", chargerMeasurements.vbusMv);
    appendU16Field(j, "batteryVoltageMv", chargerMeasurements.batteryMv);
    appendU16Field(j, "systemVoltageMv", chargerMeasurements.systemMv);
    appendU16Field(j, "chargeCurrentMa", chargerMeasurements.chargeCurrentMa);
    appendU16Field(j, "effectiveInputCurrentLimitMa", chargerMeasurements.effectiveInputCurrentLimitMa);
    appendU16Field(j, "tsPercentX100", chargerMeasurements.tsPercentX100, false);
    j += "},\"faults\":{";
    appendBoolField(j, "watchdog", chargerFaults.watchdog);
    appendBoolField(j, "boost", chargerFaults.boost);
    appendStringField(j, "charge", bq25::chargeFaultName(chargerFaults.charge));
    appendBoolField(j, "battery", chargerFaults.battery);
    appendStringField(j, "ntc", bq25::ntcFaultName(chargerFaults.ntc), false);
    j += "},\"actual\":";
    j += chargerActualJson(regs);
  }
  j += "},";

  j += "\"bq27\":{";
  appendBoolField(j, "present", bq27ok);
  if (bq27ok) {
    bq27::Snapshot gauge = power.gauge.snapshot();
    bq27::ControlStatusBits control = power.gauge.controlStatusBits();
    j += "\"live\":{";
    appendU16Field(j, "voltageMv", gauge.voltageMv);
    appendI16Field(j, "averageCurrentMa", gauge.averageCurrentMa);
    appendI16Field(j, "standbyCurrentMa", gauge.standbyCurrentMa);
    appendI16Field(j, "maxLoadCurrentMa", gauge.maxLoadCurrentMa);
    appendI16Field(j, "averagePowerMw", gauge.averagePowerMw);
    appendU16Field(j, "stateOfChargePercent", gauge.stateOfChargePercent);
    appendU16Field(j, "stateOfChargeUnfilteredPercent", gauge.stateOfChargeUnfilteredPercent);
    appendU16Field(j, "remainingCapacityMah", gauge.remainingCapacityMah);
    appendU16Field(j, "fullChargeCapacityMah", gauge.fullChargeCapacityMah);
    appendU16Field(j, "stateOfHealthPercent", gauge.stateOfHealthPercent);
    appendI16Field(j, "temperatureCelsiusX10", gauge.temperatureCelsiusX10);
    appendI16Field(j, "internalTemperatureCelsiusX10", gauge.internalTemperatureCelsiusX10, false);
    j += "},\"flags\":{";
    appendBoolField(j, "fullCharge", gauge.flags.fullCharge);
    appendBoolField(j, "charging", gauge.flags.charging);
    appendBoolField(j, "discharging", gauge.flags.discharging);
    appendBoolField(j, "batteryDetected", gauge.flags.batteryDetected);
    appendBoolField(j, "lowBattery", gauge.flags.soc1);
    appendBoolField(j, "finalLowBattery", gauge.flags.socFinal);
    appendBoolField(j, "powerOnReset", gauge.flags.powerOnReset);
    appendBoolField(j, "configUpdateMode", gauge.flags.configUpdateMode);
    appendBoolField(j, "overTemperature", gauge.flags.overTemperature);
    appendBoolField(j, "underTemperature", gauge.flags.underTemperature, false);
    j += "},\"control\":{";
    appendBoolField(j, "sealed", control.sealed);
    appendBoolField(j, "hibernate", control.hibernate);
    appendBoolField(j, "sleep", control.sleep);
    appendBoolField(j, "initComplete", control.initComplete);
    appendBoolField(j, "voltageOk", control.voltageOk);
    appendBoolField(j, "shutdownEnabled", control.shutdownEnabled, false);
    j += "},\"actualOptions\":";
    j += gaugeOptionsActualJson();
    j += ",\"lastReadBatteryProfileValid\":";
    j += boolJson(lastReadBatteryProfileValid);
    if (lastReadBatteryProfileValid) {
      j += ",\"lastReadBatteryProfile\":";
      j += batteryProfileJson(lastReadBatteryProfile);
    }
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
<title>Batthub Web UI</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
:root{--bg:#0d1017;--panel:#151a22;--line:#2a3240;--text:#edf2f8;--muted:#97a3b6;--ok:#23c878;--bad:#ff5b6e;--warn:#ffc857;--accent:#5aa7ff}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font-family:Arial,Helvetica,sans-serif}
header{padding:16px 18px;background:#111722;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;gap:12px;align-items:center;flex-wrap:wrap}
h1{font-size:21px;margin:0}.muted{color:var(--muted);font-size:13px}.wrap{padding:14px;display:grid;gap:14px}
.tabs{display:flex;gap:8px;flex-wrap:wrap}.tabbtn{border:1px solid var(--line);background:#1c2330;color:var(--text);border-radius:8px;padding:11px 14px;cursor:pointer}.tabbtn.active{background:#234b75;border-color:var(--accent)}
.tab{display:none}.tab.active{display:grid;gap:14px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:14px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px}.card h2{font-size:16px;margin:0 0 12px}.card h3{font-size:14px;margin:14px 0 8px;color:#c8d3e3}
.row{display:flex;justify-content:space-between;align-items:center;gap:12px;border-bottom:1px solid #222a36;padding:8px 0}.row:last-child{border-bottom:0}.row b{text-align:right}
.metric{font-size:27px;font-weight:bold}.ok{color:var(--ok)}.bad{color:var(--bad)}.warn{color:var(--warn)}.blue{color:var(--accent)}
	.badge{display:inline-block;border-radius:999px;padding:5px 9px;font-size:12px;font-weight:bold;background:#222b38}.badge.ok{background:#123826;color:var(--ok)}.badge.bad{background:#3c1820;color:var(--bad)}.badge.warn{background:#3b3217;color:var(--warn)}
	.notice{display:grid;gap:7px;line-height:1.35}.notice strong{font-size:15px}.notice.ok strong{color:var(--ok)}.notice.warn strong{color:var(--warn)}.notice.bad strong{color:var(--bad)}.riskmark{outline:1px solid rgba(255,200,87,.45)}
	.formgrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px}.field{display:grid;gap:5px}.field label{font-size:12px;color:var(--muted)}input,select{width:100%;background:#0f141d;color:var(--text);border:1px solid #354154;border-radius:7px;padding:9px}input[type=checkbox]{width:auto}
	.checkgrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(175px,1fr));gap:8px}.toggle{display:flex;align-items:center;justify-content:space-between;gap:10px;background:#3b1820;border:1px solid #893543;color:#ffd7dd;border-radius:7px;padding:10px;cursor:pointer;text-align:left;font-weight:bold}.toggle.on{background:#123826;border-color:#23c878;color:#caffdf}.toggle.pending{opacity:.6}.toggle b{font-size:12px;border-radius:999px;padding:4px 8px;background:rgba(0,0,0,.28);min-width:42px;text-align:center}
	.actions{display:flex;flex-wrap:wrap;gap:8px;margin-top:12px}button{background:#202a38;color:white;border:1px solid #39465a;border-radius:7px;padding:10px 12px;cursor:pointer}button.primary{background:#1f5f93;border-color:#5aa7ff}button.danger{background:#421b23;border-color:#ff5b6e}button.warn{background:#463a18;border-color:#ffc857}button.safe{background:#153823;border-color:#23c878}button.unlocked{background:#3b3217;border-color:#ffc857}
@media(max-width:620px){header{align-items:flex-start}.row{align-items:flex-start;flex-direction:column}.row b{text-align:left}.metric{font-size:23px}}
</style>
</head>
<body>
<header>
  <div>
    <h1>Batthub Web UI</h1>
    <div class="muted" id="topline">Connecting...</div>
  </div>
	  <div class="actions">
	    <button id="expertLockButton" class="warn" onclick="toggleExpertLock()">Expert Locked</button>
	    <button class="safe" onclick="cmd('safe_charge')">Safe Charge</button>
	    <button class="warn" onclick="cmdDanger('reset_both','Reset both chips?')">Reset Both</button>
	  </div>
	</header>

<div class="wrap">
  <div class="tabs">
    <button id="tabBtnBq25" class="tabbtn active" onclick="showTab('bq25')">BQ25895 Charger</button>
    <button id="tabBtnBq27" class="tabbtn" onclick="showTab('bq27')">BQ27441 Gauge</button>
  </div>

  <section id="tabBq25" class="tab active">
	    <div class="grid">
	      <div class="card"><h2>Charge State</h2><div id="chargeStateMetric" class="metric">---</div><div class="muted" id="chargeStateNote">---</div></div>
	      <div class="card"><h2>Input Voltage</h2><div id="inputVoltageMetric" class="metric">---</div><div class="muted" id="inputTypeNote">---</div></div>
	      <div class="card"><h2>Battery Voltage</h2><div id="batteryVoltageMetric" class="metric">---</div><div class="muted" id="systemVoltageNote">---</div></div>
	      <div class="card"><h2>Charge Current</h2><div id="chargeCurrentMetric" class="metric blue">---</div><div class="muted">Measured by charger ADC</div></div>
	    </div>
	    <div class="card">
	      <h2>Charge Diagnosis</h2>
	      <div id="chargeDiagnosis" class="notice"><strong>---</strong><span>---</span></div>
	    </div>

    <div class="grid">
      <div class="card">
        <h2>BQ25895 Readings</h2>
        <div class="row"><span>Charge Blocker</span><b id="chargeBlockers">---</b></div>
        <div class="row"><span>Power Good</span><b id="powerGood">---</b></div>
        <div class="row"><span>Input Current Limit</span><b id="effectiveInputCurrentLimitMa">---</b></div>
        <div class="row"><span>Input Current Limited</span><b id="inputCurrentLimited">---</b></div>
        <div class="row"><span>Input Voltage Limited</span><b id="inputVoltageLimited">---</b></div>
        <div class="row"><span>Thermal Regulation</span><b id="thermalRegulationStatus">---</b></div>
        <div class="row"><span>TS / NTC</span><b id="tsPercent">---</b></div>
      </div>

      <div class="card">
        <h2>BQ25895 Faults</h2>
        <div class="row"><span>Charge Fault</span><b id="chargeFault">---</b></div>
        <div class="row"><span>NTC Fault</span><b id="ntcFault">---</b></div>
        <div class="row"><span>Watchdog Fault</span><b id="watchdogFault">---</b></div>
        <div class="row"><span>Boost Fault</span><b id="boostFault">---</b></div>
        <div class="row"><span>Battery Fault</span><b id="batteryFault">---</b></div>
      </div>

      <div class="card">
        <h2>Actual BQ25895 Settings</h2>
        <div class="row"><span>Charge Current</span><b id="actualChargeCurrentMa">---</b></div>
        <div class="row"><span>Charge Voltage</span><b id="actualChargeVoltageMv">---</b></div>
        <div class="row"><span>Input Limit</span><b id="actualInputCurrentLimitMa">---</b></div>
        <div class="row"><span>Input Voltage Limit</span><b id="actualInputVoltageLimitMv">---</b></div>
        <div class="row"><span>Absolute VINDPM</span><b id="actualAbsoluteVindpm">---</b></div>
        <div class="row"><span>VINDPM Offset</span><b id="actualVindpmOffsetMv">---</b></div>
        <div class="row"><span>Watchdog</span><b id="actualWatchdog">---</b></div>
        <div class="row"><span>Safety Timer</span><b id="actualSafetyTimer">---</b></div>
        <div class="row"><span>Thermal Limit</span><b id="actualThermalRegulation">---</b></div>
      </div>
    </div>

    <div class="card">
      <h2>BQ25895 Settings</h2>
      <div class="checkgrid">
        <button id="cePinEnabled" class="toggle" onclick="toggleSetting('charger','cePinEnabled')"><span>CE Pin Enabled</span><b>OFF</b></button>
        <button id="chargingEnabled" class="toggle" onclick="toggleSetting('charger','chargingEnabled')"><span>Charging Enabled</span><b>OFF</b></button>
        <button id="otgEnabled" class="toggle" onclick="toggleSetting('charger','otgEnabled')"><span>OTG Boost Enabled</span><b>OFF</b></button>
        <button id="adcContinuous" class="toggle" onclick="toggleSetting('charger','adcContinuous')"><span>ADC Continuous</span><b>OFF</b></button>
        <button id="highImpedanceMode" class="toggle" onclick="toggleSetting('charger','highImpedanceMode')"><span>High Impedance</span><b>OFF</b></button>
        <button id="ilimPinEnabled" class="toggle" onclick="toggleSetting('charger','ilimPinEnabled')"><span>ILIM Pin Enabled</span><b>OFF</b></button>
        <button id="terminationEnabled" class="toggle" onclick="toggleSetting('charger','terminationEnabled')"><span>Charge Termination</span><b>OFF</b></button>
        <button id="safetyTimerEnabled" class="toggle" onclick="toggleSetting('charger','safetyTimerEnabled')"><span>Safety Timer Enabled</span><b>OFF</b></button>
        <button id="safetyTimerSlowedInDpm" class="toggle" onclick="toggleSetting('charger','safetyTimerSlowedInDpm')"><span>Slow Timer In DPM</span><b>OFF</b></button>
        <button id="autoDpdmEnabled" class="toggle" onclick="toggleSetting('charger','autoDpdmEnabled')"><span>Auto DPDM</span><b>OFF</b></button>
        <button id="hvdcpEnabled" class="toggle" onclick="toggleSetting('charger','hvdcpEnabled')"><span>HVDCP</span><b>OFF</b></button>
        <button id="maxChargeEnabled" class="toggle" onclick="toggleSetting('charger','maxChargeEnabled')"><span>MaxCharge</span><b>OFF</b></button>
        <button id="icoEnabled" class="toggle" onclick="toggleSetting('charger','icoEnabled')"><span>ICO</span><b>OFF</b></button>
        <button id="batteryLoadEnabled" class="toggle" onclick="toggleSetting('charger','batteryLoadEnabled')"><span>Battery Load Test</span><b>OFF</b></button>
        <button id="pumpXEnabled" class="toggle" onclick="toggleSetting('charger','pumpXEnabled')"><span>PumpX</span><b>OFF</b></button>
        <button id="absoluteVindpm" class="toggle" onclick="toggleSetting('charger','absoluteVindpm')"><span>Absolute VINDPM</span><b>OFF</b></button>
        <button id="batfetResetEnabled" class="toggle" onclick="toggleSetting('charger','batfetResetEnabled')"><span>BATFET Reset Enable</span><b>OFF</b></button>
      </div>

      <h3>Current And Voltage</h3>
      <div class="formgrid">
	        <div class="field"><label>inputCurrentLimitMa</label><input id="inputCurrentLimitMa" type="number" min="100" max="3250" step="50"></div>
	        <div class="field"><label>inputVoltageLimitMv</label><input id="inputVoltageLimitMv" type="number" min="2600" max="15300" step="100"></div>
	        <div class="field"><label>vindpmOffsetMv</label><input id="vindpmOffsetMv" type="number" min="0" max="3100" step="100"></div>
	        <div class="field"><label>chargeCurrentMa</label><input id="chargeCurrentMa" type="number" min="0" max="5056" step="64"></div>
	        <div class="field"><label>prechargeCurrentMa</label><input id="prechargeCurrentMa" type="number" min="64" max="1024" step="64"></div>
	        <div class="field"><label>terminationCurrentMa</label><input id="terminationCurrentMa" type="number" min="64" max="1024" step="64"></div>
	        <div class="field"><label>chargeVoltageMv</label><input id="chargeVoltageMv" type="number" min="3840" max="4608" step="16"></div>
	        <div class="field"><label>minSystemVoltageMv</label><input id="minSystemVoltageMv" type="number" min="3000" max="3700" step="100"></div>
      </div>

      <h3>Boost And Thermal</h3>
      <div class="formgrid">
	        <div class="field"><label>boostVoltageMv</label><input id="boostVoltageMv" type="number" min="4550" max="5510" step="64"></div>
        <div class="field"><label>boostFrequency</label><select id="boostFrequency"><option value="0">1500 kHz</option><option value="1">500 kHz</option></select></div>
        <div class="field"><label>boostHotThreshold</label><select id="boostHotThreshold"><option value="0">34.75 percent</option><option value="1">37.75 percent</option><option value="2">31.25 percent</option><option value="3">Disabled</option></select></div>
        <div class="field"><label>boostColdThreshold</label><select id="boostColdThreshold"><option value="0">77 percent</option><option value="1">80 percent</option></select></div>
        <div class="field"><label>thermalRegulation</label><select id="thermalRegulation"><option value="0">60 C</option><option value="1">80 C</option><option value="2">100 C</option><option value="3">120 C</option></select></div>
        <div class="field"><label>batteryLowThreshold</label><select id="batteryLowThreshold"><option value="0">2800 mV</option><option value="1">3000 mV</option></select></div>
        <div class="field"><label>rechargeThreshold</label><select id="rechargeThreshold"><option value="0">100 mV below full</option><option value="1">200 mV below full</option></select></div>
      </div>

      <h3>Safety And Compensation</h3>
      <div class="formgrid">
        <div class="field"><label>watchdog</label><select id="watchdog"><option value="0">Off</option><option value="1">40 seconds</option><option value="2">80 seconds</option><option value="3">160 seconds</option></select></div>
        <div class="field"><label>safetyTimer</label><select id="safetyTimer"><option value="0">5 hours</option><option value="1">8 hours</option><option value="2">12 hours</option><option value="3">20 hours</option></select></div>
	        <div class="field"><label>irCompResistanceMohm</label><input id="irCompResistanceMohm" type="number" min="0" max="140" step="20"></div>
	        <div class="field"><label>irCompVoltageClampMv</label><input id="irCompVoltageClampMv" type="number" min="0" max="224" step="32"></div>
      </div>

      <div class="actions">
        <button class="primary" onclick="saveCharger()">Apply BQ25895 Settings</button>
	        <button onclick="cmdExpert('force_dpdm','Force DPDM can change detected adapter type and current limit.')">Force DPDM</button>
	        <button onclick="cmdExpert('force_ico','Force ICO lets the chip search and change the input current limit.')">Force ICO</button>
	        <button onclick="cmdExpert('pumpx_up','PumpX changes adapter negotiation on compatible chargers.')">PumpX Up</button>
	        <button onclick="cmdExpert('pumpx_down','PumpX changes adapter negotiation on compatible chargers.')">PumpX Down</button>
        <button class="warn" onclick="cmdDanger('ship_mode','Enter Ship Mode? Charging can stop until wake/replug.')">Ship Mode</button>
        <button class="danger" onclick="cmdDanger('reset_bq25','Reset BQ25895 charger?')">Reset BQ25895</button>
      </div>
    </div>
  </section>

  <section id="tabBq27" class="tab">
    <div class="grid">
      <div class="card"><h2>State Of Charge</h2><div id="socMetric" class="metric">---</div><div class="muted" id="socNote">---</div></div>
      <div class="card"><h2>Battery Voltage</h2><div id="gaugeVoltageMetric" class="metric">---</div><div class="muted" id="gaugeCurrentNote">---</div></div>
      <div class="card"><h2>Temperature</h2><div id="gaugeTempMetric" class="metric">---</div><div class="muted" id="gaugeInternalTempNote">---</div></div>
      <div class="card"><h2>Health</h2><div id="sohMetric" class="metric">---</div><div class="muted">State of health</div></div>
    </div>

    <div class="grid">
      <div class="card">
        <h2>BQ27441 Readings</h2>
        <div class="row"><span>Remaining Capacity</span><b id="remainingCapacityMah">---</b></div>
        <div class="row"><span>Full Capacity</span><b id="fullChargeCapacityMah">---</b></div>
        <div class="row"><span>Average Power</span><b id="averagePowerMw">---</b></div>
        <div class="row"><span>Standby Current</span><b id="standbyCurrentMa">---</b></div>
        <div class="row"><span>Max Load Current</span><b id="maxLoadCurrentMa">---</b></div>
      </div>

      <div class="card">
        <h2>BQ27441 Flags</h2>
        <div class="row"><span>Battery Detected</span><b id="batteryDetected">---</b></div>
        <div class="row"><span>Charging</span><b id="gaugeCharging">---</b></div>
        <div class="row"><span>Discharging</span><b id="gaugeDischarging">---</b></div>
        <div class="row"><span>Full Charge</span><b id="fullCharge">---</b></div>
        <div class="row"><span>Low Battery</span><b id="lowBattery">---</b></div>
        <div class="row"><span>Final Low Battery</span><b id="finalLowBattery">---</b></div>
        <div class="row"><span>Power On Reset</span><b id="powerOnReset">---</b></div>
      </div>

      <div class="card">
        <h2>BQ27441 State</h2>
        <div class="row"><span>Sealed</span><b id="sealed">---</b></div>
        <div class="row"><span>Hibernate</span><b id="hibernate">---</b></div>
        <div class="row"><span>Sleep</span><b id="sleep">---</b></div>
        <div class="row"><span>Ready</span><b id="initComplete">---</b></div>
        <div class="row"><span>Voltage OK</span><b id="voltageOk">---</b></div>
        <div class="row"><span>Shutdown Enabled</span><b id="shutdownEnabled">---</b></div>
      </div>
    </div>

    <div class="grid">
      <div class="card">
        <h2>BQ27441 Battery Profile</h2>
        <div class="formgrid">
	          <div class="field"><label>designCapacityMah</label><input id="designCapacityMah" type="number" min="1" max="30000" step="1"></div>
	          <div class="field"><label>designEnergyMwh</label><input id="designEnergyMwh" type="number" min="1" max="60000" step="1"></div>
	          <div class="field"><label>terminateVoltageMv</label><input id="terminateVoltageMv" type="number" min="2500" max="3700" step="1"></div>
	          <div class="field"><label>taperRate</label><input id="taperRate" type="number" min="1" max="2000" step="1"></div>
	          <div class="field"><label>soc1SetPercent</label><input id="soc1SetPercent" type="number" min="0" max="100" step="1"></div>
	          <div class="field"><label>soc1ClearPercent</label><input id="soc1ClearPercent" type="number" min="0" max="100" step="1"></div>
	          <div class="field"><label>socfSetPercent</label><input id="socfSetPercent" type="number" min="0" max="100" step="1"></div>
	          <div class="field"><label>socfClearPercent</label><input id="socfClearPercent" type="number" min="0" max="100" step="1"></div>
	          <div class="field"><label>socIntDeltaPercent</label><input id="socIntDeltaPercent" type="number" min="1" max="100" step="1"></div>
        </div>
        <div class="actions">
          <button class="primary" onclick="saveBatteryProfile()">Write Battery Profile</button>
          <button onclick="cmd('read_battery_profile')">Read Battery Profile</button>
        </div>
        <div class="muted" id="lastReadBatteryProfile">No profile read yet.</div>
      </div>

      <div class="card">
        <h2>BQ27441 Options</h2>
        <div class="formgrid">
          <div class="field"><label>gpoutMode</label><select id="gpoutMode"><option value="0">Battery low</option><option value="1">SOC interrupt</option></select></div>
          <div class="field"><label>temperatureSource</label><select id="temperatureSource"><option value="0">Internal sensor</option><option value="1">Host provided</option></select></div>
        </div>
        <div class="checkgrid">
          <button id="gpoutActiveHigh" class="toggle" onclick="toggleSetting('gauge','gpoutActiveHigh')"><span>GPOUT Active High</span><b>OFF</b></button>
          <button id="batteryInsertionEnabled" class="toggle" onclick="toggleSetting('gauge','batteryInsertionEnabled')"><span>Battery Insertion Detection</span><b>OFF</b></button>
          <button id="binPullupEnabled" class="toggle" onclick="toggleSetting('gauge','binPullupEnabled')"><span>BIN Pullup</span><b>OFF</b></button>
          <button id="sleepEnabled" class="toggle" onclick="toggleSetting('gauge','sleepEnabled')"><span>Sleep Enabled</span><b>OFF</b></button>
          <button id="rmFccSmoothingEnabled" class="toggle" onclick="toggleSetting('gauge','rmFccSmoothingEnabled')"><span>Capacity Smoothing</span><b>OFF</b></button>
        </div>
        <div class="actions">
          <button class="primary" onclick="saveGaugeOptions()">Apply BQ27441 Options</button>
	          <button onclick="cmdExpert('pulse_gpout','Pulse GPOUT changes the interrupt output briefly.')">Pulse GPOUT</button>
	          <button onclick="cmdExpert('battery_insert','Battery Insert changes the gauge battery-detection state.')">Battery Insert</button>
	          <button onclick="cmdExpert('battery_remove','Battery Remove changes the gauge battery-detection state.')">Battery Remove</button>
        </div>
      </div>

      <div class="card">
        <h2>BQ27441 Reset And Power</h2>
        <div class="actions">
	          <button onclick="cmdExpert('clear_hibernate','Clear Hibernate changes the gauge power-state behavior.')">Clear Hibernate</button>
	          <button onclick="cmdExpert('set_hibernate','Set Hibernate can make the gauge enter low-power behavior.')">Set Hibernate</button>
	          <button onclick="cmdExpert('unseal_gauge','Unseal changes gauge access protection.')">Unseal</button>
	          <button onclick="cmdExpert('seal_gauge','Seal changes gauge access protection.')">Seal</button>
	          <button class="warn" onclick="cmdDanger('shutdown_enable','Enable gauge shutdown?')">Enable Shutdown</button>
	          <button class="warn" onclick="cmdDanger('shutdown_gauge','Shutdown BQ27441 gauge?')">Shutdown</button>
	          <button class="danger" onclick="cmdDanger('soft_reset_gauge','Soft reset BQ27441 gauge?')">Soft Reset</button>
	          <button class="danger" onclick="cmdDanger('reset_bq27','Reset BQ27441 gauge?')">Reset BQ27441</button>
        </div>
      </div>
    </div>
  </section>
</div>

<script>
const chargerFields = [
  "cePinEnabled","highImpedanceMode","ilimPinEnabled","inputCurrentLimitMa","inputVoltageLimitMv","vindpmOffsetMv",
  "chargeCurrentMa","prechargeCurrentMa","terminationCurrentMa","chargeVoltageMv","minSystemVoltageMv",
  "boostVoltageMv","irCompResistanceMohm","irCompVoltageClampMv","watchdog","safetyTimer","thermalRegulation",
  "boostFrequency","boostHotThreshold","boostColdThreshold","batteryLowThreshold","rechargeThreshold",
  "chargingEnabled","otgEnabled","batteryLoadEnabled","pumpXEnabled","terminationEnabled","safetyTimerEnabled",
  "safetyTimerSlowedInDpm","autoDpdmEnabled","hvdcpEnabled","maxChargeEnabled","icoEnabled","adcContinuous",
  "absoluteVindpm","batfetResetEnabled"
];
const batteryProfileFields = ["designCapacityMah","designEnergyMwh","terminateVoltageMv","taperRate","soc1SetPercent","soc1ClearPercent","socfSetPercent","socfClearPercent","socIntDeltaPercent"];
const gaugeOptionFields = ["gpoutMode","temperatureSource","gpoutActiveHigh","batteryInsertionEnabled","binPullupEnabled","sleepEnabled","rmFccSmoothingEnabled"];
let lastStatus = null;
let expertUnlocked = localStorage.getItem("batthubExpertUnlocked") === "1";
const riskyOn = {
  highImpedanceMode: "High Impedance can stop input power and charging.",
  ilimPinEnabled: "ILIM pin can limit input current depending on hardware resistor.",
  otgEnabled: "OTG Boost uses the battery as power source instead of charging it.",
  batteryLoadEnabled: "Battery Load Test briefly loads the battery.",
  pumpXEnabled: "PumpX changes adapter voltage negotiation.",
  autoDpdmEnabled: "Auto DPDM can change detected adapter type and current limit.",
  hvdcpEnabled: "HVDCP can request higher adapter voltage on supported chargers.",
  maxChargeEnabled: "MaxCharge can change adapter detection behavior.",
  icoEnabled: "ICO can override the configured input current limit."
};
const riskyOff = {
  cePinEnabled: "CE off can block charging at the hardware pin.",
  chargingEnabled: "Charging off disables the charger.",
  terminationEnabled: "Termination off can prevent normal end-of-charge detection.",
  safetyTimerEnabled: "Safety Timer off removes the charge-time safety limit.",
  adcContinuous: "ADC off can make live measurements look stale.",
  absoluteVindpm: "Absolute VINDPM off can make the input voltage limit dynamic and harder to understand.",
  batfetResetEnabled: "BATFET Reset disabled changes recovery behavior after BATFET events."
};
const numericRisk = {
  inputCurrentLimitMa: {limit: 1500, text: "Input current above 1500 mA must match adapter, cable, connector, and PCB thermals."},
  chargeCurrentMa: {limit: 1000, text: "Charge current above 1000 mA must match battery rating and board thermals."},
  chargeVoltageMv: {limit: 4208, text: "Charge voltage above about 4.2 V is only for batteries that explicitly allow it."},
  thermalRegulation: {limit: 2, text: "Thermal limit above 100 C allows the charger to get hotter before reducing current."},
  watchdog: {limit: 0, text: "Watchdog on requires regular host service or the BQ25895 can reset settings."}
};

function showTab(tab) {
  document.getElementById("tabBq25").classList.toggle("active", tab === "bq25");
  document.getElementById("tabBq27").classList.toggle("active", tab === "bq27");
  document.getElementById("tabBtnBq25").classList.toggle("active", tab === "bq25");
  document.getElementById("tabBtnBq27").classList.toggle("active", tab === "bq27");
}
function updateExpertLockUi() {
  const button = document.getElementById("expertLockButton");
  if (!button) return;
  button.textContent = expertUnlocked ? "Expert Unlocked" : "Expert Locked";
  button.classList.toggle("unlocked", expertUnlocked);
  for (const id of Object.keys(riskyOn).concat(Object.keys(riskyOff), Object.keys(numericRisk))) {
    const el = document.getElementById(id);
    if (el) el.classList.toggle("riskmark", !expertUnlocked);
  }
}
function toggleExpertLock() {
  if (!expertUnlocked && !confirm("Unlock expert actions? Risky charger and gauge controls can stop charging, change adapter negotiation, or reset chip state.")) {
    return;
  }
  expertUnlocked = !expertUnlocked;
  localStorage.setItem("batthubExpertUnlocked", expertUnlocked ? "1" : "0");
  updateExpertLockUi();
}
function confirmRisk(message) {
  if (expertUnlocked) return true;
  return confirm(message + "\n\nExpert Locked is active. Continue only if this is intentional.");
}
function currentChargerValue(id) {
  return lastStatus && lastStatus.settings && lastStatus.settings.charger ? lastStatus.settings.charger[id] : undefined;
}
function riskMessageForChargerChange(id, next) {
  if (next === true && riskyOn[id]) return riskyOn[id];
  if (next === false && riskyOff[id]) return riskyOff[id];
  if (numericRisk[id]) {
    const value = Number(next);
    if (!Number.isNaN(value) && value > numericRisk[id].limit) return numericRisk[id].text;
  }
  return "";
}
function confirmRiskyToggle(group, id, next) {
  if (group !== "charger") return true;
  const message = riskMessageForChargerChange(id, next);
  return !message || confirmRisk(id + ": " + message);
}
function normalizeNumberInput(el) {
  if (!el || el.type !== "number") return;
  let value = Number(el.value);
  if (Number.isNaN(value)) return;
  if (el.min !== "") value = Math.max(value, Number(el.min));
  if (el.max !== "") value = Math.min(value, Number(el.max));
  el.value = String(value);
}
function confirmRiskyChargerParams(params, changedId) {
  const ids = changedId ? [changedId] : chargerFields;
  for (const id of ids) {
    if (!params.has(id)) continue;
    const previous = currentChargerValue(id);
    let next = params.get(id);
    if (next === "1" || next === "0") next = next === "1";
    if (String(previous) === String(next)) continue;
    const message = riskMessageForChargerChange(id, next);
    if (message && !confirmRisk(id + ": " + message)) return false;
  }
  return true;
}
function setText(id, text, cls="") {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = text;
  el.className = cls;
}
function badge(value, goodText="Yes") {
  const good = value === true || value === goodText || value === "normal" || value === "idle";
  return '<span class="badge ' + (good ? "ok" : "warn") + '">' + (value === true ? "Yes" : value === false ? "No" : value) + '</span>';
}
function setBadge(id, value, goodText="Yes") {
  const el = document.getElementById(id);
  if (el) el.innerHTML = badge(value, goodText);
}
function setInput(id, value) {
  const el = document.getElementById(id);
  if (!el || document.activeElement === el) return;
  if (el.classList.contains("toggle")) setToggleVisual(el, !!value);
  else if (el.type === "checkbox") el.checked = !!value;
  else el.value = value;
}
function setToggleVisual(el, on) {
  el.classList.toggle("on", !!on);
  el.classList.remove("pending");
  const badge = el.querySelector("b");
  if (badge) badge.textContent = on ? "ON" : "OFF";
}
function tempText(x10) {
  return (x10 / 10).toFixed(1) + " C";
}
function percentX100Text(x100) {
  return (x100 / 100).toFixed(1) + " percent";
}
function collectParams(fields) {
  const params = new URLSearchParams();
  for (const id of fields) {
    const el = document.getElementById(id);
    if (!el) continue;
    normalizeNumberInput(el);
    if (el.classList.contains("toggle")) params.set(id, el.classList.contains("on") ? "1" : "0");
    else params.set(id, el.type === "checkbox" ? (el.checked ? "1" : "0") : el.value);
  }
  return params;
}
async function cmd(x) {
  const response = await fetch("/cmd?x=" + encodeURIComponent(x));
  if (!response.ok) alert("Command failed: " + x);
  update();
}
async function cmdExpert(x, message) {
  if (!confirmRisk(message)) return;
  await cmd(x);
}
async function cmdDanger(x, message) {
  if (!confirm(message)) return;
  await cmd(x);
}
async function toggleSetting(group, id) {
  const el = document.getElementById(id);
  if (!el) return;
  const next = !el.classList.contains("on");
  if (!confirmRiskyToggle(group, id, next)) return;
  setToggleVisual(el, next);
  el.classList.add("pending");
  const response = await fetch("/cmd?x=toggle&group=" + encodeURIComponent(group) + "&name=" + encodeURIComponent(id) + "&value=" + (next ? "1" : "0"));
  if (!response.ok) {
    setToggleVisual(el, !next);
  }
  update();
}
async function saveCharger(changedId) {
  const params = collectParams(chargerFields);
  if (!confirmRiskyChargerParams(params, changedId)) {
    update();
    return;
  }
  const response = await fetch("/cmd?x=set_charger&" + params.toString());
  if (!response.ok) alert("BQ25895 settings failed");
  update();
}
async function saveBatteryProfile() {
  if (!confirmRisk("Writing the battery profile changes the BQ27441 capacity model. Wrong values make percent and capacity wrong.")) return;
  const params = collectParams(batteryProfileFields);
  const response = await fetch("/cmd?x=set_battery_profile&" + params.toString());
  if (!response.ok) alert("BQ27441 battery profile write failed");
  update();
}
async function saveGaugeOptions() {
  const params = collectParams(gaugeOptionFields);
  const response = await fetch("/cmd?x=set_gauge_options&" + params.toString());
  if (!response.ok) alert("BQ27441 options failed");
  update();
}
function fillForms(j) {
  const c = j.settings.charger;
  for (const id of chargerFields) {
    setInput(id, c[id]);
  }
  const p = j.settings.batteryProfile;
  for (const id of batteryProfileFields) {
    setInput(id, p[id]);
  }
  const g = j.settings.gauge;
  for (const id of gaugeOptionFields) {
    if (id in g) setInput(id, g[id]);
  }
}
function chargeBlockerText(j) {
  const c = j.settings.charger;
  const live = j.bq25.live;
  const faults = j.bq25.faults;
  const blockers = [];

  if (!c.cePinEnabled) blockers.push("CE pin off");
  if (!c.chargingEnabled) blockers.push("charging off");
  if (c.highImpedanceMode) blockers.push("high impedance on");
  if (c.otgEnabled) blockers.push("OTG on");
  if (!live.powerGood) blockers.push("no power good");
  if (faults.charge !== "normal") blockers.push("charge fault: " + faults.charge);
  if (faults.ntc !== "normal") blockers.push("NTC: " + faults.ntc);
  if (faults.watchdog) blockers.push("watchdog fault");
  if (faults.battery) blockers.push("battery fault");
  if (live.inputVoltageLimited) blockers.push("input voltage limited");
  if (live.inputCurrentLimited) blockers.push("input current limited");
  if (live.effectiveInputCurrentLimitMa <= 500 && c.chargeCurrentMa > 500) blockers.push("input limit only 500mA");
  if (c.chargeCurrentMa > c.inputCurrentLimitMa && live.chargeCurrentMa === 0) blockers.push("charge current set too high");

  return blockers.length ? blockers.join(", ") : "Ready";
}
function setDiagnosis(kind, title, detail) {
  const el = document.getElementById("chargeDiagnosis");
  if (!el) return;
  el.className = "notice " + kind;
  el.innerHTML = "<strong>" + title + "</strong><span>" + detail + "</span>";
}
function updateChargeDiagnosis(j) {
  const c = j.settings.charger;
  const bq = j.bq25;
  if (!bq.present) {
    setDiagnosis("bad", "BQ25895 not found", "The charger chip does not answer on I2C. Check SDA, SCL, address 0x6A, and power.");
    return;
  }
  const live = bq.live;
  const faults = bq.faults;
  const actual = bq.actual;
  const gauge = j.bq27 && j.bq27.present ? j.bq27 : null;
  const gaugeCurrent = gauge ? gauge.live.averageCurrentMa : 0;

  if (!c.cePinEnabled) {
    setDiagnosis("bad", "Charging blocked by CE", "CE Pin Enabled is OFF. Turn it ON or press Safe Charge.");
  } else if (!c.chargingEnabled) {
    setDiagnosis("bad", "Charging disabled in software", "Charging Enabled is OFF. Turn it ON or press Safe Charge.");
  } else if (c.highImpedanceMode) {
    setDiagnosis("bad", "Input is in high impedance", "High Impedance reduces or disconnects the input path. Turn it OFF for normal charging.");
  } else if (c.otgEnabled) {
    setDiagnosis("bad", "OTG boost is active", "OTG uses the battery to power VBUS. Turn OTG Boost OFF for normal charging.");
  } else if (!live.powerGood) {
    setDiagnosis("bad", "No valid USB input", "Power Good is No. Check USB cable, adapter, VBUS wiring, and GND.");
  } else if (faults.charge !== "normal") {
    setDiagnosis("bad", "Charge fault: " + faults.charge, "Reset only after fixing the cause. Input, thermal, and safety-timer faults can stop charging.");
  } else if (faults.ntc !== "normal") {
    setDiagnosis("bad", "NTC temperature fault", "TS/NTC is outside the allowed range. Check the battery NTC wiring or TS resistor network.");
  } else if (faults.watchdog) {
    setDiagnosis("bad", "Watchdog fault", "The BQ25895 watchdog expired and may have reset settings. Set watchdog Off or keep the host running.");
  } else if (faults.battery) {
    setDiagnosis("bad", "Battery fault", "The charger reports a battery fault. Check battery connection and cell voltage.");
  } else if (live.inputVoltageLimited) {
    if (actual.absoluteVindpm && live.inputVoltageMv > actual.inputVoltageLimitMv + 200) {
      setDiagnosis("warn", "VINDPM flag looks inconsistent", "VBUS is above the actual VINDPM limit. It may be a short cable dip or stale status. Press Safe Charge; if it repeats, check wiring and raw status.");
    } else {
      setDiagnosis("warn", "Input voltage limit is active", "The BQ25895 is reducing current to keep VBUS above the VINDPM limit. Lower charge current or improve cable/adapter path.");
    }
  } else if (live.inputCurrentLimited) {
    setDiagnosis("warn", "Input current limit is active", "The charger reached the configured input current limit. Raise it only if adapter, cable, connector, and PCB can handle it.");
  } else if (actual.inputCurrentLimitMa <= 500 && c.chargeCurrentMa > 500) {
    setDiagnosis("warn", "Input limit is only 500 mA", "The charger may be adapter-limited. Disable Auto DPDM/ICO for fixed control or use Force DPDM intentionally.");
  } else if (live.thermalRegulation) {
    setDiagnosis("warn", "Thermal regulation is active", "The charger is reducing current because it is warm. Lower charge current and verify PCB temperature.");
  } else if (live.chargeState === "precharge") {
    if (gauge && (gauge.flags.charging || gaugeCurrent > 0)) {
      setDiagnosis("ok", "Precharge is working", "Battery voltage is low, so the charger intentionally uses small precharge current. BQ27441 sees current into the battery.");
    } else {
      setDiagnosis("warn", "Precharge, but current is unclear", "Battery is low. If current stays zero, check battery wiring, NTC, and whether the cell protection is open.");
    }
  } else if (live.chargeState === "fast charge") {
    if (live.chargeCurrentMa > 0 || gaugeCurrent > 0) {
      setDiagnosis("ok", "Fast charge is active", "Input, charger, and battery path look usable. Watch temperature and charge current on real hardware.");
    } else {
      setDiagnosis("warn", "Fast charge state, but measured current is zero", "Check BQ27441 current too. If both stay zero, the battery path or current measurement needs checking.");
    }
  } else if (live.chargeState === "done") {
    setDiagnosis("ok", "Charge complete", "The charger thinks the battery is full or below termination current.");
  } else if (gauge && (gauge.flags.charging || gaugeCurrent > 0)) {
    setDiagnosis("ok", "Battery current is flowing", "BQ27441 sees current into the battery even if the charger ADC is low or slow to update.");
  } else {
    setDiagnosis("warn", "No obvious blocker, but not charging", "Use Safe Charge, then check actual settings, battery voltage, NTC, and BQ27441 average current.");
  }
}
function updateBq25(j) {
  const bq = j.bq25;
  if (!bq.present) {
    setText("chargeStateMetric", "Not found", "metric bad");
    return;
  }
  const live = bq.live;
  const faults = bq.faults;
  const actual = bq.actual;
  let chargeCls = "metric warn";
  if (live.chargeState === "fast charge") chargeCls = "metric ok";
  if (live.chargeState === "done") chargeCls = "metric blue";
  if (faults.charge !== "normal") chargeCls = "metric bad";
  setText("chargeStateMetric", live.chargeState, chargeCls);
  setText("chargeStateNote", "Charge fault: " + faults.charge);
  setText("inputVoltageMetric", live.inputVoltageMv + " mV", live.inputVoltageMv < 4400 ? "metric bad" : "metric ok");
  setText("inputTypeNote", live.inputType);
  setText("batteryVoltageMetric", live.batteryVoltageMv + " mV", "metric ok");
  setText("systemVoltageNote", "System: " + live.systemVoltageMv + " mV");
  setText("chargeCurrentMetric", live.chargeCurrentMa + " mA", "metric blue");
  const blockers = chargeBlockerText(j);
  updateChargeDiagnosis(j);
  setText("chargeBlockers", blockers, blockers === "Ready" ? "ok" : "bad");
  setBadge("powerGood", live.powerGood);
  setText("effectiveInputCurrentLimitMa", live.effectiveInputCurrentLimitMa + " mA");
  setBadge("inputCurrentLimited", live.inputCurrentLimited ? "Yes" : "No", "No");
  setBadge("inputVoltageLimited", live.inputVoltageLimited ? "Yes" : "No", "No");
  setBadge("thermalRegulationStatus", live.thermalRegulation ? "Yes" : "No", "No");
  setText("tsPercent", percentX100Text(live.tsPercentX100));
  setText("chargeFault", faults.charge, faults.charge === "normal" ? "ok" : "bad");
  setText("ntcFault", faults.ntc, faults.ntc === "normal" ? "ok" : "bad");
  setBadge("watchdogFault", faults.watchdog ? "Yes" : "No", "No");
  setBadge("boostFault", faults.boost ? "Yes" : "No", "No");
  setBadge("batteryFault", faults.battery ? "Yes" : "No", "No");
  setText("actualChargeCurrentMa", actual.chargeCurrentMa + " mA");
  setText("actualChargeVoltageMv", actual.chargeVoltageMv + " mV");
  setText("actualInputCurrentLimitMa", actual.inputCurrentLimitMa + " mA");
  setText("actualInputVoltageLimitMv", actual.inputVoltageLimitMv + " mV");
  setBadge("actualAbsoluteVindpm", actual.absoluteVindpm);
  setText("actualVindpmOffsetMv", actual.vindpmOffsetMv + " mV");
  setText("actualWatchdog", actual.watchdog);
  setText("actualSafetyTimer", actual.safetyTimer);
  setText("actualThermalRegulation", actual.thermalRegulation);
}
function updateBq27(j) {
  const fg = j.bq27;
  if (!fg.present) {
    setText("socMetric", "Not found", "metric bad");
    return;
  }
  const live = fg.live;
  const flags = fg.flags;
  const control = fg.control;
  setText("socMetric", live.stateOfChargePercent + " %", "metric ok");
  setText("socNote", "Unfiltered: " + live.stateOfChargeUnfilteredPercent + " %");
  setText("gaugeVoltageMetric", live.voltageMv + " mV", "metric ok");
  setText("gaugeCurrentNote", "Average current: " + live.averageCurrentMa + " mA");
  setText("gaugeTempMetric", tempText(live.temperatureCelsiusX10), "metric blue");
  setText("gaugeInternalTempNote", "Internal: " + tempText(live.internalTemperatureCelsiusX10));
  setText("sohMetric", live.stateOfHealthPercent + " %", "metric ok");
  setText("remainingCapacityMah", live.remainingCapacityMah + " mAh");
  setText("fullChargeCapacityMah", live.fullChargeCapacityMah + " mAh");
  setText("averagePowerMw", live.averagePowerMw + " mW");
  setText("standbyCurrentMa", live.standbyCurrentMa + " mA");
  setText("maxLoadCurrentMa", live.maxLoadCurrentMa + " mA");
  setBadge("batteryDetected", flags.batteryDetected);
  setBadge("gaugeCharging", flags.charging);
  setBadge("gaugeDischarging", flags.discharging);
  setBadge("fullCharge", flags.fullCharge);
  setBadge("lowBattery", flags.lowBattery ? "Yes" : "No", "No");
  setBadge("finalLowBattery", flags.finalLowBattery ? "Yes" : "No", "No");
  setBadge("powerOnReset", flags.powerOnReset ? "Yes" : "No", "No");
  setBadge("sealed", control.sealed ? "Yes" : "No", "No");
  setBadge("hibernate", control.hibernate ? "Yes" : "No", "No");
  setBadge("sleep", control.sleep ? "Yes" : "No", "No");
  setBadge("initComplete", control.initComplete);
  setBadge("voltageOk", control.voltageOk);
  setBadge("shutdownEnabled", control.shutdownEnabled ? "Yes" : "No", "No");
  if (fg.lastReadBatteryProfileValid) {
    const p = fg.lastReadBatteryProfile;
    document.getElementById("lastReadBatteryProfile").textContent =
      "Read from gauge: " + p.designCapacityMah + " mAh, " + p.designEnergyMwh + " mWh, terminate " + p.terminateVoltageMv + " mV";
  }
}
async function update() {
  try {
    const response = await fetch("/api/status");
    const j = await response.json();
    lastStatus = j;
    document.getElementById("topline").textContent = j.wifiMode + " | IP " + j.ip;
    fillForms(j);
    updateBq25(j);
    updateBq27(j);
  } catch (error) {
    document.getElementById("topline").textContent = "Connection error";
  }
}
function bindLiveChargerFields() {
  for (const id of chargerFields) {
    const el = document.getElementById(id);
    if (!el || el.classList.contains("toggle")) continue;
    el.addEventListener("change", () => saveCharger(id));
  }
}
bindLiveChargerFields();
updateExpertLockUi();
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

void updateChargerSettingsFromRequest() {
  settings.cePinEnabled = argBool("cePinEnabled", settings.cePinEnabled);
  settings.charger.highImpedanceMode = argBool("highImpedanceMode", settings.charger.highImpedanceMode);
  settings.charger.ilimPinEnabled = argBool("ilimPinEnabled", settings.charger.ilimPinEnabled);
  settings.charger.inputCurrentLimitMa = argU16("inputCurrentLimitMa", settings.charger.inputCurrentLimitMa, 100, 3250);
  settings.charger.inputVoltageLimitMv = argU16("inputVoltageLimitMv", settings.charger.inputVoltageLimitMv, 2600, 15300);
  settings.charger.vindpmOffsetMv = argU16("vindpmOffsetMv", settings.charger.vindpmOffsetMv, 0, 3100);
  settings.charger.chargeCurrentMa = argU16("chargeCurrentMa", settings.charger.chargeCurrentMa, 0, 5056);
  settings.charger.prechargeCurrentMa = argU16("prechargeCurrentMa", settings.charger.prechargeCurrentMa, 64, 1024);
  settings.charger.terminationCurrentMa = argU16("terminationCurrentMa", settings.charger.terminationCurrentMa, 64, 1024);
  settings.charger.chargeVoltageMv = argU16("chargeVoltageMv", settings.charger.chargeVoltageMv, 3840, 4608);
  settings.charger.minSystemVoltageMv = argU16("minSystemVoltageMv", settings.charger.minSystemVoltageMv, 3000, 3700);
  settings.charger.boostVoltageMv = argU16("boostVoltageMv", settings.charger.boostVoltageMv, 4550, 5510);
  settings.charger.irCompResistanceMohm = argU16("irCompResistanceMohm", settings.charger.irCompResistanceMohm, 0, 140);
  settings.charger.irCompVoltageClampMv = argU16("irCompVoltageClampMv", settings.charger.irCompVoltageClampMv, 0, 224);
  settings.charger.watchdog = static_cast<bq25::WatchdogTimer>(argInt("watchdog", settings.charger.watchdog, 0, 3));
  settings.charger.safetyTimer = static_cast<bq25::SafetyTimer>(argInt("safetyTimer", settings.charger.safetyTimer, 0, 3));
  settings.charger.thermalRegulation = static_cast<bq25::ThermalRegulation>(argInt("thermalRegulation", settings.charger.thermalRegulation, 0, 3));
  settings.charger.boostFrequency = static_cast<bq25::BoostFrequency>(argInt("boostFrequency", settings.charger.boostFrequency, 0, 1));
  settings.charger.boostHotThreshold = static_cast<bq25::BoostHotThreshold>(argInt("boostHotThreshold", settings.charger.boostHotThreshold, 0, 3));
  settings.charger.boostColdThreshold = static_cast<bq25::BoostColdThreshold>(argInt("boostColdThreshold", settings.charger.boostColdThreshold, 0, 1));
  settings.charger.batteryLowThreshold = static_cast<bq25::BatteryLowThreshold>(argInt("batteryLowThreshold", settings.charger.batteryLowThreshold, 0, 1));
  settings.charger.rechargeThreshold = static_cast<bq25::RechargeThreshold>(argInt("rechargeThreshold", settings.charger.rechargeThreshold, 0, 1));
  settings.charger.chargingEnabled = argBool("chargingEnabled", settings.charger.chargingEnabled);
  settings.charger.otgEnabled = argBool("otgEnabled", settings.charger.otgEnabled);
  settings.charger.batteryLoadEnabled = argBool("batteryLoadEnabled", settings.charger.batteryLoadEnabled);
  settings.charger.pumpXEnabled = argBool("pumpXEnabled", settings.charger.pumpXEnabled);
  settings.charger.terminationEnabled = argBool("terminationEnabled", settings.charger.terminationEnabled);
  settings.charger.safetyTimerEnabled = argBool("safetyTimerEnabled", settings.charger.safetyTimerEnabled);
  settings.charger.safetyTimerSlowedInDpm = argBool("safetyTimerSlowedInDpm", settings.charger.safetyTimerSlowedInDpm);
  settings.charger.autoDpdmEnabled = argBool("autoDpdmEnabled", settings.charger.autoDpdmEnabled);
  settings.charger.hvdcpEnabled = argBool("hvdcpEnabled", settings.charger.hvdcpEnabled);
  settings.charger.maxChargeEnabled = argBool("maxChargeEnabled", settings.charger.maxChargeEnabled);
  settings.charger.icoEnabled = argBool("icoEnabled", settings.charger.icoEnabled);
  settings.charger.adcContinuous = argBool("adcContinuous", settings.charger.adcContinuous);
  settings.charger.absoluteVindpm = argBool("absoluteVindpm", settings.charger.absoluteVindpm);
  settings.charger.batfetResetEnabled = argBool("batfetResetEnabled", settings.charger.batfetResetEnabled);
}

void updateBatteryProfileFromRequest() {
  settings.batteryProfile.designCapacityMah = argU16("designCapacityMah", settings.batteryProfile.designCapacityMah, 1, 30000);
  settings.batteryProfile.designEnergyMwh = argU16("designEnergyMwh", settings.batteryProfile.designEnergyMwh, 1, 60000);
  settings.batteryProfile.terminateVoltageMv = argU16("terminateVoltageMv", settings.batteryProfile.terminateVoltageMv, 2500, 3700);
  settings.batteryProfile.taperRate = argU16("taperRate", settings.batteryProfile.taperRate, 1, 2000);
  settings.batteryProfile.soc1SetPercent = argU8("soc1SetPercent", settings.batteryProfile.soc1SetPercent, 0, 100);
  settings.batteryProfile.soc1ClearPercent = argU8("soc1ClearPercent", settings.batteryProfile.soc1ClearPercent, 0, 100);
  settings.batteryProfile.socfSetPercent = argU8("socfSetPercent", settings.batteryProfile.socfSetPercent, 0, 100);
  settings.batteryProfile.socfClearPercent = argU8("socfClearPercent", settings.batteryProfile.socfClearPercent, 0, 100);
  settings.batteryProfile.socIntDeltaPercent = argU8("socIntDeltaPercent", settings.batteryProfile.socIntDeltaPercent, 1, 100);
  settings.gauge.batteryProfile = settings.batteryProfile;
}

void updateGaugeOptionsFromRequest() {
  settings.gauge.gpoutMode = static_cast<bq27::GpoutMode>(argInt("gpoutMode", settings.gauge.gpoutMode, 0, 1));
  settings.gauge.temperatureSource = static_cast<bq27::TemperatureSource>(argInt("temperatureSource", settings.gauge.temperatureSource, 0, 1));
  settings.gauge.gpoutActiveHigh = argBool("gpoutActiveHigh", settings.gauge.gpoutActiveHigh);
  settings.gauge.batteryInsertionEnabled = argBool("batteryInsertionEnabled", settings.gauge.batteryInsertionEnabled);
  settings.gauge.binPullupEnabled = argBool("binPullupEnabled", settings.gauge.binPullupEnabled);
  settings.gauge.sleepEnabled = argBool("sleepEnabled", settings.gauge.sleepEnabled);
  settings.gauge.rmFccSmoothingEnabled = argBool("rmFccSmoothingEnabled", settings.gauge.rmFccSmoothingEnabled);
}

bool setChargerToggle(const String &name, bool value) {
  if (name == "cePinEnabled") settings.cePinEnabled = value;
  else if (name == "highImpedanceMode") settings.charger.highImpedanceMode = value;
  else if (name == "ilimPinEnabled") settings.charger.ilimPinEnabled = value;
  else if (name == "chargingEnabled") settings.charger.chargingEnabled = value;
  else if (name == "otgEnabled") settings.charger.otgEnabled = value;
  else if (name == "batteryLoadEnabled") settings.charger.batteryLoadEnabled = value;
  else if (name == "pumpXEnabled") settings.charger.pumpXEnabled = value;
  else if (name == "terminationEnabled") settings.charger.terminationEnabled = value;
  else if (name == "safetyTimerEnabled") settings.charger.safetyTimerEnabled = value;
  else if (name == "safetyTimerSlowedInDpm") settings.charger.safetyTimerSlowedInDpm = value;
  else if (name == "autoDpdmEnabled") settings.charger.autoDpdmEnabled = value;
  else if (name == "hvdcpEnabled") settings.charger.hvdcpEnabled = value;
  else if (name == "maxChargeEnabled") settings.charger.maxChargeEnabled = value;
  else if (name == "icoEnabled") settings.charger.icoEnabled = value;
  else if (name == "adcContinuous") settings.charger.adcContinuous = value;
  else if (name == "absoluteVindpm") settings.charger.absoluteVindpm = value;
  else if (name == "batfetResetEnabled") settings.charger.batfetResetEnabled = value;
  else return false;
  return applyChargerSettingToHardware(name, true);
}

bool setGaugeToggle(const String &name, bool value) {
  if (name == "gpoutActiveHigh") settings.gauge.gpoutActiveHigh = value;
  else if (name == "batteryInsertionEnabled") settings.gauge.batteryInsertionEnabled = value;
  else if (name == "binPullupEnabled") settings.gauge.binPullupEnabled = value;
  else if (name == "sleepEnabled") settings.gauge.sleepEnabled = value;
  else if (name == "rmFccSmoothingEnabled") settings.gauge.rmFccSmoothingEnabled = value;
  else return false;
  return applyGaugeOptionsToHardware(true);
}

void handleCmd() {
  String x = server.arg("x");
  bool ok = true;

  if (x == "set_charger") {
    bool oldCePinEnabled = settings.cePinEnabled;
    Batthub::ChargerSettings oldCharger = settings.charger;
    updateChargerSettingsFromRequest();
    ok = applyChangedChargerSettingsToHardware(oldCePinEnabled, oldCharger, true);
  } else if (x == "toggle") {
    String group = server.arg("group");
    String name = server.arg("name");
    bool value = argBool("value", false);
    if (group == "charger") ok = setChargerToggle(name, value);
    else if (group == "gauge") ok = setGaugeToggle(name, value);
    else ok = false;
  } else if (x == "set_battery_profile") {
    updateBatteryProfileFromRequest();
    ok = applyGaugeProfileToHardware(true);
  } else if (x == "set_gauge_options") {
    updateGaugeOptionsFromRequest();
    ok = applyGaugeOptionsToHardware(true);
  } else if (x == "read_battery_profile") {
    ok = power.readBatteryProfile(lastReadBatteryProfile);
    lastReadBatteryProfileValid = ok;
    if (ok) {
      settings.batteryProfile = lastReadBatteryProfile;
      settings.gauge.batteryProfile = settings.batteryProfile;
      saveGaugeSettings();
    }
  } else if (x == "force_dpdm") {
    ok = power.forceDpdmDetection();
  } else if (x == "force_ico") {
    ok = power.forceIco();
  } else if (x == "pumpx_up") {
    ok = power.charger.pumpXIncrease();
  } else if (x == "pumpx_down") {
    ok = power.charger.pumpXDecrease();
  } else if (x == "ship_mode") {
    ok = power.enterShipMode();
  } else if (x == "reset_bq25") {
    ok = power.resetCharger();
    delay(100);
    ok &= applyChargerSettingsToHardware(false);
  } else if (x == "reset_bq27") {
    ok = power.resetGauge();
  } else if (x == "reset_both") {
    ok = power.resetBoth();
    delay(200);
    ok &= applyChargerSettingsToHardware(false);
  } else if (x == "soft_reset_gauge") {
    ok = power.gauge.softReset();
  } else if (x == "pulse_gpout") {
    ok = power.gauge.pulseGpout();
  } else if (x == "battery_insert") {
    ok = power.gauge.batteryInsert();
  } else if (x == "battery_remove") {
    ok = power.gauge.batteryRemove();
  } else if (x == "set_hibernate") {
    ok = power.gauge.setHibernate();
  } else if (x == "clear_hibernate") {
    ok = power.gauge.clearHibernate();
  } else if (x == "seal_gauge") {
    ok = power.gauge.seal();
  } else if (x == "unseal_gauge") {
    ok = power.gauge.unseal();
  } else if (x == "shutdown_enable") {
    ok = power.gauge.shutdownEnable();
  } else if (x == "shutdown_gauge") {
    ok = power.gauge.shutdown();
  } else if (x == "safe") {
    ok = applySafeDefaults(true);
  } else if (x == "safe_charge") {
    ok = applySafeChargeMode(true);
  }

  server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAILED");
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

  loadSettings();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Batthub::BeginOptions options;
  options.startWire = false;
  options.pins.ce = CE_PIN;
  options.pins.otg = OTG_PIN;
  options.pins.intPin = INT_PIN;
  options.initialCeEnabled = settings.cePinEnabled;
  options.initialOtgEnabled = settings.charger.otgEnabled;
  power.begin(Wire, options);
  applyChargerSettingsToHardware(false);

  setupWifi();

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/cmd", handleCmd);
  server.begin();

  Serial.println("Batthub Web UI started.");
}

void loop() {
  server.handleClient();
  serviceChargerWatchdog();
  serviceChargerAfterVbusChange();
}
