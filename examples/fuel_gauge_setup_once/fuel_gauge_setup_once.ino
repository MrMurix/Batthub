#include <Wire.h>
#include <bq27.h>

/*
  Fuel Gauge Setup Once

  The BQ27441 is not just a voltage sensor. It needs battery parameters.
  Run this sketch when you want to program the gauge for a specific cell.

  Serial commands:
    h  help
    r  read live gauge values
    c  configure the battery parameters below
    p  pulse GPOUT/SOC_INT
    s  seal the gauge
    u  unseal the gauge

  Important:
  - The default BQ27441-G1A profile is intended for 4.2 V Li-ion/LiPo cells.
  - Set DESIGN_CAPACITY_MAH and DESIGN_ENERGY_MWH for your real battery.
  - Full SOC accuracy still requires real charge/discharge validation.
*/

const uint16_t DESIGN_CAPACITY_MAH = 2500;
const uint16_t DESIGN_ENERGY_MWH = 9250;
const uint16_t TERMINATE_VOLTAGE_MV = 3200;
const uint16_t TAPER_RATE = 250;

bq27 gauge;

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h help");
  Serial.println("  r read values");
  Serial.println("  c configure battery data memory");
  Serial.println("  p pulse GPOUT / SOC_INT");
  Serial.println("  s seal gauge");
  Serial.println("  u unseal gauge");
}

void printGauge() {
  Serial.print("Device type: 0x");
  Serial.println(gauge.deviceType(), HEX);
  Serial.print("Chemistry ID: 0x");
  Serial.println(gauge.chemistryId(), HEX);
  Serial.print("Voltage: ");
  Serial.print(gauge.voltage());
  Serial.println(" mV");
  Serial.print("SOC: ");
  Serial.print(gauge.stateOfCharge());
  Serial.println(" %");
  Serial.print("Average current: ");
  Serial.print(gauge.averageCurrent());
  Serial.println(" mA");
  Serial.print("Remaining capacity: ");
  Serial.print(gauge.capacity(bq27::CapacityRemaining));
  Serial.println(" mAh");
  Serial.print("Full charge capacity: ");
  Serial.print(gauge.capacity(bq27::CapacityFullCharge));
  Serial.println(" mAh");
  Serial.print("Flags: 0x");
  Serial.println(gauge.flagsRaw(), HEX);
}

void configureGauge() {
  bq27::BatteryConfig cfg;
  cfg.designCapacityMah = DESIGN_CAPACITY_MAH;
  cfg.designEnergyMwh = DESIGN_ENERGY_MWH;
  cfg.terminateVoltageMv = TERMINATE_VOLTAGE_MV;
  cfg.taperRate = TAPER_RATE;
  cfg.soc1SetPercent = 10;
  cfg.soc1ClearPercent = 15;
  cfg.socfSetPercent = 5;
  cfg.socfClearPercent = 10;
  cfg.socIntDeltaPercent = 1;

  bool ok = gauge.configureBattery(cfg);
  Serial.println(ok ? "Battery configuration written." : "Battery configuration failed.");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  Wire.begin();
  Wire.setClock(100000);

  if (!gauge.begin(Wire)) {
    Serial.println("BQ27441 not found.");
    while (true) {
      delay(1000);
    }
  }

  printHelp();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  char cmd = Serial.read();
  switch (cmd) {
    case 'h': printHelp(); break;
    case 'r': printGauge(); break;
    case 'c': configureGauge(); break;
    case 'p': Serial.println(gauge.pulseGpout() ? "GPOUT pulsed." : "GPOUT pulse failed."); break;
    case 's': Serial.println(gauge.seal() ? "Gauge sealed." : "Seal failed."); break;
    case 'u': Serial.println(gauge.unseal() ? "Gauge unsealed." : "Unseal failed."); break;
  }
}
