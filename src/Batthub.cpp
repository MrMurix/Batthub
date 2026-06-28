#include "Batthub.h"

Batthub::Batthub()
  : _wire(nullptr), _pins(), _options(),
    _lastBeginReport{false, false, 0, 0, 0, 0, 0} {
}

bool Batthub::begin() {
  return begin(Wire);
}

bool Batthub::begin(TwoWire &wire) {
  BeginOptions options;
  return begin(wire, options);
}

bool Batthub::begin(TwoWire &wire, const BeginOptions &options) {
  return beginReport(wire, options).ok();
}

Batthub::BeginReport Batthub::beginReport() {
  return beginReport(Wire);
}

Batthub::BeginReport Batthub::beginReport(TwoWire &wire) {
  BeginOptions options;
  return beginReport(wire, options);
}

Batthub::BeginReport Batthub::beginReport(TwoWire &wire, const BeginOptions &options) {
  _wire = &wire;
  _options = options;
  _pins = options.pins;

  if (options.setupPins) {
    setupBoardPins(options.initialCeEnabled, options.initialOtgEnabled);
  }
  if (options.startWire) {
    beginWire(wire, options);
  }

  charger.begin(wire, options.chargerAddress, false);
  bq25::DeviceInfo chargerInfo = charger.deviceInfo();

  gauge.begin(wire, options.gaugeAddress, false);
  uint16_t gaugeType = gauge.deviceType();

  _lastBeginReport.chargerPresent = charger.lastError() == 0 && chargerInfo.isBQ25895();
  _lastBeginReport.gaugePresent = gauge.lastError() == 0 && gaugeType == bq27::kDeviceId;
  _lastBeginReport.chargerI2cError = charger.lastError();
  _lastBeginReport.gaugeI2cError = gauge.lastError();
  _lastBeginReport.chargerPartNumber = chargerInfo.partNumber;
  _lastBeginReport.chargerRevision = chargerInfo.revision;
  _lastBeginReport.gaugeDeviceType = gaugeType;
  return _lastBeginReport;
}

Batthub::BeginReport Batthub::lastBeginReport() const {
  return _lastBeginReport;
}

bool Batthub::isConnected() {
  return charger.isConnected() && gauge.isConnected();
}

const Batthub::Pins &Batthub::pins() const {
  return _pins;
}

bool Batthub::cePinConnected() const {
  return pinDefined(_pins.ce);
}

bool Batthub::otgPinConnected() const {
  return pinDefined(_pins.otg);
}

bool Batthub::intPinConnected() const {
  return pinDefined(_pins.intPin);
}

bool Batthub::setCePinEnabled(bool enabled) {
  return setPinActive(_pins.ce, !_pins.ceActiveLow, enabled);
}

bool Batthub::cePinEnabled() const {
  return pinActive(_pins.ce, !_pins.ceActiveLow);
}

bool Batthub::setOtgPinEnabled(bool enabled) {
  return setPinActive(_pins.otg, _pins.otgActiveHigh, enabled);
}

bool Batthub::otgPinEnabled() const {
  return pinActive(_pins.otg, _pins.otgActiveHigh);
}

bool Batthub::interruptAsserted() const {
  return pinActive(_pins.intPin, !_pins.intActiveLow);
}

bool Batthub::setChargeEnabled(bool enabled) {
  bool ok = setCePinEnabled(enabled);
  ok &= charger.setChargingEnabled(enabled);
  return ok;
}

bool Batthub::setOtgEnabled(bool enabled) {
  bool ok = setOtgPinEnabled(enabled);
  ok &= charger.setOtgEnabled(enabled);
  return ok;
}

bool Batthub::enterShipMode(bool delayTurnOff) {
  return charger.enterShipMode(delayTurnOff);
}

bool Batthub::applySafeDefaults() {
  ChargerSettings settings;
  return applyChargerSettings(settings);
}

bool Batthub::applyChargerSettings(const ChargerSettings &settings) {
  bq25::Config config = toChargerConfig(settings);
  bool ok = charger.setAbsoluteVindpm(settings.absoluteVindpm);
  ok &= charger.applyConfig(config);
  ok &= charger.setHighImpedanceMode(settings.highImpedanceMode);
  ok &= charger.setVindpmOffset(settings.vindpmOffsetMv);
  ok &= charger.setBoostFrequency(settings.boostFrequency);
  ok &= charger.setBoostHotThreshold(settings.boostHotThreshold);
  ok &= charger.setBoostColdThreshold(settings.boostColdThreshold);
  ok &= charger.setBatteryLoadEnabled(settings.batteryLoadEnabled);
  ok &= charger.setOtgEnabled(settings.otgEnabled);
  ok &= setOtgPinEnabled(settings.otgEnabled);
  ok &= charger.setPumpXEnabled(settings.pumpXEnabled);
  ok &= charger.setBatteryLowThreshold(settings.batteryLowThreshold);
  ok &= charger.setRechargeThreshold(settings.rechargeThreshold);
  ok &= charger.setIrCompResistance(settings.irCompResistanceMohm);
  ok &= charger.setIrCompVoltageClamp(settings.irCompVoltageClampMv);
  ok &= charger.setSafetyTimerSlowedInDpm(settings.safetyTimerSlowedInDpm);
  ok &= charger.setBatfetResetEnabled(settings.batfetResetEnabled);
  if (settings.absoluteVindpm) {
    ok &= charger.setInputVoltageLimit(settings.inputVoltageLimitMv);
  }
  if (settings.adcContinuous) {
    ok &= charger.startAdc(true);
  } else {
    ok &= charger.stopAdc();
  }
  return ok;
}

bool Batthub::applyGaugeSettings(const GaugeSettings &settings, bool configureBatteryProfile) {
  bool ok = true;
  if (configureBatteryProfile) {
    ok &= configureBattery(settings.batteryProfile);
  }
  ok &= gauge.setGpoutMode(settings.gpoutMode);
  ok &= gauge.setGpoutPolarity(settings.gpoutActiveHigh);
  ok &= gauge.setBatteryInsertionEnable(settings.batteryInsertionEnabled);
  ok &= gauge.setBinPullupEnabled(settings.binPullupEnabled);
  ok &= gauge.setSleepEnabled(settings.sleepEnabled);
  ok &= gauge.setRmFccSmoothing(settings.rmFccSmoothingEnabled);
  ok &= gauge.setTemperatureSource(settings.temperatureSource);
  return ok;
}

bool Batthub::configureBattery(const BatteryProfile &profile) {
  return gauge.configureBattery(toBatteryConfig(profile));
}

bool Batthub::readBatteryProfile(BatteryProfile &profile) {
  bool ok = true;
  ok &= gauge.readDataMemoryWord(bq27::ClassState, 10, profile.designCapacityMah);
  ok &= gauge.readDataMemoryWord(bq27::ClassState, 12, profile.designEnergyMwh);
  ok &= gauge.readDataMemoryWord(bq27::ClassState, 16, profile.terminateVoltageMv);
  ok &= gauge.readDataMemoryByte(bq27::ClassState, 26, profile.socIntDeltaPercent);
  ok &= gauge.readDataMemoryWord(bq27::ClassState, 27, profile.taperRate);

  uint8_t socThresholds[4] = {0, 0, 0, 0};
  ok &= gauge.readDataMemory(bq27::ClassDischarge, 0, socThresholds, 4);
  profile.soc1SetPercent = socThresholds[0];
  profile.soc1ClearPercent = socThresholds[1];
  profile.socfSetPercent = socThresholds[2];
  profile.socfClearPercent = socThresholds[3];
  return ok;
}

bool Batthub::configure1s(uint16_t capacityMah,
                          uint16_t chargeCurrentMa,
                          uint16_t inputCurrentLimitMa,
                          uint16_t chargeVoltageMv,
                          uint16_t terminateVoltageMv,
                          bool configureGauge) {
  ChargerSettings chargerSettings;
  chargerSettings.inputCurrentLimitMa = inputCurrentLimitMa;
  chargerSettings.chargeCurrentMa = chargeCurrentMa;
  chargerSettings.chargeVoltageMv = chargeVoltageMv;
  chargerSettings.chargingEnabled = true;

  uint16_t tenthChargeCurrent = chargeCurrentMa / 10;
  if (tenthChargeCurrent < 64) {
    tenthChargeCurrent = 64;
  }
  chargerSettings.prechargeCurrentMa = tenthChargeCurrent;
  chargerSettings.terminationCurrentMa = tenthChargeCurrent;

  bool ok = applyChargerSettings(chargerSettings);

  if (configureGauge) {
    BatteryProfile battery;
    battery.designCapacityMah = capacityMah;
    battery.designEnergyMwh = static_cast<uint16_t>((static_cast<uint32_t>(capacityMah) * 37UL) / 10UL);
    battery.terminateVoltageMv = terminateVoltageMv;
    uint16_t taperDenominator = tenthChargeCurrent / 10;
    if (taperDenominator == 0) {
      taperDenominator = 1;
    }
    battery.taperRate = capacityMah / taperDenominator;
    if (battery.taperRate == 0) {
      battery.taperRate = 1;
    }
    ok &= configureBattery(battery);
  }

  return ok;
}

bool Batthub::startAdc(bool continuous) {
  return charger.startAdc(continuous);
}

bool Batthub::stopAdc() {
  return charger.stopAdc();
}

bool Batthub::setInputCurrentLimit(uint16_t milliamps) {
  return charger.setInputCurrentLimit(milliamps);
}

bool Batthub::restoreInputCurrentLimit(uint16_t milliamps) {
  if (charger.inputCurrentLimit() == milliamps) {
    return true;
  }
  return charger.setInputCurrentLimit(milliamps);
}

bool Batthub::setInputVoltageLimit(uint16_t millivolts) {
  return charger.setInputVoltageLimit(millivolts);
}

bool Batthub::setChargeCurrent(uint16_t milliamps) {
  return charger.setChargeCurrent(milliamps);
}

bool Batthub::setChargeVoltage(uint16_t millivolts) {
  return charger.setChargeVoltage(millivolts);
}

bool Batthub::setBoostVoltage(uint16_t millivolts) {
  return charger.setBoostVoltage(millivolts);
}

bool Batthub::setAdapterDetection(bool autoDpdmEnabled,
                                  bool hvdcpEnabled,
                                  bool maxChargeEnabled,
                                  bool icoEnabled) {
  bool ok = charger.setAutoDpdmEnabled(autoDpdmEnabled);
  ok &= charger.setHvdcpEnabled(hvdcpEnabled);
  ok &= charger.setMaxChargeEnabled(maxChargeEnabled);
  ok &= charger.setIcoEnabled(icoEnabled);
  return ok;
}

bool Batthub::forceDpdmDetection() {
  return charger.forceDpdmDetection();
}

bool Batthub::forceIco() {
  return charger.forceIco();
}

bool Batthub::resetCharger() {
  return charger.resetRegisters();
}

bool Batthub::resetGauge() {
  return gauge.reset();
}

bool Batthub::resetBoth() {
  bool ok = resetCharger();
  delay(100);
  ok &= resetGauge();
  return ok;
}

bool Batthub::readChargerRegisters(uint8_t *buffer, uint8_t length) {
  return charger.readRegisters(buffer, length);
}

uint16_t Batthub::stateOfChargePercent() {
  return gauge.stateOfCharge();
}

uint16_t Batthub::batteryVoltageMv() {
  return gauge.voltage();
}

int16_t Batthub::batteryCurrentMa() {
  return gauge.averageCurrent();
}

uint16_t Batthub::systemVoltageMv() {
  return charger.adcSystemVoltage();
}

uint16_t Batthub::inputVoltageMv() {
  return charger.adcVbusVoltage();
}

bool Batthub::powerGood() {
  return charger.isPowerGood();
}

bool Batthub::isCharging() {
  bq25::ChargeState state = charger.status().chargeState;
  return state == bq25::ChargePrecharge || state == bq25::ChargeFastCharge;
}

const char *Batthub::chargeStateText() {
  return bq25::chargeStateName(charger.status().chargeState);
}

const char *Batthub::inputTypeText() {
  return bq25::vbusTypeName(charger.status().vbusType);
}

const char *Batthub::chargeFaultText() {
  return bq25::chargeFaultName(charger.faults().charge);
}

const char *Batthub::ntcFaultText() {
  return bq25::ntcFaultName(charger.faults().ntc);
}

Batthub::Snapshot Batthub::snapshot() {
  Snapshot out;
  out.chargerStatus = charger.status();
  out.chargerFaults = charger.faults();
  out.chargerMeasurements = charger.measurements();
  out.chargerInfo = charger.deviceInfo();
  out.gauge = gauge.snapshot();
  out.cePinEnabled = cePinEnabled();
  out.otgPinEnabled = otgPinEnabled();
  out.interruptAsserted = interruptAsserted();
  return out;
}

bq25::Config Batthub::toChargerConfig(const ChargerSettings &settings) {
  bq25::Config config;
  config.inputCurrentLimitMa = settings.inputCurrentLimitMa;
  config.inputVoltageLimitMv = settings.inputVoltageLimitMv;
  config.chargeCurrentMa = settings.chargeCurrentMa;
  config.prechargeCurrentMa = settings.prechargeCurrentMa;
  config.terminationCurrentMa = settings.terminationCurrentMa;
  config.chargeVoltageMv = settings.chargeVoltageMv;
  config.minSystemVoltageMv = settings.minSystemVoltageMv;
  config.boostVoltageMv = settings.boostVoltageMv;
  config.watchdog = settings.watchdog;
  config.safetyTimer = settings.safetyTimer;
  config.thermalRegulation = settings.thermalRegulation;
  config.enableCharging = settings.chargingEnabled;
  config.enableTermination = settings.terminationEnabled;
  config.enableSafetyTimer = settings.safetyTimerEnabled;
  config.enableIlimPin = settings.ilimPinEnabled;
  config.enableIco = settings.icoEnabled;
  config.enableAutoDpdm = settings.autoDpdmEnabled;
  config.enableHvdcp = settings.hvdcpEnabled;
  config.enableMaxCharge = settings.maxChargeEnabled;
  return config;
}

bq27::BatteryConfig Batthub::toBatteryConfig(const BatteryProfile &profile) {
  bq27::BatteryConfig config;
  config.designCapacityMah = profile.designCapacityMah;
  config.designEnergyMwh = profile.designEnergyMwh;
  config.terminateVoltageMv = profile.terminateVoltageMv;
  config.taperRate = profile.taperRate;
  config.soc1SetPercent = profile.soc1SetPercent;
  config.soc1ClearPercent = profile.soc1ClearPercent;
  config.socfSetPercent = profile.socfSetPercent;
  config.socfClearPercent = profile.socfClearPercent;
  config.socIntDeltaPercent = profile.socIntDeltaPercent;
  return config;
}

bool Batthub::pinDefined(int8_t pin) {
  return pin >= 0;
}

void Batthub::setupBoardPins(bool initialCeEnabled, bool initialOtgEnabled) {
  if (pinDefined(_pins.ce)) {
    pinMode(_pins.ce, OUTPUT);
    setCePinEnabled(initialCeEnabled);
  }
  if (pinDefined(_pins.otg)) {
    pinMode(_pins.otg, OUTPUT);
    setOtgPinEnabled(initialOtgEnabled);
  }
  if (pinDefined(_pins.intPin)) {
    pinMode(_pins.intPin, INPUT_PULLUP);
  }
}

void Batthub::beginWire(TwoWire &wire, const BeginOptions &options) {
  wire.begin();
  wire.setClock(options.i2cClockHz);
}

bool Batthub::setPinActive(int8_t pin, bool activeHigh, bool active) {
  if (!pinDefined(pin)) {
    return true;
  }
  bool high = active ? activeHigh : !activeHigh;
  digitalWrite(pin, high ? HIGH : LOW);
  return true;
}

bool Batthub::pinActive(int8_t pin, bool activeHigh) const {
  if (!pinDefined(pin)) {
    return false;
  }
  return digitalRead(pin) == (activeHigh ? HIGH : LOW);
}
