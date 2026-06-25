#include <Wire.h>
#include <bq25.h>

/*
  BQ25895 Basic Charger Example

  Use this first when you want to prove that the charger IC is alive.

  What it does:
  - starts I2C with default Arduino pins
  - configures safe 1S Li-ion/LiPo charge defaults
  - disables the watchdog so the demo does not reset the charger settings
  - enables continuous ADC conversion
  - prints VBUS type, charge state, battery voltage, SYS voltage, charge current,
    charge fault, and NTC fault once per second

  For a real product, lower CHARGE_CURRENT_MA until your PCB, inductor, connector,
  USB source, and battery are all comfortably inside their limits.
*/

const uint16_t INPUT_LIMIT_MA = 500;
const uint16_t CHARGE_CURRENT_MA = 1000;
const uint16_t CHARGE_VOLTAGE_MV = 4208;

bq25 charger;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  if (!charger.begin(Wire)) {
    Serial.println("BQ25895 not found.");
    while (true) {
      delay(1000);
    }
  }

  bq25::Config cfg;
  cfg.inputCurrentLimitMa = INPUT_LIMIT_MA;
  cfg.chargeCurrentMa = CHARGE_CURRENT_MA;
  cfg.terminationCurrentMa = 100;
  cfg.prechargeCurrentMa = 128;
  cfg.chargeVoltageMv = CHARGE_VOLTAGE_MV;
  cfg.enableCharging = true;
  cfg.watchdog = bq25::WatchdogOff;
  charger.applyConfig(cfg);
  charger.startAdc(true);
}

void loop() {
  bq25::Status s = charger.status();
  bq25::Faults f = charger.faults();
  bq25::Measurements m = charger.measurements();

  Serial.print("VBUS=");
  Serial.print(bq25::vbusTypeName(s.vbusType));
  Serial.print(" CHG=");
  Serial.print(bq25::chargeStateName(s.chargeState));
  Serial.print(" BAT=");
  Serial.print(m.batteryMv);
  Serial.print("mV SYS=");
  Serial.print(m.systemMv);
  Serial.print("mV ICHG=");
  Serial.print(m.chargeCurrentMa);
  Serial.print("mA FAULT=");
  Serial.print(bq25::chargeFaultName(f.charge));
  Serial.print(" NTC=");
  Serial.println(bq25::ntcFaultName(f.ntc));

  delay(1000);
}
