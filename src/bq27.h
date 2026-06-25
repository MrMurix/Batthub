#ifndef BQ27_H
#define BQ27_H

#include <Arduino.h>
#include <Wire.h>

class bq27 {
public:
  static const uint8_t kDefaultAddress = 0x55;
  static const uint16_t kDeviceId = 0x0421;
  static const uint16_t kUnsealKey = 0x8000;
  static const uint16_t kDefaultConfigTimeoutMs = 1000;

  enum Command : uint8_t {
    CmdControl = 0x00,
    CmdTemperature = 0x02,
    CmdVoltage = 0x04,
    CmdFlags = 0x06,
    CmdNominalAvailableCapacity = 0x08,
    CmdFullAvailableCapacity = 0x0A,
    CmdRemainingCapacity = 0x0C,
    CmdFullChargeCapacity = 0x0E,
    CmdAverageCurrent = 0x10,
    CmdStandbyCurrent = 0x12,
    CmdMaxLoadCurrent = 0x14,
    CmdAveragePower = 0x18,
    CmdStateOfCharge = 0x1C,
    CmdInternalTemperature = 0x1E,
    CmdStateOfHealth = 0x20,
    CmdRemainingCapacityUnfiltered = 0x28,
    CmdRemainingCapacityFiltered = 0x2A,
    CmdFullChargeCapacityUnfiltered = 0x2C,
    CmdFullChargeCapacityFiltered = 0x2E,
    CmdStateOfChargeUnfiltered = 0x30
  };

  enum ExtendedCommand : uint8_t {
    ExtOpConfig = 0x3A,
    ExtDesignCapacity = 0x3C,
    ExtDataClass = 0x3E,
    ExtDataBlock = 0x3F,
    ExtBlockData = 0x40,
    ExtBlockChecksum = 0x60,
    ExtBlockDataControl = 0x61
  };

  enum ControlSubcommand : uint16_t {
    ControlStatus = 0x0000,
    ControlDeviceType = 0x0001,
    ControlFirmwareVersion = 0x0002,
    ControlDmCode = 0x0004,
    ControlPreviousMacWrite = 0x0007,
    ControlChemId = 0x0008,
    ControlBatteryInsert = 0x000C,
    ControlBatteryRemove = 0x000D,
    ControlSetHibernate = 0x0011,
    ControlClearHibernate = 0x0012,
    ControlSetConfigUpdate = 0x0013,
    ControlShutdownEnable = 0x001B,
    ControlShutdown = 0x001C,
    ControlSeal = 0x0020,
    ControlPulseSocInt = 0x0023,
    ControlReset = 0x0041,
    ControlSoftReset = 0x0042,
    ControlExitConfigUpdate = 0x0043,
    ControlExitResim = 0x0044
  };

  enum DataClass : uint8_t {
    ClassSafety = 2,
    ClassChargeTermination = 36,
    ClassData = 48,
    ClassDischarge = 49,
    ClassRegisters = 64,
    ClassPower = 68,
    ClassItConfig = 80,
    ClassCurrentThresholds = 81,
    ClassState = 82,
    ClassRaRam = 89,
    ClassCalibrationData = 104,
    ClassCcCalibration = 105,
    ClassCurrent = 107,
    ClassSecurity = 112
  };

  enum ExitConfigMode : uint8_t {
    ExitWithoutResim = 0,
    ExitWithResim = 1,
    ExitWithSoftReset = 2
  };

  enum TemperatureSource : uint8_t {
    TemperatureFromInternal = 0,
    TemperatureFromHost = 1
  };

  enum GpoutMode : uint8_t {
    GpoutBatLow = 0,
    GpoutSocInt = 1
  };

  enum CapacityKind : uint8_t {
    CapacityNominalAvailable,
    CapacityFullAvailable,
    CapacityRemaining,
    CapacityFullCharge,
    CapacityRemainingUnfiltered,
    CapacityRemainingFiltered,
    CapacityFullChargeUnfiltered,
    CapacityFullChargeFiltered,
    CapacityDesign
  };

  struct Flags {
    bool overTemperature;
    bool underTemperature;
    bool fullCharge;
    bool charging;
    bool ocvTaken;
    bool powerOnReset;
    bool configUpdateMode;
    bool batteryDetected;
    bool soc1;
    bool socFinal;
    bool discharging;
  };

  struct ControlStatusBits {
    bool shutdownEnabled;
    bool watchdogReset;
    bool sealed;
    bool calibrationMode;
    bool cca;
    bool bca;
    bool qmaxUpdated;
    bool resistanceUpdated;
    bool initComplete;
    bool hibernate;
    bool sleep;
    bool loadMode;
    bool resistanceUpdatesDisabled;
    bool voltageOk;
  };

  struct Snapshot {
    uint16_t voltageMv;
    int16_t averageCurrentMa;
    int16_t standbyCurrentMa;
    int16_t maxLoadCurrentMa;
    int16_t averagePowerMw;
    uint16_t stateOfChargePercent;
    uint16_t stateOfChargeUnfilteredPercent;
    uint16_t remainingCapacityMah;
    uint16_t fullChargeCapacityMah;
    uint8_t stateOfHealthPercent;
    uint8_t stateOfHealthStatus;
    int16_t temperatureCelsiusX10;
    int16_t internalTemperatureCelsiusX10;
    Flags flags;
  };

  struct BatteryConfig {
    uint16_t designCapacityMah = 1000;
    uint16_t designEnergyMwh = 3700;
    uint16_t terminateVoltageMv = 3200;
    uint16_t taperRate = 100;
    uint8_t soc1SetPercent = 10;
    uint8_t soc1ClearPercent = 15;
    uint8_t socfSetPercent = 5;
    uint8_t socfClearPercent = 10;
    uint8_t socIntDeltaPercent = 1;
  };

  bq27();

  bool begin(TwoWire &wire = Wire, uint8_t address = kDefaultAddress, bool startWire = true);
  bool isConnected();
  uint8_t address() const;
  uint8_t lastError() const;

  bool readBytes(uint8_t subAddress, uint8_t *dest, uint8_t count);
  bool writeBytes(uint8_t subAddress, const uint8_t *src, uint8_t count);
  bool readWord(uint8_t command, uint16_t &value);
  uint16_t readWord(uint8_t command);
  bool writeWord(uint8_t command, uint16_t value);

  bool executeControl(uint16_t subcommand);
  bool readControl(uint16_t subcommand, uint16_t &value);
  uint16_t readControl(uint16_t subcommand);

  uint16_t deviceType();
  uint16_t firmwareVersion();
  uint16_t chemistryId();
  uint16_t controlStatus();
  ControlStatusBits controlStatusBits();

  bool unseal(uint16_t key = kUnsealKey);
  bool seal();
  bool sealed();
  bool softReset();
  bool reset();
  bool batteryInsert();
  bool batteryRemove();
  bool setHibernate();
  bool clearHibernate();
  bool shutdownEnable();
  bool shutdown();
  bool pulseGpout();

  uint16_t flagsRaw();
  Flags flags();
  bool enterConfigUpdate(uint16_t timeoutMs = kDefaultConfigTimeoutMs);
  bool exitConfigUpdate(ExitConfigMode mode = ExitWithSoftReset, uint16_t timeoutMs = kDefaultConfigTimeoutMs);

  uint16_t voltage();
  int16_t averageCurrent();
  int16_t standbyCurrent();
  int16_t maxLoadCurrent();
  int16_t averagePower();
  uint16_t stateOfCharge(bool unfiltered = false);
  uint8_t stateOfHealthPercent();
  uint8_t stateOfHealthStatus();
  uint16_t capacity(CapacityKind kind = CapacityRemaining);
  uint16_t temperatureDeciKelvin(bool internal = false);
  int16_t temperatureCelsiusX10(bool internal = false);
  Snapshot snapshot();

  bool readDataMemory(uint8_t classId, uint8_t offset, uint8_t *data, uint8_t length, bool manageConfig = true);
  bool writeDataMemory(uint8_t classId, uint8_t offset, const uint8_t *data, uint8_t length, bool manageConfig = true);
  uint8_t readDataMemoryByte(uint8_t classId, uint8_t offset, bool manageConfig = true);
  bool writeDataMemoryByte(uint8_t classId, uint8_t offset, uint8_t value, bool manageConfig = true);
  bool readDataMemoryWord(uint8_t classId, uint8_t offset, uint16_t &value, bool manageConfig = true);
  bool writeDataMemoryWord(uint8_t classId, uint8_t offset, uint16_t value, bool manageConfig = true);

  bool configureBattery(const BatteryConfig &config);
  bool setDesignCapacity(uint16_t capacityMah, bool manageConfig = true);
  bool setDesignEnergy(uint16_t energyMwh, bool manageConfig = true);
  bool setTerminateVoltage(uint16_t millivolts, bool manageConfig = true);
  bool setTaperRate(uint16_t rate, bool manageConfig = true);
  bool setSoc1Thresholds(uint8_t setPercent, uint8_t clearPercent, bool manageConfig = true);
  bool setSocfThresholds(uint8_t setPercent, uint8_t clearPercent, bool manageConfig = true);
  bool setSocIntDelta(uint8_t deltaPercent, bool manageConfig = true);

  bool opConfig(uint16_t &value);
  uint16_t opConfig();
  bool setOpConfig(uint16_t value, bool manageConfig = true);
  bool setGpoutPolarity(bool activeHigh, bool manageConfig = true);
  bool setGpoutMode(GpoutMode mode, bool manageConfig = true);
  bool setBatteryInsertionEnable(bool enabled, bool manageConfig = true);
  bool setBinPullupEnabled(bool enabled, bool manageConfig = true);
  bool setSleepEnabled(bool enabled, bool manageConfig = true);
  bool setRmFccSmoothing(bool enabled, bool manageConfig = true);
  bool setTemperatureSource(TemperatureSource source, bool manageConfig = true);

private:
  TwoWire *_wire;
  uint8_t _address;
  uint8_t _lastError;
  bool _wasSealed;
  bool _inConfigUpdate;

  bool selectDataBlock(uint8_t classId, uint8_t blockNumber);
  bool readBlock(uint8_t *data);
  bool writeBlock(const uint8_t *data);
  uint8_t computeChecksum(const uint8_t *data);
  bool waitForConfigFlag(bool set, uint16_t timeoutMs);
  bool setOpConfigBit(uint16_t mask, bool enabled, bool manageConfig);
  static uint8_t clampPercent(uint8_t percent);
};

#endif
