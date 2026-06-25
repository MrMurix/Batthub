#include <Wire.h>
#include <bq27.h>

/*
  BQ27441 Basic Fuel Gauge Example

  Use this when the gauge chip is connected by itself or when you only want
  fuel-gauge readings during early firmware bring-up.

  The BQ27441 needs battery parameters. These defaults are only examples for
  a 2500 mAh 1S Li-ion/LiPo cell. Change them for the exact cell you sell or
  document with your module.
*/

const uint16_t DESIGN_CAPACITY_MAH = 2500;
const uint16_t DESIGN_ENERGY_MWH = 9250;
const uint16_t TERMINATE_VOLTAGE_MV = 3200;
const uint16_t TAPER_RATE = 250;

bq27 gauge;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  if (!gauge.begin(Wire)) {
    Serial.println("BQ27441 not found.");
    while (true) {
      delay(1000);
    }
  }

  bq27::BatteryConfig cfg;
  cfg.designCapacityMah = DESIGN_CAPACITY_MAH;
  cfg.designEnergyMwh = DESIGN_ENERGY_MWH;
  cfg.terminateVoltageMv = TERMINATE_VOLTAGE_MV;
  cfg.taperRate = TAPER_RATE;
  gauge.configureBattery(cfg);
}

void loop() {
  bq27::Snapshot s = gauge.snapshot();

  Serial.print("SOC=");
  Serial.print(s.stateOfChargePercent);
  Serial.print("% V=");
  Serial.print(s.voltageMv);
  Serial.print("mV I=");
  Serial.print(s.averageCurrentMa);
  Serial.print("mA Rem=");
  Serial.print(s.remainingCapacityMah);
  Serial.print("mAh Full=");
  Serial.print(s.fullChargeCapacityMah);
  Serial.print("mAh SOH=");
  Serial.print(s.stateOfHealthPercent);
  Serial.println("%");

  delay(1000);
}
