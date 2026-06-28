#ifndef BATTHUB_H
#define BATTHUB_H

#include <Arduino.h>
#include <Wire.h>

#include "bq25.h"
#include "bq27.h"
#include "bq25_bq27.h"

class Batthub {
public:
  static const int8_t PinNotConnected = -1;

  struct Pins {
    int8_t ce = PinNotConnected;
    int8_t otg = PinNotConnected;
    int8_t intPin = PinNotConnected;
    bool ceActiveLow = true;
    bool otgActiveHigh = true;
    bool intActiveLow = true;
  };

  struct BeginOptions {
    Pins pins;
    uint8_t chargerAddress = bq25::kDefaultAddress;
    uint8_t gaugeAddress = bq27::kDefaultAddress;
    uint32_t i2cClockHz = 100000;
    bool startWire = true;
    bool setupPins = true;
    bool initialCeEnabled = true;
    bool initialOtgEnabled = false;
  };

  struct ChargerSettings {
    bool highImpedanceMode = false;
    bool ilimPinEnabled = true;
    uint16_t inputCurrentLimitMa = 500;
    uint16_t inputVoltageLimitMv = 4400;
    uint16_t vindpmOffsetMv = 0;
    uint16_t chargeCurrentMa = 1000;
    uint16_t prechargeCurrentMa = 128;
    uint16_t terminationCurrentMa = 128;
    uint16_t chargeVoltageMv = 4208;
    uint16_t minSystemVoltageMv = 3500;
    uint16_t boostVoltageMv = 5126;
    uint16_t irCompResistanceMohm = 0;
    uint16_t irCompVoltageClampMv = 0;
    bq25::WatchdogTimer watchdog = bq25::WatchdogOff;
    bq25::SafetyTimer safetyTimer = bq25::SafetyTimer12h;
    bq25::ThermalRegulation thermalRegulation = bq25::Thermal100C;
    bq25::BoostFrequency boostFrequency = bq25::Boost1500kHz;
    bq25::BoostHotThreshold boostHotThreshold = bq25::BoostHot3475;
    bq25::BoostColdThreshold boostColdThreshold = bq25::BoostCold77;
    bq25::BatteryLowThreshold batteryLowThreshold = bq25::BatteryLow3000mV;
    bq25::RechargeThreshold rechargeThreshold = bq25::Recharge100mV;
    bool chargingEnabled = true;
    bool otgEnabled = false;
    bool batteryLoadEnabled = false;
    bool pumpXEnabled = false;
    bool terminationEnabled = true;
    bool safetyTimerEnabled = true;
    bool safetyTimerSlowedInDpm = true;
    bool icoEnabled = false;
    bool autoDpdmEnabled = false;
    bool hvdcpEnabled = true;
    bool maxChargeEnabled = true;
    bool adcContinuous = true;
    bool absoluteVindpm = false;
    bool batfetResetEnabled = true;
  };

  struct BatteryProfile {
    uint16_t designCapacityMah = 2500;
    uint16_t designEnergyMwh = 9250;
    uint16_t terminateVoltageMv = 3200;
    uint16_t taperRate = 250;
    uint8_t soc1SetPercent = 10;
    uint8_t soc1ClearPercent = 15;
    uint8_t socfSetPercent = 5;
    uint8_t socfClearPercent = 10;
    uint8_t socIntDeltaPercent = 1;
  };

  struct GaugeSettings {
    BatteryProfile batteryProfile;
    bq27::GpoutMode gpoutMode = bq27::GpoutBatLow;
    bq27::TemperatureSource temperatureSource = bq27::TemperatureFromInternal;
    bool gpoutActiveHigh = false;
    bool batteryInsertionEnabled = true;
    bool binPullupEnabled = true;
    bool sleepEnabled = true;
    bool rmFccSmoothingEnabled = true;
  };

  struct Snapshot {
    bq25::Status chargerStatus;
    bq25::Faults chargerFaults;
    bq25::Measurements chargerMeasurements;
    bq25::DeviceInfo chargerInfo;
    bq27::Snapshot gauge;
    bool cePinEnabled;
    bool otgPinEnabled;
    bool interruptAsserted;
  };

  using BeginReport = bq25bq27::BeginReport;

  bq25 charger;
  bq27 gauge;

  Batthub();

  bool begin();
  bool begin(TwoWire &wire);
  bool begin(TwoWire &wire, const BeginOptions &options);
  BeginReport beginReport();
  BeginReport beginReport(TwoWire &wire);
  BeginReport beginReport(TwoWire &wire, const BeginOptions &options);
  BeginReport lastBeginReport() const;
  bool isConnected();

  const Pins &pins() const;
  bool cePinConnected() const;
  bool otgPinConnected() const;
  bool intPinConnected() const;

  bool setCePinEnabled(bool enabled);
  bool cePinEnabled() const;
  bool setOtgPinEnabled(bool enabled);
  bool otgPinEnabled() const;
  bool interruptAsserted() const;

  bool setChargeEnabled(bool enabled);
  bool setOtgEnabled(bool enabled);
  bool enterShipMode(bool delayTurnOff = false);

  bool applySafeDefaults();
  bool applyChargerSettings(const ChargerSettings &settings);
  bool applyGaugeSettings(const GaugeSettings &settings, bool configureBatteryProfile = true);
  bool configureBattery(const BatteryProfile &profile);
  bool readBatteryProfile(BatteryProfile &profile);
  bool configure1s(uint16_t capacityMah,
                   uint16_t chargeCurrentMa = 1000,
                   uint16_t inputCurrentLimitMa = 500,
                   uint16_t chargeVoltageMv = 4208,
                   uint16_t terminateVoltageMv = 3200,
                   bool configureGauge = true);

  bool startAdc(bool continuous = true);
  bool stopAdc();
  bool setInputCurrentLimit(uint16_t milliamps);
  bool restoreInputCurrentLimit(uint16_t milliamps);
  bool setInputVoltageLimit(uint16_t millivolts);
  bool setChargeCurrent(uint16_t milliamps);
  bool setChargeVoltage(uint16_t millivolts);
  bool setBoostVoltage(uint16_t millivolts);
  bool setAdapterDetection(bool autoDpdmEnabled,
                           bool hvdcpEnabled,
                           bool maxChargeEnabled,
                           bool icoEnabled);
  bool forceDpdmDetection();
  bool forceIco();
  bool resetCharger();
  bool resetGauge();
  bool resetBoth();
  bool readChargerRegisters(uint8_t *buffer, uint8_t length = bq25::kRegisterCount);

  uint16_t stateOfChargePercent();
  uint16_t batteryVoltageMv();
  int16_t batteryCurrentMa();
  uint16_t systemVoltageMv();
  uint16_t inputVoltageMv();
  bool powerGood();
  bool isCharging();
  const char *chargeStateText();
  const char *inputTypeText();
  const char *chargeFaultText();
  const char *ntcFaultText();

  Snapshot snapshot();

  static bq25::Config toChargerConfig(const ChargerSettings &settings);
  static bq27::BatteryConfig toBatteryConfig(const BatteryProfile &profile);

private:
  TwoWire *_wire;
  Pins _pins;
  BeginOptions _options;
  BeginReport _lastBeginReport;

  static bool pinDefined(int8_t pin);
  void setupBoardPins(bool initialCeEnabled, bool initialOtgEnabled);
  void beginWire(TwoWire &wire, const BeginOptions &options);
  bool setPinActive(int8_t pin, bool activeHigh, bool active);
  bool pinActive(int8_t pin, bool activeHigh) const;
};

#endif
