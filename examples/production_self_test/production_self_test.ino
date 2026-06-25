#include <Wire.h>
#include <bq25_bq27.h>

/*
  Production Self Test

  Use this on a freshly assembled board before shipping or before deeper tests.
  It does not change charge settings except enabling continuous ADC conversion.

  It prints:
  - all I2C addresses found on the bus
  - BQ25895 part number and revision
  - BQ27441 device type and chemistry ID
  - BQ25895 raw register dump
  - a short pass/fail summary
*/

bq25bq27 power;

void scanI2c() {
  Serial.println("I2C scan:");
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  found 0x");
      if (addr < 16) {
        Serial.print("0");
      }
      Serial.println(addr, HEX);
    }
  }
}

void dumpBq25895Registers() {
  uint8_t regs[bq25::kRegisterCount];
  if (!power.charger.readRegisters(regs, bq25::kRegisterCount)) {
    Serial.print("BQ25895 register read failed, I2C error ");
    Serial.println(power.charger.lastError());
    return;
  }

  Serial.println("BQ25895 registers:");
  for (uint8_t i = 0; i < bq25::kRegisterCount; ++i) {
    Serial.print("  REG");
    if (i < 16) {
      Serial.print("0");
    }
    Serial.print(i, HEX);
    Serial.print(" = 0x");
    if (regs[i] < 16) {
      Serial.print("0");
    }
    Serial.println(regs[i], HEX);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  Wire.begin();
  Wire.setClock(100000);

  scanI2c();

  bq25bq27::BeginReport report = power.beginReport(Wire, bq25bq27::BeginOptions());
  Serial.println();
  Serial.println("Probe report:");
  Serial.print("  BQ25895 present: ");
  Serial.println(report.chargerPresent ? "PASS" : "FAIL");
  Serial.print("  BQ25895 part/rev: 0x");
  Serial.print(report.chargerPartNumber, HEX);
  Serial.print(" / ");
  Serial.println(report.chargerRevision);
  Serial.print("  BQ27441 present: ");
  Serial.println(report.gaugePresent ? "PASS" : "FAIL");
  Serial.print("  BQ27441 device type: 0x");
  Serial.println(report.gaugeDeviceType, HEX);
  Serial.print("  BQ27441 chemistry ID: 0x");
  Serial.println(power.gauge.chemistryId(), HEX);

  power.charger.startAdc(true);
  delay(200);
  dumpBq25895Registers();

  Serial.println();
  Serial.println(report.ok() ? "SELF TEST RESULT: PASS" : "SELF TEST RESULT: FAIL");
}

void loop() {
}
