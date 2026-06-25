#include "bq25.h"

namespace {
const uint8_t MASK_REG00_EN_HIZ = 0x80;
const uint8_t MASK_REG00_EN_ILIM = 0x40;
const uint8_t MASK_REG00_IINLIM = 0x3F;

const uint8_t MASK_REG01_BHOT = 0xC0;
const uint8_t MASK_REG01_BCOLD = 0x20;
const uint8_t MASK_REG01_VINDPM_OS = 0x1F;

const uint8_t MASK_REG02_CONV_START = 0x80;
const uint8_t MASK_REG02_CONV_RATE = 0x40;
const uint8_t MASK_REG02_BOOST_FREQ = 0x20;
const uint8_t MASK_REG02_ICO_EN = 0x10;
const uint8_t MASK_REG02_HVDCP_EN = 0x08;
const uint8_t MASK_REG02_MAXC_EN = 0x04;
const uint8_t MASK_REG02_FORCE_DPDM = 0x02;
const uint8_t MASK_REG02_AUTO_DPDM = 0x01;

const uint8_t MASK_REG03_BAT_LOAD = 0x80;
const uint8_t MASK_REG03_WD_RST = 0x40;
const uint8_t MASK_REG03_OTG = 0x20;
const uint8_t MASK_REG03_CHG = 0x10;
const uint8_t MASK_REG03_SYS_MIN = 0x0E;

const uint8_t MASK_REG04_EN_PUMPX = 0x80;
const uint8_t MASK_REG04_ICHG = 0x7F;

const uint8_t MASK_REG05_IPRECHG = 0xF0;
const uint8_t MASK_REG05_ITERM = 0x0F;

const uint8_t MASK_REG06_VREG = 0xFC;
const uint8_t MASK_REG06_BATLOWV = 0x02;
const uint8_t MASK_REG06_VRECHG = 0x01;

const uint8_t MASK_REG07_EN_TERM = 0x80;
const uint8_t MASK_REG07_WDT = 0x30;
const uint8_t MASK_REG07_EN_TIMER = 0x08;
const uint8_t MASK_REG07_CHG_TIMER = 0x06;

const uint8_t MASK_REG08_BAT_COMP = 0xE0;
const uint8_t MASK_REG08_VCLAMP = 0x1C;
const uint8_t MASK_REG08_TREG = 0x03;

const uint8_t MASK_REG09_FORCE_ICO = 0x80;
const uint8_t MASK_REG09_TMR2X_EN = 0x40;
const uint8_t MASK_REG09_BATFET_DIS = 0x20;
const uint8_t MASK_REG09_BATFET_DLY = 0x08;
const uint8_t MASK_REG09_BATFET_RST_EN = 0x04;
const uint8_t MASK_REG09_PUMPX_UP = 0x02;
const uint8_t MASK_REG09_PUMPX_DN = 0x01;

const uint8_t MASK_REG0A_BOOSTV = 0xF0;

const uint8_t MASK_REG0B_VBUS_STAT = 0xE0;
const uint8_t MASK_REG0B_CHRG_STAT = 0x18;
const uint8_t MASK_REG0B_PG_STAT = 0x04;
const uint8_t MASK_REG0B_SDP_STAT = 0x02;
const uint8_t MASK_REG0B_VSYS_STAT = 0x01;

const uint8_t MASK_REG0C_WDT_FAULT = 0x80;
const uint8_t MASK_REG0C_BOOST_FAULT = 0x40;
const uint8_t MASK_REG0C_CHRG_FAULT = 0x30;
const uint8_t MASK_REG0C_BAT_FAULT = 0x08;
const uint8_t MASK_REG0C_NTC_FAULT = 0x07;

const uint8_t MASK_REG0D_FORCE_VINDPM = 0x80;
const uint8_t MASK_REG0D_VINDPM = 0x7F;

const uint8_t MASK_REG0E_THERM_STAT = 0x80;
const uint8_t MASK_REG0E_BATV = 0x7F;

const uint8_t MASK_REG0F_SYSV = 0x7F;
const uint8_t MASK_REG10_TSPCT = 0x7F;
const uint8_t MASK_REG11_VBUS_GD = 0x80;
const uint8_t MASK_REG11_VBUSV = 0x7F;
const uint8_t MASK_REG12_ICHGR = 0x7F;
const uint8_t MASK_REG13_VDPM = 0x80;
const uint8_t MASK_REG13_IDPM = 0x40;
const uint8_t MASK_REG13_IDPM_LIM = 0x3F;
const uint8_t MASK_REG14_REG_RST = 0x80;
const uint8_t MASK_REG14_ICO_OPTIMIZED = 0x40;
const uint8_t MASK_REG14_PN = 0x38;
const uint8_t MASK_REG14_TS_PROFILE = 0x04;
const uint8_t MASK_REG14_DEV_REV = 0x03;
}

bq25::bq25() : _wire(nullptr), _address(kDefaultAddress), _lastError(0) {
}

bool bq25::begin(TwoWire &wire, uint8_t address, bool startWire) {
  _wire = &wire;
  _address = address;
  _lastError = 0;
  if (startWire) {
    _wire->begin();
  }
  return isConnected();
}

bool bq25::isConnected() {
  DeviceInfo info = deviceInfo();
  return _lastError == 0 && info.isBQ25895();
}

uint8_t bq25::address() const {
  return _address;
}

uint8_t bq25::lastError() const {
  return _lastError;
}

bool bq25::readRegister(uint8_t reg, uint8_t &value) {
  if (_wire == nullptr) {
    _lastError = 0xFE;
    return false;
  }

  _wire->beginTransmission(_address);
  _wire->write(reg);
  _lastError = _wire->endTransmission(false);
  if (_lastError != 0) {
    return false;
  }

  uint8_t received = _wire->requestFrom(_address, static_cast<uint8_t>(1));
  if (received != 1) {
    _lastError = 0xFD;
    return false;
  }

  value = _wire->read();
  _lastError = 0;
  return true;
}

bool bq25::writeRegister(uint8_t reg, uint8_t value) {
  if (_wire == nullptr) {
    _lastError = 0xFE;
    return false;
  }

  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  _lastError = _wire->endTransmission(true);
  return _lastError == 0;
}

bool bq25::updateRegister(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t current = 0;
  if (!readRegister(reg, current)) {
    return false;
  }
  current = (current & ~mask) | (value & mask);
  return writeRegister(reg, current);
}

bool bq25::readRegisters(uint8_t *buffer, uint8_t length, uint8_t startRegister) {
  if (buffer == nullptr) {
    _lastError = 0xFC;
    return false;
  }
  for (uint8_t i = 0; i < length; ++i) {
    if (!readRegister(startRegister + i, buffer[i])) {
      return false;
    }
  }
  return true;
}

bool bq25::resetRegisters() {
  bool ok = updateRegister(RegPartInfo, MASK_REG14_REG_RST, MASK_REG14_REG_RST);
  delay(50);
  return ok;
}

bool bq25::resetWatchdog() {
  return updateRegister(RegPowerPath, MASK_REG03_WD_RST, MASK_REG03_WD_RST);
}

bool bq25::applyConfig(const Config &config) {
  bool ok = true;
  ok &= setIlimPinEnabled(config.enableIlimPin);
  ok &= setInputCurrentLimit(config.inputCurrentLimitMa);
  ok &= setInputVoltageLimit(config.inputVoltageLimitMv);
  ok &= setChargeCurrent(config.chargeCurrentMa);
  ok &= setPrechargeCurrent(config.prechargeCurrentMa);
  ok &= setTerminationCurrent(config.terminationCurrentMa);
  ok &= setChargeVoltage(config.chargeVoltageMv);
  ok &= setMinSystemVoltage(config.minSystemVoltageMv);
  ok &= setBoostVoltage(config.boostVoltageMv);
  ok &= setWatchdogTimer(config.watchdog);
  ok &= setSafetyTimer(config.safetyTimer);
  ok &= setSafetyTimerEnabled(config.enableSafetyTimer);
  ok &= setTerminationEnabled(config.enableTermination);
  ok &= setThermalRegulation(config.thermalRegulation);
  ok &= setIcoEnabled(config.enableIco);
  ok &= setAutoDpdmEnabled(config.enableAutoDpdm);
  ok &= setHvdcpEnabled(config.enableHvdcp);
  ok &= setMaxChargeEnabled(config.enableMaxCharge);
  ok &= setChargingEnabled(config.enableCharging);
  return ok;
}

bool bq25::setHighImpedanceMode(bool enabled) {
  return updateRegister(RegInputCurrent, MASK_REG00_EN_HIZ, enabled ? MASK_REG00_EN_HIZ : 0);
}

bool bq25::setIlimPinEnabled(bool enabled) {
  return updateRegister(RegInputCurrent, MASK_REG00_EN_ILIM, enabled ? MASK_REG00_EN_ILIM : 0);
}

bool bq25::setInputCurrentLimit(uint16_t milliamps) {
  return writeField(RegInputCurrent, MASK_REG00_IINLIM, 0, encodeLinear(milliamps, 100, 50, 63));
}

uint16_t bq25::inputCurrentLimit() {
  return decodeLinear(readField(RegInputCurrent, MASK_REG00_IINLIM, 0), 100, 50);
}

bool bq25::setBoostHotThreshold(BoostHotThreshold threshold) {
  return writeField(RegVindpmOffset, MASK_REG01_BHOT, 6, static_cast<uint8_t>(threshold));
}

bool bq25::setBoostColdThreshold(BoostColdThreshold threshold) {
  return writeField(RegVindpmOffset, MASK_REG01_BCOLD, 5, static_cast<uint8_t>(threshold));
}

bool bq25::setVindpmOffset(uint16_t millivolts) {
  return writeField(RegVindpmOffset, MASK_REG01_VINDPM_OS, 0, encodeLinear(millivolts, 0, 100, 31));
}

bool bq25::startAdc(bool continuous) {
  bool ok = setAdcContinuous(continuous);
  if (!continuous) {
    ok &= updateRegister(RegAdcAndDpdm, MASK_REG02_CONV_START, MASK_REG02_CONV_START);
  }
  return ok;
}

bool bq25::stopAdc() {
  return setAdcContinuous(false);
}

bool bq25::setAdcContinuous(bool enabled) {
  return updateRegister(RegAdcAndDpdm, MASK_REG02_CONV_RATE, enabled ? MASK_REG02_CONV_RATE : 0);
}

bool bq25::setBoostFrequency(BoostFrequency frequency) {
  return writeField(RegAdcAndDpdm, MASK_REG02_BOOST_FREQ, 5, static_cast<uint8_t>(frequency));
}

bool bq25::setIcoEnabled(bool enabled) {
  return updateRegister(RegAdcAndDpdm, MASK_REG02_ICO_EN, enabled ? MASK_REG02_ICO_EN : 0);
}

bool bq25::setHvdcpEnabled(bool enabled) {
  return updateRegister(RegAdcAndDpdm, MASK_REG02_HVDCP_EN, enabled ? MASK_REG02_HVDCP_EN : 0);
}

bool bq25::setMaxChargeEnabled(bool enabled) {
  return updateRegister(RegAdcAndDpdm, MASK_REG02_MAXC_EN, enabled ? MASK_REG02_MAXC_EN : 0);
}

bool bq25::forceDpdmDetection() {
  return updateRegister(RegAdcAndDpdm, MASK_REG02_FORCE_DPDM, MASK_REG02_FORCE_DPDM);
}

bool bq25::setAutoDpdmEnabled(bool enabled) {
  return updateRegister(RegAdcAndDpdm, MASK_REG02_AUTO_DPDM, enabled ? MASK_REG02_AUTO_DPDM : 0);
}

bool bq25::setBatteryLoadEnabled(bool enabled) {
  return updateRegister(RegPowerPath, MASK_REG03_BAT_LOAD, enabled ? MASK_REG03_BAT_LOAD : 0);
}

bool bq25::setOtgEnabled(bool enabled) {
  return updateRegister(RegPowerPath, MASK_REG03_OTG, enabled ? MASK_REG03_OTG : 0);
}

bool bq25::setChargingEnabled(bool enabled) {
  return updateRegister(RegPowerPath, MASK_REG03_CHG, enabled ? MASK_REG03_CHG : 0);
}

bool bq25::setMinSystemVoltage(uint16_t millivolts) {
  return writeField(RegPowerPath, MASK_REG03_SYS_MIN, 1, encodeLinear(millivolts, 3000, 100, 7));
}

bool bq25::setPumpXEnabled(bool enabled) {
  return updateRegister(RegChargeCurrent, MASK_REG04_EN_PUMPX, enabled ? MASK_REG04_EN_PUMPX : 0);
}

bool bq25::setChargeCurrent(uint16_t milliamps) {
  return writeField(RegChargeCurrent, MASK_REG04_ICHG, 0, encodeLinear(milliamps, 0, 64, 79));
}

bool bq25::setPrechargeCurrent(uint16_t milliamps) {
  return writeField(RegPrechargeTermination, MASK_REG05_IPRECHG, 4, encodeLinear(milliamps, 64, 64, 15));
}

bool bq25::setTerminationCurrent(uint16_t milliamps) {
  return writeField(RegPrechargeTermination, MASK_REG05_ITERM, 0, encodeLinear(milliamps, 64, 64, 15));
}

bool bq25::setChargeVoltage(uint16_t millivolts) {
  return writeField(RegChargeVoltage, MASK_REG06_VREG, 2, encodeLinear(millivolts, 3840, 16, 48));
}

bool bq25::setBatteryLowThreshold(BatteryLowThreshold threshold) {
  return writeField(RegChargeVoltage, MASK_REG06_BATLOWV, 1, static_cast<uint8_t>(threshold));
}

bool bq25::setRechargeThreshold(RechargeThreshold threshold) {
  return writeField(RegChargeVoltage, MASK_REG06_VRECHG, 0, static_cast<uint8_t>(threshold));
}

bool bq25::setTerminationEnabled(bool enabled) {
  return updateRegister(RegChargeControl, MASK_REG07_EN_TERM, enabled ? MASK_REG07_EN_TERM : 0);
}

bool bq25::setWatchdogTimer(WatchdogTimer timer) {
  return writeField(RegChargeControl, MASK_REG07_WDT, 4, static_cast<uint8_t>(timer));
}

bool bq25::setSafetyTimerEnabled(bool enabled) {
  return updateRegister(RegChargeControl, MASK_REG07_EN_TIMER, enabled ? MASK_REG07_EN_TIMER : 0);
}

bool bq25::setSafetyTimer(SafetyTimer timer) {
  return writeField(RegChargeControl, MASK_REG07_CHG_TIMER, 1, static_cast<uint8_t>(timer));
}

bool bq25::setIrCompResistance(uint16_t milliohms) {
  return writeField(RegIrCompThermal, MASK_REG08_BAT_COMP, 5, encodeLinear(milliohms, 0, 20, 7));
}

bool bq25::setIrCompVoltageClamp(uint16_t millivolts) {
  return writeField(RegIrCompThermal, MASK_REG08_VCLAMP, 2, encodeLinear(millivolts, 0, 32, 7));
}

bool bq25::setThermalRegulation(ThermalRegulation temperature) {
  return writeField(RegIrCompThermal, MASK_REG08_TREG, 0, static_cast<uint8_t>(temperature));
}

bool bq25::forceIco() {
  return updateRegister(RegMiscControl, MASK_REG09_FORCE_ICO, MASK_REG09_FORCE_ICO);
}

bool bq25::setSafetyTimerSlowedInDpm(bool enabled) {
  return updateRegister(RegMiscControl, MASK_REG09_TMR2X_EN, enabled ? MASK_REG09_TMR2X_EN : 0);
}

bool bq25::enterShipMode(bool delayTurnOff) {
  bool ok = updateRegister(RegMiscControl, MASK_REG09_BATFET_DLY, delayTurnOff ? MASK_REG09_BATFET_DLY : 0);
  ok &= updateRegister(RegMiscControl, MASK_REG09_BATFET_DIS, MASK_REG09_BATFET_DIS);
  return ok;
}

bool bq25::setBatfetResetEnabled(bool enabled) {
  return updateRegister(RegMiscControl, MASK_REG09_BATFET_RST_EN, enabled ? MASK_REG09_BATFET_RST_EN : 0);
}

bool bq25::pumpXIncrease() {
  return updateRegister(RegMiscControl, MASK_REG09_PUMPX_UP, MASK_REG09_PUMPX_UP);
}

bool bq25::pumpXDecrease() {
  return updateRegister(RegMiscControl, MASK_REG09_PUMPX_DN, MASK_REG09_PUMPX_DN);
}

bool bq25::setBoostVoltage(uint16_t millivolts) {
  return writeField(RegBoostVoltage, MASK_REG0A_BOOSTV, 4, encodeLinear(millivolts, 4550, 64, 15));
}

bool bq25::setAbsoluteVindpm(bool enabled) {
  return updateRegister(RegVindpm, MASK_REG0D_FORCE_VINDPM, enabled ? MASK_REG0D_FORCE_VINDPM : 0);
}

bool bq25::setInputVoltageLimit(uint16_t millivolts) {
  return writeField(RegVindpm, MASK_REG0D_VINDPM, 0, encodeLinear(millivolts, 2600, 100, 127));
}

bq25::Status bq25::status() {
  uint8_t reg = 0;
  Status out = {VbusNone, ChargeNotCharging, false, false, false};
  if (!readRegister(RegSystemStatus, reg)) {
    return out;
  }
  out.vbusType = static_cast<VbusType>((reg & MASK_REG0B_VBUS_STAT) >> 5);
  out.chargeState = static_cast<ChargeState>((reg & MASK_REG0B_CHRG_STAT) >> 3);
  out.powerGood = (reg & MASK_REG0B_PG_STAT) != 0;
  out.usbInput = (reg & MASK_REG0B_SDP_STAT) != 0;
  out.systemInRegulation = (reg & MASK_REG0B_VSYS_STAT) != 0;
  return out;
}

bq25::Faults bq25::faults() {
  uint8_t reg = 0;
  Faults out = {false, false, ChargeFaultNormal, NtcNormal};
  if (!readRegister(RegFault, reg)) {
    return out;
  }
  out.watchdog = (reg & MASK_REG0C_WDT_FAULT) != 0;
  out.boost = (reg & MASK_REG0C_BOOST_FAULT) != 0;
  out.charge = static_cast<ChargeFault>((reg & MASK_REG0C_CHRG_FAULT) >> 4);
  out.battery = (reg & MASK_REG0C_BAT_FAULT) != 0;
  out.ntc = static_cast<NtcFault>(reg & MASK_REG0C_NTC_FAULT);
  return out;
}

bq25::Measurements bq25::measurements() {
  Measurements out;
  out.thermalRegulation = false;
  out.vbusGood = false;
  out.vindpm = false;
  out.iindpm = false;
  out.batteryMv = adcBatteryVoltage();
  out.systemMv = adcSystemVoltage();
  out.vbusMv = adcVbusVoltage();
  out.chargeCurrentMa = adcChargeCurrent();
  out.effectiveInputCurrentLimitMa = effectiveInputCurrentLimit();
  out.tsPercentX100 = adcTsPercentX100();

  uint8_t reg = 0;
  if (readRegister(RegBatteryVoltageAdc, reg)) {
    out.thermalRegulation = (reg & MASK_REG0E_THERM_STAT) != 0;
  }
  if (readRegister(RegVbusVoltageAdc, reg)) {
    out.vbusGood = (reg & MASK_REG11_VBUS_GD) != 0;
  }
  if (readRegister(RegDpmStatus, reg)) {
    out.vindpm = (reg & MASK_REG13_VDPM) != 0;
    out.iindpm = (reg & MASK_REG13_IDPM) != 0;
  }
  return out;
}

bq25::DeviceInfo bq25::deviceInfo() {
  uint8_t reg = 0;
  DeviceInfo out = {false, 0, false, 0};
  if (!readRegister(RegPartInfo, reg)) {
    return out;
  }
  out.icoOptimized = (reg & MASK_REG14_ICO_OPTIMIZED) != 0;
  out.partNumber = (reg & MASK_REG14_PN) >> 3;
  out.coldHotTemperatureProfile = (reg & MASK_REG14_TS_PROFILE) == 0;
  out.revision = reg & MASK_REG14_DEV_REV;
  return out;
}

uint16_t bq25::adcBatteryVoltage() {
  return decodeLinear(readField(RegBatteryVoltageAdc, MASK_REG0E_BATV, 0), 2304, 20);
}

uint16_t bq25::adcSystemVoltage() {
  return decodeLinear(readField(RegSystemVoltageAdc, MASK_REG0F_SYSV, 0), 2304, 20);
}

uint16_t bq25::adcVbusVoltage() {
  return decodeLinear(readField(RegVbusVoltageAdc, MASK_REG11_VBUSV, 0), 2600, 100);
}

uint16_t bq25::adcChargeCurrent() {
  return decodeLinear(readField(RegChargeCurrentAdc, MASK_REG12_ICHGR, 0), 0, 50);
}

uint16_t bq25::adcTsPercentX100() {
  uint8_t code = readField(RegTsAdc, MASK_REG10_TSPCT, 0);
  return 2100 + static_cast<uint16_t>(code) * 46 + (static_cast<uint16_t>(code) / 2);
}

uint16_t bq25::effectiveInputCurrentLimit() {
  return decodeLinear(readField(RegDpmStatus, MASK_REG13_IDPM_LIM, 0), 100, 50);
}

bool bq25::isChargeDone() {
  return status().chargeState == ChargeDone;
}

bool bq25::isPowerGood() {
  return status().powerGood;
}

bool bq25::isInVindpm() {
  uint8_t reg = 0;
  return readRegister(RegDpmStatus, reg) && ((reg & MASK_REG13_VDPM) != 0);
}

bool bq25::isInIindpm() {
  uint8_t reg = 0;
  return readRegister(RegDpmStatus, reg) && ((reg & MASK_REG13_IDPM) != 0);
}

const char *bq25::vbusTypeName(VbusType type) {
  switch (type) {
    case VbusNone: return "none";
    case VbusUsbSdp: return "USB SDP";
    case VbusUsbCdp: return "USB CDP";
    case VbusUsbDcp: return "USB DCP";
    case VbusMaxCharge: return "MaxCharge";
    case VbusUnknown: return "unknown";
    case VbusNonStandard: return "non-standard";
    case VbusOtg: return "OTG";
  }
  return "?";
}

const char *bq25::chargeStateName(ChargeState state) {
  switch (state) {
    case ChargeNotCharging: return "not charging";
    case ChargePrecharge: return "precharge";
    case ChargeFastCharge: return "fast charge";
    case ChargeDone: return "done";
  }
  return "?";
}

const char *bq25::chargeFaultName(ChargeFault fault) {
  switch (fault) {
    case ChargeFaultNormal: return "normal";
    case ChargeFaultInput: return "input";
    case ChargeFaultThermalShutdown: return "thermal shutdown";
    case ChargeFaultSafetyTimer: return "safety timer";
  }
  return "?";
}

const char *bq25::ntcFaultName(NtcFault fault) {
  switch (fault) {
    case NtcNormal: return "normal";
    case NtcBuckCold: return "buck cold";
    case NtcBuckHot: return "buck hot";
    case NtcBoostCold: return "boost cold";
    case NtcBoostHot: return "boost hot";
  }
  return "?";
}

bool bq25::writeField(uint8_t reg, uint8_t mask, uint8_t shift, uint8_t code) {
  return updateRegister(reg, mask, static_cast<uint8_t>((code << shift) & mask));
}

uint8_t bq25::readField(uint8_t reg, uint8_t mask, uint8_t shift) {
  uint8_t value = 0;
  if (!readRegister(reg, value)) {
    return 0;
  }
  return (value & mask) >> shift;
}

uint8_t bq25::encodeLinear(uint16_t value, uint16_t base, uint16_t step, uint8_t maxCode) {
  if (value <= base) {
    return 0;
  }
  uint32_t code = (static_cast<uint32_t>(value) - base + (step / 2)) / step;
  if (code > maxCode) {
    code = maxCode;
  }
  return static_cast<uint8_t>(code);
}

uint16_t bq25::decodeLinear(uint8_t code, uint16_t base, uint16_t step) {
  return static_cast<uint16_t>(base + static_cast<uint16_t>(code) * step);
}
