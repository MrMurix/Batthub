#include <Wire.h>
#include <bq25_bq27.h>

/*
  Minimal Combined Module Example

  This is the small version of the full module firmware:
  - BQ25895 handles charging, power path, ADC telemetry, and faults
  - BQ27441 reports SOC, battery voltage, current, and capacity
  - bq25bq27 wraps both chips so one sketch can configure and read both

  For a more production-style starter, see examples/quick_start_module.
  For manual commands, see examples/serial_control_console.
*/

const uint16_t BATTERY_CAPACITY_MAH = 2500;
const uint16_t CHARGE_CURRENT_MA = 1000;
const uint16_t INPUT_LIMIT_MA = 500;
const uint16_t CHARGE_VOLTAGE_MV = 4208;
const uint16_t TERMINATE_VOLTAGE_MV = 3200;

bq25bq27 power;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  if (!power.begin(Wire)) {
    Serial.println("Module not found. Check SDA/SCL, 3V3, GND, and battery.");
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
}

void loop() {
  bq25bq27::Snapshot s = power.snapshot();

  Serial.print("SOC=");
  Serial.print(s.gauge.stateOfChargePercent);
  Serial.print("% BAT=");
  Serial.print(s.gauge.voltageMv);
  Serial.print("mV CHG=");
  Serial.print(bq25::chargeStateName(s.chargerStatus.chargeState));
  Serial.print(" VBUS=");
  Serial.print(bq25::vbusTypeName(s.chargerStatus.vbusType));
  Serial.print(" SYS=");
  Serial.print(s.chargerMeasurements.systemMv);
  Serial.print("mV FAULT=");
  Serial.println(bq25::chargeFaultName(s.chargerFaults.charge));

  delay(1000);
}
