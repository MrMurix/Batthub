#include "bq27.h"

namespace {
const uint16_t STATUS_SHUTDOWNEN = (1u << 15);
const uint16_t STATUS_WDRESET = (1u << 14);
const uint16_t STATUS_SS = (1u << 13);
const uint16_t STATUS_CALMODE = (1u << 12);
const uint16_t STATUS_CCA = (1u << 11);
const uint16_t STATUS_BCA = (1u << 10);
const uint16_t STATUS_QMAX_UP = (1u << 9);
const uint16_t STATUS_RES_UP = (1u << 8);
const uint16_t STATUS_INITCOMP = (1u << 7);
const uint16_t STATUS_HIBERNATE = (1u << 6);
const uint16_t STATUS_SLEEP = (1u << 4);
const uint16_t STATUS_LDMD = (1u << 3);
const uint16_t STATUS_RUP_DIS = (1u << 2);
const uint16_t STATUS_VOK = (1u << 1);

const uint16_t FLAG_OT = (1u << 15);
const uint16_t FLAG_UT = (1u << 14);
const uint16_t FLAG_FC = (1u << 9);
const uint16_t FLAG_CHG = (1u << 8);
const uint16_t FLAG_OCVTAKEN = (1u << 7);
const uint16_t FLAG_ITPOR = (1u << 5);
const uint16_t FLAG_CFGUPMODE = (1u << 4);
const uint16_t FLAG_BAT_DET = (1u << 3);
const uint16_t FLAG_SOC1 = (1u << 2);
const uint16_t FLAG_SOCF = (1u << 1);
const uint16_t FLAG_DSG = (1u << 0);

const uint16_t OPCONFIG_BIE = (1u << 13);
const uint16_t OPCONFIG_BI_PU_EN = (1u << 12);
const uint16_t OPCONFIG_GPIOPOL = (1u << 11);
const uint16_t OPCONFIG_SLEEP = (1u << 5);
const uint16_t OPCONFIG_RMFCC = (1u << 4);
const uint16_t OPCONFIG_BATLOWEN = (1u << 2);
const uint16_t OPCONFIG_TEMPS = (1u << 0);

const uint8_t STATE_OFFSET_DESIGN_CAPACITY = 10;
const uint8_t STATE_OFFSET_DESIGN_ENERGY = 12;
const uint8_t STATE_OFFSET_TERMINATE_VOLTAGE = 16;
const uint8_t STATE_OFFSET_SOC_INT_DELTA = 26;
const uint8_t STATE_OFFSET_TAPER_RATE = 27;

const uint8_t DISCHARGE_OFFSET_SOC1_SET = 0;
const uint8_t DISCHARGE_OFFSET_SOCF_SET = 2;
}

bq27::bq27()
  : _wire(nullptr), _address(kDefaultAddress), _lastError(0), _wasSealed(false), _inConfigUpdate(false) {
}

bool bq27::begin(TwoWire &wire, uint8_t address, bool startWire) {
  _wire = &wire;
  _address = address;
  _lastError = 0;
  _wasSealed = false;
  _inConfigUpdate = false;
  if (startWire) {
    _wire->begin();
  }
  return isConnected();
}

bool bq27::isConnected() {
  return deviceType() == kDeviceId && _lastError == 0;
}

uint8_t bq27::address() const {
  return _address;
}

uint8_t bq27::lastError() const {
  return _lastError;
}

bool bq27::readBytes(uint8_t subAddress, uint8_t *dest, uint8_t count) {
  if (_wire == nullptr || dest == nullptr) {
    _lastError = 0xFE;
    return false;
  }

  _wire->beginTransmission(_address);
  _wire->write(subAddress);
  _lastError = _wire->endTransmission(false);
  if (_lastError != 0) {
    return false;
  }

  uint8_t received = _wire->requestFrom(_address, count);
  if (received != count) {
    _lastError = 0xFD;
    return false;
  }

  for (uint8_t i = 0; i < count; ++i) {
    dest[i] = _wire->read();
  }
  _lastError = 0;
  return true;
}

bool bq27::writeBytes(uint8_t subAddress, const uint8_t *src, uint8_t count) {
  if (_wire == nullptr || src == nullptr) {
    _lastError = 0xFE;
    return false;
  }

  _wire->beginTransmission(_address);
  _wire->write(subAddress);
  for (uint8_t i = 0; i < count; ++i) {
    _wire->write(src[i]);
  }
  _lastError = _wire->endTransmission(true);
  return _lastError == 0;
}

bool bq27::readWord(uint8_t command, uint16_t &value) {
  uint8_t data[2] = {0, 0};
  if (!readBytes(command, data, 2)) {
    return false;
  }
  value = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  return true;
}

uint16_t bq27::readWord(uint8_t command) {
  uint16_t value = 0;
  readWord(command, value);
  return value;
}

bool bq27::writeWord(uint8_t command, uint16_t value) {
  uint8_t data[2] = {
    static_cast<uint8_t>(value & 0xFF),
    static_cast<uint8_t>(value >> 8)
  };
  return writeBytes(command, data, 2);
}

bool bq27::executeControl(uint16_t subcommand) {
  return writeWord(CmdControl, subcommand);
}

bool bq27::readControl(uint16_t subcommand, uint16_t &value) {
  if (!executeControl(subcommand)) {
    return false;
  }
  delay(2);
  return readWord(CmdControl, value);
}

uint16_t bq27::readControl(uint16_t subcommand) {
  uint16_t value = 0;
  readControl(subcommand, value);
  return value;
}

uint16_t bq27::deviceType() {
  return readControl(ControlDeviceType);
}

uint16_t bq27::firmwareVersion() {
  return readControl(ControlFirmwareVersion);
}

uint16_t bq27::chemistryId() {
  return readControl(ControlChemId);
}

uint16_t bq27::controlStatus() {
  return readControl(ControlStatus);
}

bq27::ControlStatusBits bq27::controlStatusBits() {
  uint16_t raw = controlStatus();
  ControlStatusBits out;
  out.shutdownEnabled = (raw & STATUS_SHUTDOWNEN) != 0;
  out.watchdogReset = (raw & STATUS_WDRESET) != 0;
  out.sealed = (raw & STATUS_SS) != 0;
  out.calibrationMode = (raw & STATUS_CALMODE) != 0;
  out.cca = (raw & STATUS_CCA) != 0;
  out.bca = (raw & STATUS_BCA) != 0;
  out.qmaxUpdated = (raw & STATUS_QMAX_UP) != 0;
  out.resistanceUpdated = (raw & STATUS_RES_UP) != 0;
  out.initComplete = (raw & STATUS_INITCOMP) != 0;
  out.hibernate = (raw & STATUS_HIBERNATE) != 0;
  out.sleep = (raw & STATUS_SLEEP) != 0;
  out.loadMode = (raw & STATUS_LDMD) != 0;
  out.resistanceUpdatesDisabled = (raw & STATUS_RUP_DIS) != 0;
  out.voltageOk = (raw & STATUS_VOK) != 0;
  return out;
}

bool bq27::unseal(uint16_t key) {
  bool ok = executeControl(key);
  ok &= executeControl(key);
  delay(10);
  return ok;
}

bool bq27::seal() {
  return executeControl(ControlSeal);
}

bool bq27::sealed() {
  return (controlStatus() & STATUS_SS) != 0;
}

bool bq27::softReset() {
  return executeControl(ControlSoftReset);
}

bool bq27::reset() {
  return executeControl(ControlReset);
}

bool bq27::batteryInsert() {
  return executeControl(ControlBatteryInsert);
}

bool bq27::batteryRemove() {
  return executeControl(ControlBatteryRemove);
}

bool bq27::setHibernate() {
  return executeControl(ControlSetHibernate);
}

bool bq27::clearHibernate() {
  return executeControl(ControlClearHibernate);
}

bool bq27::shutdownEnable() {
  return executeControl(ControlShutdownEnable);
}

bool bq27::shutdown() {
  return executeControl(ControlShutdown);
}

bool bq27::pulseGpout() {
  return executeControl(ControlPulseSocInt);
}

uint16_t bq27::flagsRaw() {
  return readWord(CmdFlags);
}

bq27::Flags bq27::flags() {
  uint16_t raw = flagsRaw();
  Flags out;
  out.overTemperature = (raw & FLAG_OT) != 0;
  out.underTemperature = (raw & FLAG_UT) != 0;
  out.fullCharge = (raw & FLAG_FC) != 0;
  out.charging = (raw & FLAG_CHG) != 0;
  out.ocvTaken = (raw & FLAG_OCVTAKEN) != 0;
  out.powerOnReset = (raw & FLAG_ITPOR) != 0;
  out.configUpdateMode = (raw & FLAG_CFGUPMODE) != 0;
  out.batteryDetected = (raw & FLAG_BAT_DET) != 0;
  out.soc1 = (raw & FLAG_SOC1) != 0;
  out.socFinal = (raw & FLAG_SOCF) != 0;
  out.discharging = (raw & FLAG_DSG) != 0;
  return out;
}

bool bq27::enterConfigUpdate(uint16_t timeoutMs) {
  if (_inConfigUpdate) {
    return true;
  }

  _wasSealed = sealed();
  if (_wasSealed && !unseal()) {
    return false;
  }

  if (!executeControl(ControlSetConfigUpdate)) {
    if (_wasSealed) {
      seal();
    }
    return false;
  }

  if (!waitForConfigFlag(true, timeoutMs)) {
    if (_wasSealed) {
      seal();
    }
    return false;
  }

  _inConfigUpdate = true;
  return true;
}

bool bq27::exitConfigUpdate(ExitConfigMode mode, uint16_t timeoutMs) {
  bool ok = true;
  if (mode == ExitWithoutResim) {
    ok = executeControl(ControlExitConfigUpdate);
  } else if (mode == ExitWithResim) {
    ok = executeControl(ControlExitResim);
  } else {
    ok = executeControl(ControlSoftReset);
  }

  if (ok) {
    ok = waitForConfigFlag(false, timeoutMs);
  }

  if (_wasSealed) {
    ok &= seal();
  }
  _inConfigUpdate = false;
  return ok;
}

uint16_t bq27::voltage() {
  return readWord(CmdVoltage);
}

int16_t bq27::averageCurrent() {
  return static_cast<int16_t>(readWord(CmdAverageCurrent));
}

int16_t bq27::standbyCurrent() {
  return static_cast<int16_t>(readWord(CmdStandbyCurrent));
}

int16_t bq27::maxLoadCurrent() {
  return static_cast<int16_t>(readWord(CmdMaxLoadCurrent));
}

int16_t bq27::averagePower() {
  return static_cast<int16_t>(readWord(CmdAveragePower));
}

uint16_t bq27::stateOfCharge(bool unfiltered) {
  return readWord(unfiltered ? CmdStateOfChargeUnfiltered : CmdStateOfCharge);
}

uint8_t bq27::stateOfHealthPercent() {
  return static_cast<uint8_t>(readWord(CmdStateOfHealth) & 0xFF);
}

uint8_t bq27::stateOfHealthStatus() {
  return static_cast<uint8_t>(readWord(CmdStateOfHealth) >> 8);
}

uint16_t bq27::capacity(CapacityKind kind) {
  switch (kind) {
    case CapacityNominalAvailable: return readWord(CmdNominalAvailableCapacity);
    case CapacityFullAvailable: return readWord(CmdFullAvailableCapacity);
    case CapacityRemaining: return readWord(CmdRemainingCapacity);
    case CapacityFullCharge: return readWord(CmdFullChargeCapacity);
    case CapacityRemainingUnfiltered: return readWord(CmdRemainingCapacityUnfiltered);
    case CapacityRemainingFiltered: return readWord(CmdRemainingCapacityFiltered);
    case CapacityFullChargeUnfiltered: return readWord(CmdFullChargeCapacityUnfiltered);
    case CapacityFullChargeFiltered: return readWord(CmdFullChargeCapacityFiltered);
    case CapacityDesign: return readWord(ExtDesignCapacity);
  }
  return 0;
}

uint16_t bq27::temperatureDeciKelvin(bool internal) {
  return readWord(internal ? CmdInternalTemperature : CmdTemperature);
}

int16_t bq27::temperatureCelsiusX10(bool internal) {
  uint16_t deciKelvin = temperatureDeciKelvin(internal);
  return static_cast<int16_t>(deciKelvin) - 2731;
}

bq27::Snapshot bq27::snapshot() {
  Snapshot out;
  out.voltageMv = voltage();
  out.averageCurrentMa = averageCurrent();
  out.standbyCurrentMa = standbyCurrent();
  out.maxLoadCurrentMa = maxLoadCurrent();
  out.averagePowerMw = averagePower();
  out.stateOfChargePercent = stateOfCharge(false);
  out.stateOfChargeUnfilteredPercent = stateOfCharge(true);
  out.remainingCapacityMah = capacity(CapacityRemaining);
  out.fullChargeCapacityMah = capacity(CapacityFullCharge);
  out.stateOfHealthPercent = stateOfHealthPercent();
  out.stateOfHealthStatus = stateOfHealthStatus();
  out.temperatureCelsiusX10 = temperatureCelsiusX10(false);
  out.internalTemperatureCelsiusX10 = temperatureCelsiusX10(true);
  out.flags = flags();
  return out;
}

bool bq27::readDataMemory(uint8_t classId, uint8_t offset, uint8_t *data, uint8_t length, bool manageConfig) {
  if (data == nullptr) {
    _lastError = 0xFC;
    return false;
  }
  if (length == 0) {
    return true;
  }

  bool opened = false;
  if (manageConfig && !_inConfigUpdate) {
    if (!enterConfigUpdate()) {
      return false;
    }
    opened = true;
  }

  uint8_t remaining = length;
  uint8_t srcOffset = offset;
  uint8_t destOffset = 0;
  bool ok = true;

  while (remaining > 0 && ok) {
    uint8_t blockNumber = srcOffset / 32;
    uint8_t blockOffset = srcOffset % 32;
    uint8_t available = static_cast<uint8_t>(32 - blockOffset);
    uint8_t chunk = remaining < available ? remaining : available;
    uint8_t block[32];
    ok = selectDataBlock(classId, blockNumber) && readBlock(block);
    if (ok) {
      for (uint8_t i = 0; i < chunk; ++i) {
        data[destOffset + i] = block[blockOffset + i];
      }
      remaining -= chunk;
      srcOffset += chunk;
      destOffset += chunk;
    }
  }

  if (opened) {
    ok &= exitConfigUpdate(ExitWithoutResim);
  }
  return ok;
}

bool bq27::writeDataMemory(uint8_t classId, uint8_t offset, const uint8_t *data, uint8_t length, bool manageConfig) {
  if (data == nullptr) {
    _lastError = 0xFC;
    return false;
  }
  if (length == 0) {
    return true;
  }

  bool opened = false;
  if (manageConfig && !_inConfigUpdate) {
    if (!enterConfigUpdate()) {
      return false;
    }
    opened = true;
  }

  uint8_t remaining = length;
  uint8_t destOffset = offset;
  uint8_t srcOffset = 0;
  bool ok = true;

  while (remaining > 0 && ok) {
    uint8_t blockNumber = destOffset / 32;
    uint8_t blockOffset = destOffset % 32;
    uint8_t available = static_cast<uint8_t>(32 - blockOffset);
    uint8_t chunk = remaining < available ? remaining : available;
    uint8_t block[32];
    ok = selectDataBlock(classId, blockNumber) && readBlock(block);
    if (ok) {
      for (uint8_t i = 0; i < chunk; ++i) {
        block[blockOffset + i] = data[srcOffset + i];
      }
      ok = writeBlock(block);
      remaining -= chunk;
      destOffset += chunk;
      srcOffset += chunk;
    }
  }

  if (opened) {
    ok &= exitConfigUpdate(ExitWithSoftReset);
  }
  return ok;
}

uint8_t bq27::readDataMemoryByte(uint8_t classId, uint8_t offset, bool manageConfig) {
  uint8_t value = 0;
  readDataMemory(classId, offset, &value, 1, manageConfig);
  return value;
}

bool bq27::writeDataMemoryByte(uint8_t classId, uint8_t offset, uint8_t value, bool manageConfig) {
  return writeDataMemory(classId, offset, &value, 1, manageConfig);
}

bool bq27::readDataMemoryWord(uint8_t classId, uint8_t offset, uint16_t &value, bool manageConfig) {
  uint8_t data[2] = {0, 0};
  if (!readDataMemory(classId, offset, data, 2, manageConfig)) {
    return false;
  }
  value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return true;
}

bool bq27::writeDataMemoryWord(uint8_t classId, uint8_t offset, uint16_t value, bool manageConfig) {
  uint8_t data[2] = {
    static_cast<uint8_t>(value >> 8),
    static_cast<uint8_t>(value & 0xFF)
  };
  return writeDataMemory(classId, offset, data, 2, manageConfig);
}

bool bq27::configureBattery(const BatteryConfig &config) {
  bool ok = enterConfigUpdate();
  if (!ok) {
    return false;
  }
  ok &= setDesignCapacity(config.designCapacityMah, false);
  ok &= setDesignEnergy(config.designEnergyMwh, false);
  ok &= setTerminateVoltage(config.terminateVoltageMv, false);
  ok &= setTaperRate(config.taperRate, false);
  ok &= setSoc1Thresholds(config.soc1SetPercent, config.soc1ClearPercent, false);
  ok &= setSocfThresholds(config.socfSetPercent, config.socfClearPercent, false);
  ok &= setSocIntDelta(config.socIntDeltaPercent, false);
  ok &= exitConfigUpdate(ExitWithSoftReset);
  return ok;
}

bool bq27::setDesignCapacity(uint16_t capacityMah, bool manageConfig) {
  return writeDataMemoryWord(ClassState, STATE_OFFSET_DESIGN_CAPACITY, capacityMah, manageConfig);
}

bool bq27::setDesignEnergy(uint16_t energyMwh, bool manageConfig) {
  return writeDataMemoryWord(ClassState, STATE_OFFSET_DESIGN_ENERGY, energyMwh, manageConfig);
}

bool bq27::setTerminateVoltage(uint16_t millivolts, bool manageConfig) {
  if (millivolts < 2500) {
    millivolts = 2500;
  } else if (millivolts > 3700) {
    millivolts = 3700;
  }
  return writeDataMemoryWord(ClassState, STATE_OFFSET_TERMINATE_VOLTAGE, millivolts, manageConfig);
}

bool bq27::setTaperRate(uint16_t rate, bool manageConfig) {
  if (rate > 2000) {
    rate = 2000;
  }
  return writeDataMemoryWord(ClassState, STATE_OFFSET_TAPER_RATE, rate, manageConfig);
}

bool bq27::setSoc1Thresholds(uint8_t setPercent, uint8_t clearPercent, bool manageConfig) {
  uint8_t data[2] = {clampPercent(setPercent), clampPercent(clearPercent)};
  return writeDataMemory(ClassDischarge, DISCHARGE_OFFSET_SOC1_SET, data, 2, manageConfig);
}

bool bq27::setSocfThresholds(uint8_t setPercent, uint8_t clearPercent, bool manageConfig) {
  uint8_t data[2] = {clampPercent(setPercent), clampPercent(clearPercent)};
  return writeDataMemory(ClassDischarge, DISCHARGE_OFFSET_SOCF_SET, data, 2, manageConfig);
}

bool bq27::setSocIntDelta(uint8_t deltaPercent, bool manageConfig) {
  uint8_t value = clampPercent(deltaPercent);
  if (value == 0) {
    value = 1;
  }
  return writeDataMemoryByte(ClassState, STATE_OFFSET_SOC_INT_DELTA, value, manageConfig);
}

bool bq27::opConfig(uint16_t &value) {
  return readWord(ExtOpConfig, value);
}

uint16_t bq27::opConfig() {
  uint16_t value = 0;
  opConfig(value);
  return value;
}

bool bq27::setOpConfig(uint16_t value, bool manageConfig) {
  return writeDataMemoryWord(ClassRegisters, 0, value, manageConfig);
}

bool bq27::setGpoutPolarity(bool activeHigh, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_GPIOPOL, activeHigh, manageConfig);
}

bool bq27::setGpoutMode(GpoutMode mode, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_BATLOWEN, mode == GpoutBatLow, manageConfig);
}

bool bq27::setBatteryInsertionEnable(bool enabled, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_BIE, enabled, manageConfig);
}

bool bq27::setBinPullupEnabled(bool enabled, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_BI_PU_EN, enabled, manageConfig);
}

bool bq27::setSleepEnabled(bool enabled, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_SLEEP, enabled, manageConfig);
}

bool bq27::setRmFccSmoothing(bool enabled, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_RMFCC, enabled, manageConfig);
}

bool bq27::setTemperatureSource(TemperatureSource source, bool manageConfig) {
  return setOpConfigBit(OPCONFIG_TEMPS, source == TemperatureFromHost, manageConfig);
}

bool bq27::selectDataBlock(uint8_t classId, uint8_t blockNumber) {
  uint8_t zero = 0;
  if (!writeBytes(ExtBlockDataControl, &zero, 1)) {
    return false;
  }
  if (!writeBytes(ExtDataClass, &classId, 1)) {
    return false;
  }
  if (!writeBytes(ExtDataBlock, &blockNumber, 1)) {
    return false;
  }
  delay(2);
  return true;
}

bool bq27::readBlock(uint8_t *data) {
  return readBytes(ExtBlockData, data, 32);
}

bool bq27::writeBlock(const uint8_t *data) {
  for (uint8_t i = 0; i < 32; ++i) {
    if (!writeBytes(static_cast<uint8_t>(ExtBlockData + i), &data[i], 1)) {
      return false;
    }
  }
  uint8_t checksum = computeChecksum(data);
  return writeBytes(ExtBlockChecksum, &checksum, 1);
}

uint8_t bq27::computeChecksum(const uint8_t *data) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 32; ++i) {
    sum = static_cast<uint8_t>(sum + data[i]);
  }
  return static_cast<uint8_t>(0xFF - sum);
}

bool bq27::waitForConfigFlag(bool set, uint16_t timeoutMs) {
  uint32_t start = millis();
  do {
    bool flag = (flagsRaw() & FLAG_CFGUPMODE) != 0;
    if (flag == set) {
      return true;
    }
    delay(10);
  } while ((millis() - start) < timeoutMs);
  return false;
}

bool bq27::setOpConfigBit(uint16_t mask, bool enabled, bool manageConfig) {
  uint16_t value = 0;
  bool ok = true;
  bool opened = false;
  if (manageConfig && !_inConfigUpdate) {
    ok = enterConfigUpdate();
    opened = ok;
  }
  if (ok) {
    ok = readDataMemoryWord(ClassRegisters, 0, value, false);
  }
  if (ok) {
    if (enabled) {
      value |= mask;
    } else {
      value &= ~mask;
    }
    ok = setOpConfig(value, false);
  }
  if (opened) {
    ok &= exitConfigUpdate(ExitWithSoftReset);
  }
  return ok;
}

uint8_t bq27::clampPercent(uint8_t percent) {
  return percent > 100 ? 100 : percent;
}
