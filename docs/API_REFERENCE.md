# Batthub API Reference

This page is a compact map of the public API. Normal module users should start with `Batthub`; advanced users can access the chip drivers directly.

## Main Include

```cpp
#include <Batthub.h>
```

## `Batthub`

Use this class for the Batthub PCB as a finished module.

Startup:

- `begin()`
- `begin(TwoWire &wire)`
- `begin(TwoWire &wire, const BeginOptions &options)`
- `beginReport()`
- `beginReport(TwoWire &wire)`
- `beginReport(TwoWire &wire, const BeginOptions &options)`
- `lastBeginReport()`
- `isConnected()`

Optional host pins:

- `pins()`
- `cePinConnected()`
- `otgPinConnected()`
- `intPinConnected()`
- `setCePinEnabled(bool enabled)`
- `cePinEnabled()`
- `setOtgPinEnabled(bool enabled)`
- `otgPinEnabled()`
- `interruptAsserted()`

Common board controls:

- `applySafeDefaults()`
- `applyChargerSettings(const ChargerSettings &settings)`
- `applyGaugeSettings(const GaugeSettings &settings, bool configureBatteryProfile = true)`
- `configureBattery(const BatteryProfile &profile)`
- `readBatteryProfile(BatteryProfile &profile)`
- `configure1s(...)`
- `setChargeEnabled(bool enabled)`
- `setOtgEnabled(bool enabled)`
- `enterShipMode(bool delayTurnOff = false)`
- `resetCharger()`
- `resetGauge()`
- `resetBoth()`

Charger shortcuts:

- `startAdc(bool continuous = true)`
- `stopAdc()`
- `setInputCurrentLimit(uint16_t milliamps)`
- `restoreInputCurrentLimit(uint16_t milliamps)`
- `setInputVoltageLimit(uint16_t millivolts)`
- `setChargeCurrent(uint16_t milliamps)`
- `setChargeVoltage(uint16_t millivolts)`
- `setBoostVoltage(uint16_t millivolts)`
- `setAdapterDetection(bool autoDpdmEnabled, bool hvdcpEnabled, bool maxChargeEnabled, bool icoEnabled)`
- `forceDpdmDetection()`
- `forceIco()`
- `readChargerRegisters(uint8_t *buffer, uint8_t length = bq25::kRegisterCount)`

Readings:

- `stateOfChargePercent()`
- `batteryVoltageMv()`
- `batteryCurrentMa()`
- `systemVoltageMv()`
- `inputVoltageMv()`
- `powerGood()`
- `isCharging()`
- `chargeStateText()`
- `inputTypeText()`
- `chargeFaultText()`
- `ntcFaultText()`
- `snapshot()`

Low-level access stays public:

```cpp
batthub.charger.setWatchdogTimer(bq25::WatchdogOff);
batthub.gauge.setGpoutMode(bq27::GpoutSocInt);
```

## `bq25`

Driver for the TI BQ25895 charger, USB input limiter, power-path, and OTG boost controller.

Core I2C:

- `begin(...)`
- `isConnected()`
- `address()`
- `lastError()`
- `readRegister(...)`
- `writeRegister(...)`
- `updateRegister(...)`
- `readRegisters(...)`

Configuration:

- `applyConfig(const Config &config)`
- `setHighImpedanceMode(bool enabled)`
- `setIlimPinEnabled(bool enabled)`
- `setInputCurrentLimit(uint16_t milliamps)`
- `setInputVoltageLimit(uint16_t millivolts)`
- `setChargeCurrent(uint16_t milliamps)`
- `setPrechargeCurrent(uint16_t milliamps)`
- `setTerminationCurrent(uint16_t milliamps)`
- `setChargeVoltage(uint16_t millivolts)`
- `setMinSystemVoltage(uint16_t millivolts)`
- `setBoostVoltage(uint16_t millivolts)`
- `setWatchdogTimer(WatchdogTimer timer)`
- `setSafetyTimer(SafetyTimer timer)`
- `setThermalRegulation(ThermalRegulation temperature)`

Feature controls:

- `setChargingEnabled(bool enabled)`
- `setOtgEnabled(bool enabled)`
- `startAdc(bool continuous = false)`
- `stopAdc()`
- `setAdcContinuous(bool enabled)`
- `setAutoDpdmEnabled(bool enabled)`
- `setHvdcpEnabled(bool enabled)`
- `setMaxChargeEnabled(bool enabled)`
- `setIcoEnabled(bool enabled)`
- `forceDpdmDetection()`
- `forceIco()`
- `setPumpXEnabled(bool enabled)`
- `pumpXIncrease()`
- `pumpXDecrease()`
- `enterShipMode(bool delayTurnOff = false)`
- `resetRegisters()`
- `resetWatchdog()`

Readings:

- `status()`
- `faults()`
- `measurements()`
- `deviceInfo()`
- `adcBatteryVoltage()`
- `adcSystemVoltage()`
- `adcVbusVoltage()`
- `adcChargeCurrent()`
- `adcTsPercentX100()`
- `effectiveInputCurrentLimit()`
- `isChargeDone()`
- `isPowerGood()`
- `isInVindpm()`
- `isInIindpm()`

Text helpers:

- `vbusTypeName(...)`
- `chargeStateName(...)`
- `chargeFaultName(...)`
- `ntcFaultName(...)`

## `bq27`

Driver for the TI BQ27441-G1 fuel gauge.

Core I2C:

- `begin(...)`
- `isConnected()`
- `address()`
- `lastError()`
- `readBytes(...)`
- `writeBytes(...)`
- `readWord(...)`
- `writeWord(...)`

Control commands:

- `deviceType()`
- `firmwareVersion()`
- `chemistryId()`
- `controlStatus()`
- `controlStatusBits()`
- `unseal(...)`
- `seal()`
- `sealed()`
- `softReset()`
- `reset()`
- `batteryInsert()`
- `batteryRemove()`
- `setHibernate()`
- `clearHibernate()`
- `shutdownEnable()`
- `shutdown()`
- `pulseGpout()`

Readings:

- `flagsRaw()`
- `flags()`
- `voltage()`
- `averageCurrent()`
- `standbyCurrent()`
- `maxLoadCurrent()`
- `averagePower()`
- `stateOfCharge(bool unfiltered = false)`
- `stateOfHealthPercent()`
- `stateOfHealthStatus()`
- `capacity(CapacityKind kind = CapacityRemaining)`
- `temperatureDeciKelvin(bool internal = false)`
- `temperatureCelsiusX10(bool internal = false)`
- `snapshot()`

Battery and data-memory configuration:

- `enterConfigUpdate(...)`
- `exitConfigUpdate(...)`
- `readDataMemory(...)`
- `writeDataMemory(...)`
- `readDataMemoryByte(...)`
- `writeDataMemoryByte(...)`
- `readDataMemoryWord(...)`
- `writeDataMemoryWord(...)`
- `configureBattery(const BatteryConfig &config)`
- `setDesignCapacity(...)`
- `setDesignEnergy(...)`
- `setTerminateVoltage(...)`
- `setTaperRate(...)`
- `setSoc1Thresholds(...)`
- `setSocfThresholds(...)`
- `setSocIntDelta(...)`

GPOUT and option configuration:

- `opConfig(...)`
- `setOpConfig(...)`
- `setGpoutPolarity(...)`
- `setGpoutMode(...)`
- `setBatteryInsertionEnable(...)`
- `setBinPullupEnabled(...)`
- `setSleepEnabled(...)`
- `setRmFccSmoothing(...)`
- `setTemperatureSource(...)`

## Compatibility Include

Older examples can still include:

```cpp
#include <bq25_bq27.h>
```

The alias header `bq25bq27.h` includes the same combined driver.
