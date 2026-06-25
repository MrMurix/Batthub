#include "bq25_bq27.h"

bq25bq27::bq25bq27()
  : _chargerOk(false), _gaugeOk(false),
    _lastBeginReport{false, false, 0, 0, 0, 0, 0} {
}

bool bq25bq27::begin(TwoWire &wire) {
  BeginOptions options;
  return begin(wire, options);
}

bool bq25bq27::begin(TwoWire &wire, const BeginOptions &options) {
  return beginReport(wire, options).ok();
}

bq25bq27::BeginReport bq25bq27::beginReport(TwoWire &wire) {
  BeginOptions options;
  return beginReport(wire, options);
}

bq25bq27::BeginReport bq25bq27::beginReport(TwoWire &wire, const BeginOptions &options) {
  if (options.startWire) {
    wire.begin();
  }
  charger.begin(wire, options.bq25Address, false);
  bq25::DeviceInfo chargerInfo = charger.deviceInfo();
  _chargerOk = charger.lastError() == 0 && chargerInfo.isBQ25895();

  gauge.begin(wire, options.bq27Address, false);
  uint16_t gaugeType = gauge.deviceType();
  _gaugeOk = gauge.lastError() == 0 && gaugeType == bq27::kDeviceId;

  _lastBeginReport.chargerPresent = _chargerOk;
  _lastBeginReport.gaugePresent = _gaugeOk;
  _lastBeginReport.chargerI2cError = charger.lastError();
  _lastBeginReport.gaugeI2cError = gauge.lastError();
  _lastBeginReport.chargerPartNumber = chargerInfo.partNumber;
  _lastBeginReport.chargerRevision = chargerInfo.revision;
  _lastBeginReport.gaugeDeviceType = gaugeType;
  return _lastBeginReport;
}

bool bq25bq27::isConnected() {
  _chargerOk = charger.isConnected();
  _gaugeOk = gauge.isConnected();
  return _chargerOk && _gaugeOk;
}

bq25bq27::BeginReport bq25bq27::lastBeginReport() const {
  return _lastBeginReport;
}

bool bq25bq27::applyConfig(const ModuleConfig &config) {
  bool ok = charger.applyConfig(config.charger);
  ok &= gauge.configureBattery(config.gauge);
  return ok;
}

bool bq25bq27::configure1s(uint16_t capacityMah,
                           uint16_t chargeCurrentMa,
                           uint16_t inputCurrentLimitMa,
                           uint16_t chargeVoltageMv,
                           uint16_t terminateVoltageMv) {
  ModuleConfig config;
  config.charger.chargeCurrentMa = chargeCurrentMa;
  config.charger.inputCurrentLimitMa = inputCurrentLimitMa;
  config.charger.chargeVoltageMv = chargeVoltageMv;
  uint16_t tenthChargeCurrent = chargeCurrentMa / 10;
  if (tenthChargeCurrent < 64) {
    tenthChargeCurrent = 64;
  }
  config.charger.terminationCurrentMa = tenthChargeCurrent;
  config.charger.prechargeCurrentMa = tenthChargeCurrent;
  config.charger.enableCharging = true;
  config.charger.enableTermination = true;
  config.charger.watchdog = bq25::WatchdogOff;

  config.gauge.designCapacityMah = capacityMah;
  config.gauge.designEnergyMwh = static_cast<uint16_t>((static_cast<uint32_t>(capacityMah) * 37UL) / 10UL);
  config.gauge.terminateVoltageMv = terminateVoltageMv;
  uint16_t taperDenominator = config.charger.terminationCurrentMa / 10;
  if (taperDenominator == 0) {
    taperDenominator = 1;
  }
  config.gauge.taperRate = capacityMah / taperDenominator;
  if (config.gauge.taperRate == 0) {
    config.gauge.taperRate = 1;
  }

  return applyConfig(config);
}

bq25bq27::Snapshot bq25bq27::snapshot() {
  Snapshot out;
  out.chargerStatus = charger.status();
  out.chargerFaults = charger.faults();
  out.chargerMeasurements = charger.measurements();
  out.chargerInfo = charger.deviceInfo();
  out.gauge = gauge.snapshot();
  return out;
}
