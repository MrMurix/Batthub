#ifndef BQ25_H
#define BQ25_H

#include <Arduino.h>
#include <Wire.h>

class bq25 {
public:
  static const uint8_t kDefaultAddress = 0x6A;
  static const uint8_t kFirstRegister = 0x00;
  static const uint8_t kLastRegister = 0x14;
  static const uint8_t kRegisterCount = 0x15;

  enum Register : uint8_t {
    RegInputCurrent = 0x00,
    RegVindpmOffset = 0x01,
    RegAdcAndDpdm = 0x02,
    RegPowerPath = 0x03,
    RegChargeCurrent = 0x04,
    RegPrechargeTermination = 0x05,
    RegChargeVoltage = 0x06,
    RegChargeControl = 0x07,
    RegIrCompThermal = 0x08,
    RegMiscControl = 0x09,
    RegBoostVoltage = 0x0A,
    RegSystemStatus = 0x0B,
    RegFault = 0x0C,
    RegVindpm = 0x0D,
    RegBatteryVoltageAdc = 0x0E,
    RegSystemVoltageAdc = 0x0F,
    RegTsAdc = 0x10,
    RegVbusVoltageAdc = 0x11,
    RegChargeCurrentAdc = 0x12,
    RegDpmStatus = 0x13,
    RegPartInfo = 0x14
  };

  enum VbusType : uint8_t {
    VbusNone = 0,
    VbusUsbSdp = 1,
    VbusUsbCdp = 2,
    VbusUsbDcp = 3,
    VbusMaxCharge = 4,
    VbusUnknown = 5,
    VbusNonStandard = 6,
    VbusOtg = 7
  };

  enum ChargeState : uint8_t {
    ChargeNotCharging = 0,
    ChargePrecharge = 1,
    ChargeFastCharge = 2,
    ChargeDone = 3
  };

  enum ChargeFault : uint8_t {
    ChargeFaultNormal = 0,
    ChargeFaultInput = 1,
    ChargeFaultThermalShutdown = 2,
    ChargeFaultSafetyTimer = 3
  };

  enum NtcFault : uint8_t {
    NtcNormal = 0,
    NtcBuckCold = 1,
    NtcBuckHot = 2,
    NtcBoostCold = 5,
    NtcBoostHot = 6
  };

  enum WatchdogTimer : uint8_t {
    WatchdogOff = 0,
    Watchdog40s = 1,
    Watchdog80s = 2,
    Watchdog160s = 3
  };

  enum SafetyTimer : uint8_t {
    SafetyTimer5h = 0,
    SafetyTimer8h = 1,
    SafetyTimer12h = 2,
    SafetyTimer20h = 3
  };

  enum BoostFrequency : uint8_t {
    Boost1500kHz = 0,
    Boost500kHz = 1
  };

  enum BoostHotThreshold : uint8_t {
    BoostHot3475 = 0,
    BoostHot3775 = 1,
    BoostHot3125 = 2,
    BoostHotDisabled = 3
  };

  enum BoostColdThreshold : uint8_t {
    BoostCold77 = 0,
    BoostCold80 = 1
  };

  enum BatteryLowThreshold : uint8_t {
    BatteryLow2800mV = 0,
    BatteryLow3000mV = 1
  };

  enum RechargeThreshold : uint8_t {
    Recharge100mV = 0,
    Recharge200mV = 1
  };

  enum ThermalRegulation : uint8_t {
    Thermal60C = 0,
    Thermal80C = 1,
    Thermal100C = 2,
    Thermal120C = 3
  };

  struct Status {
    VbusType vbusType;
    ChargeState chargeState;
    bool powerGood;
    bool usbInput;
    bool systemInRegulation;
  };

  struct Faults {
    bool watchdog;
    bool boost;
    ChargeFault charge;
    bool battery;
    NtcFault ntc;
  };

  struct Measurements {
    bool thermalRegulation;
    bool vbusGood;
    bool vindpm;
    bool iindpm;
    uint16_t batteryMv;
    uint16_t systemMv;
    uint16_t vbusMv;
    uint16_t chargeCurrentMa;
    uint16_t effectiveInputCurrentLimitMa;
    uint16_t tsPercentX100;
  };

  struct DeviceInfo {
    bool icoOptimized;
    uint8_t partNumber;
    bool coldHotTemperatureProfile;
    uint8_t revision;

    bool isBQ25895() const {
      return partNumber == 0x07;
    }
  };

  struct Config {
    uint16_t inputCurrentLimitMa = 500;
    uint16_t inputVoltageLimitMv = 4400;
    uint16_t chargeCurrentMa = 1000;
    uint16_t prechargeCurrentMa = 128;
    uint16_t terminationCurrentMa = 128;
    uint16_t chargeVoltageMv = 4208;
    uint16_t minSystemVoltageMv = 3500;
    uint16_t boostVoltageMv = 5126;
    WatchdogTimer watchdog = WatchdogOff;
    SafetyTimer safetyTimer = SafetyTimer12h;
    ThermalRegulation thermalRegulation = Thermal100C;
    bool enableCharging = true;
    bool enableTermination = true;
    bool enableSafetyTimer = true;
    bool enableIlimPin = true;
    bool enableIco = true;
    bool enableAutoDpdm = true;
    bool enableHvdcp = true;
    bool enableMaxCharge = true;
  };

  bq25();

  bool begin(TwoWire &wire = Wire, uint8_t address = kDefaultAddress, bool startWire = true);
  bool isConnected();
  uint8_t address() const;
  uint8_t lastError() const;

  bool readRegister(uint8_t reg, uint8_t &value);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool updateRegister(uint8_t reg, uint8_t mask, uint8_t value);
  bool readRegisters(uint8_t *buffer, uint8_t length, uint8_t startRegister = kFirstRegister);

  bool resetRegisters();
  bool resetWatchdog();
  bool applyConfig(const Config &config);

  bool setHighImpedanceMode(bool enabled);
  bool setIlimPinEnabled(bool enabled);
  bool setInputCurrentLimit(uint16_t milliamps);
  uint16_t inputCurrentLimit();

  bool setBoostHotThreshold(BoostHotThreshold threshold);
  bool setBoostColdThreshold(BoostColdThreshold threshold);
  bool setVindpmOffset(uint16_t millivolts);

  bool startAdc(bool continuous = false);
  bool stopAdc();
  bool setAdcContinuous(bool enabled);
  bool setBoostFrequency(BoostFrequency frequency);
  bool setIcoEnabled(bool enabled);
  bool setHvdcpEnabled(bool enabled);
  bool setMaxChargeEnabled(bool enabled);
  bool forceDpdmDetection();
  bool setAutoDpdmEnabled(bool enabled);

  bool setBatteryLoadEnabled(bool enabled);
  bool setOtgEnabled(bool enabled);
  bool setChargingEnabled(bool enabled);
  bool setMinSystemVoltage(uint16_t millivolts);

  bool setPumpXEnabled(bool enabled);
  bool setChargeCurrent(uint16_t milliamps);
  bool setPrechargeCurrent(uint16_t milliamps);
  bool setTerminationCurrent(uint16_t milliamps);
  bool setChargeVoltage(uint16_t millivolts);
  bool setBatteryLowThreshold(BatteryLowThreshold threshold);
  bool setRechargeThreshold(RechargeThreshold threshold);

  bool setTerminationEnabled(bool enabled);
  bool setWatchdogTimer(WatchdogTimer timer);
  bool setSafetyTimerEnabled(bool enabled);
  bool setSafetyTimer(SafetyTimer timer);

  bool setIrCompResistance(uint16_t milliohms);
  bool setIrCompVoltageClamp(uint16_t millivolts);
  bool setThermalRegulation(ThermalRegulation temperature);

  bool forceIco();
  bool setSafetyTimerSlowedInDpm(bool enabled);
  bool enterShipMode(bool delayTurnOff = false);
  bool setBatfetResetEnabled(bool enabled);
  bool pumpXIncrease();
  bool pumpXDecrease();

  bool setBoostVoltage(uint16_t millivolts);
  bool setAbsoluteVindpm(bool enabled);
  bool setInputVoltageLimit(uint16_t millivolts);

  Status status();
  Faults faults();
  Measurements measurements();
  DeviceInfo deviceInfo();

  uint16_t adcBatteryVoltage();
  uint16_t adcSystemVoltage();
  uint16_t adcVbusVoltage();
  uint16_t adcChargeCurrent();
  uint16_t adcTsPercentX100();
  uint16_t effectiveInputCurrentLimit();
  bool isChargeDone();
  bool isPowerGood();
  bool isInVindpm();
  bool isInIindpm();

  static const char *vbusTypeName(VbusType type);
  static const char *chargeStateName(ChargeState state);
  static const char *chargeFaultName(ChargeFault fault);
  static const char *ntcFaultName(NtcFault fault);

private:
  TwoWire *_wire;
  uint8_t _address;
  uint8_t _lastError;

  bool writeField(uint8_t reg, uint8_t mask, uint8_t shift, uint8_t code);
  uint8_t readField(uint8_t reg, uint8_t mask, uint8_t shift);
  static uint8_t encodeLinear(uint16_t value, uint16_t base, uint16_t step, uint8_t maxCode);
  static uint16_t decodeLinear(uint8_t code, uint16_t base, uint16_t step);
};

#endif
