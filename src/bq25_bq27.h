#ifndef BQ25_BQ27_H
#define BQ25_BQ27_H

#include "bq25.h"
#include "bq27.h"

class bq25bq27 {
public:
  struct BeginOptions {
    uint8_t bq25Address = bq25::kDefaultAddress;
    uint8_t bq27Address = bq27::kDefaultAddress;
    bool startWire = true;
  };

  struct ModuleConfig {
    bq25::Config charger;
    bq27::BatteryConfig gauge;
  };

  struct Snapshot {
    bq25::Status chargerStatus;
    bq25::Faults chargerFaults;
    bq25::Measurements chargerMeasurements;
    bq25::DeviceInfo chargerInfo;
    bq27::Snapshot gauge;
  };

  struct BeginReport {
    bool chargerPresent;
    bool gaugePresent;
    uint8_t chargerI2cError;
    uint8_t gaugeI2cError;
    uint8_t chargerPartNumber;
    uint8_t chargerRevision;
    uint16_t gaugeDeviceType;

    bool ok() const {
      return chargerPresent && gaugePresent;
    }
  };

  bq25 charger;
  bq27 gauge;

  bq25bq27();

  bool begin(TwoWire &wire = Wire);
  bool begin(TwoWire &wire, const BeginOptions &options);
  BeginReport beginReport(TwoWire &wire = Wire);
  BeginReport beginReport(TwoWire &wire, const BeginOptions &options);
  bool isConnected();
  BeginReport lastBeginReport() const;
  bool applyConfig(const ModuleConfig &config);
  bool configure1s(uint16_t capacityMah,
                   uint16_t chargeCurrentMa = 1000,
                   uint16_t inputCurrentLimitMa = 500,
                   uint16_t chargeVoltageMv = 4208,
                   uint16_t terminateVoltageMv = 3200);
  Snapshot snapshot();

private:
  bool _chargerOk;
  bool _gaugeOk;
  BeginReport _lastBeginReport;
};

#endif
