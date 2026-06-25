#include <Wire.h>
#include <bq25_bq27.h>

/*
  Quick Start: combined BQ25895 + BQ27441 module

  This is the shortest useful product-style example:
  - start I2C
  - probe both chips
  - apply a conservative 1S Li-ion/LiPo configuration
  - print live charger and gauge readings once per second

  Change these values for your real cell:
    BATTERY_CAPACITY_MAH  capacity printed on the cell
    CHARGE_CURRENT_MA     do not exceed the cell, PCB and thermal limits
    INPUT_LIMIT_MA        current limit for your USB/input source
*/

const uint16_t BATTERY_CAPACITY_MAH = 2500;
const uint16_t CHARGE_CURRENT_MA = 1000;
const uint16_t INPUT_LIMIT_MA = 500;
const uint16_t CHARGE_VOLTAGE_MV = 4208;
const uint16_t TERMINATE_VOLTAGE_MV = 3200;

bq25bq27 power;

void printBeginReport(const bq25bq27::BeginReport &report) {
  Serial.print("BQ25895 present: ");
  Serial.print(report.chargerPresent ? "yes" : "no");
  Serial.print("  PN: 0x");
  Serial.print(report.chargerPartNumber, HEX);
  Serial.print("  REV: ");
  Serial.print(report.chargerRevision);
  Serial.print("  I2C error: ");
  Serial.println(report.chargerI2cError);

  Serial.print("BQ27441 present: ");
  Serial.print(report.gaugePresent ? "yes" : "no");
  Serial.print("  Device type: 0x");
  Serial.print(report.gaugeDeviceType, HEX);
  Serial.print("  I2C error: ");
  Serial.println(report.gaugeI2cError);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  Wire.begin();
  Wire.setClock(100000);

  bq25bq27::BeginReport report = power.beginReport(Wire);
  printBeginReport(report);
  if (!report.ok()) {
    Serial.println("Module probe failed. Check wiring, pullups, 3V3 and battery.");
    while (true) {
      delay(1000);
    }
  }

  power.configure1s(
    BATTERY_CAPACITY_MAH,
    CHARGE_CURRENT_MA,
    INPUT_LIMIT_MA,
    CHARGE_VOLTAGE_MV,
    TERMINATE_VOLTAGE_MV
  );
  power.charger.startAdc(true);

  Serial.println("Module configured.");
}

void loop() {
  bq25bq27::Snapshot s = power.snapshot();

  Serial.print("SOC=");
  Serial.print(s.gauge.stateOfChargePercent);
  Serial.print("% BAT=");
  Serial.print(s.gauge.voltageMv);
  Serial.print("mV SYS=");
  Serial.print(s.chargerMeasurements.systemMv);
  Serial.print("mV VBUS=");
  Serial.print(bq25::vbusTypeName(s.chargerStatus.vbusType));
  Serial.print(" CHG=");
  Serial.print(bq25::chargeStateName(s.chargerStatus.chargeState));
  Serial.print(" FAULT=");
  Serial.println(bq25::chargeFaultName(s.chargerFaults.charge));

  delay(1000);
}
