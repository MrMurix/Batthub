#include <Batthub.h>

Batthub batthub;

void setup() {
  Serial.begin(115200);
  delay(500);

  Batthub::BeginReport report = batthub.beginReport();
  if (!report.ok()) {
    Serial.println("Batthub module not found. Check power, battery, and I2C.");
    while (true) {
      delay(1000);
    }
  }

  batthub.applySafeDefaults();
  Serial.println("Batthub ready.");
}

void loop() {
  Serial.print("Battery=");
  Serial.print(batthub.stateOfChargePercent());
  Serial.print("% VBAT=");
  Serial.print(batthub.batteryVoltageMv());
  Serial.print("mV I=");
  Serial.print(batthub.batteryCurrentMa());
  Serial.print("mA CHG=");
  Serial.print(batthub.chargeStateText());
  Serial.print(" VBUS=");
  Serial.print(batthub.inputTypeText());
  Serial.print(" SYS=");
  Serial.print(batthub.systemVoltageMv());
  Serial.println("mV");

  delay(1000);
}
