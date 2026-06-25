#include <Wire.h>
#include <bq25_bq27.h>

/*
  Serial Control Console

  Open Serial Monitor at 115200 baud and use one-character commands:

    h  print help
    s  print status
    a  ADC continuous ON
    A  ADC OFF
    c  charger ON
    C  charger OFF
    o  OTG ON
    O  OTG OFF
    d  force D+/D- input detection
    w  watchdog OFF
    r  reset BQ25895 registers, then reapply safe settings
    5  set USB input limit to 500 mA
    9  set USB input limit to 900 mA
    1  set charge current to 1000 mA
    2  set charge current to 2000 mA

  This example is useful for bring-up because every action is explicit.
*/

bq25bq27 power;

void applySafeSettings() {
  power.charger.setWatchdogTimer(bq25::WatchdogOff);
  power.charger.setInputCurrentLimit(500);
  power.charger.setChargeCurrent(1000);
  power.charger.setChargeVoltage(4208);
  power.charger.setTerminationCurrent(128);
  power.charger.setChargingEnabled(true);
  power.charger.startAdc(true);
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h help");
  Serial.println("  s status");
  Serial.println("  a/A ADC on/off");
  Serial.println("  c/C charger on/off");
  Serial.println("  o/O OTG on/off");
  Serial.println("  d force DPDM detection");
  Serial.println("  w watchdog off");
  Serial.println("  r reset charger registers and reapply safe settings");
  Serial.println("  5/9 input limit 500/900 mA");
  Serial.println("  1/2 charge current 1000/2000 mA");
}

void printStatus() {
  bq25::Status st = power.charger.status();
  bq25::Faults f = power.charger.faults();
  bq25::Measurements m = power.charger.measurements();

  Serial.print("VBUS=");
  Serial.print(bq25::vbusTypeName(st.vbusType));
  Serial.print(" powerGood=");
  Serial.print(st.powerGood ? "yes" : "no");
  Serial.print(" charge=");
  Serial.print(bq25::chargeStateName(st.chargeState));
  Serial.print(" fault=");
  Serial.print(bq25::chargeFaultName(f.charge));
  Serial.print(" BAT=");
  Serial.print(m.batteryMv);
  Serial.print("mV SYS=");
  Serial.print(m.systemMv);
  Serial.print("mV ICHG=");
  Serial.print(m.chargeCurrentMa);
  Serial.println("mA");

  Serial.print("Gauge SOC=");
  Serial.print(power.gauge.stateOfCharge());
  Serial.print("% voltage=");
  Serial.print(power.gauge.voltage());
  Serial.print("mV avgCurrent=");
  Serial.print(power.gauge.averageCurrent());
  Serial.println("mA");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  Wire.begin();
  Wire.setClock(100000);

  bq25bq27::BeginReport report = power.beginReport(Wire);
  if (!report.ok()) {
    Serial.println("Module probe failed. Some commands may not work.");
  }

  applySafeSettings();
  printHelp();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  char cmd = Serial.read();
  switch (cmd) {
    case 'h': printHelp(); break;
    case 's': printStatus(); break;
    case 'a': power.charger.startAdc(true); Serial.println("ADC on"); break;
    case 'A': power.charger.stopAdc(); Serial.println("ADC off"); break;
    case 'c': power.charger.setChargingEnabled(true); Serial.println("charger on"); break;
    case 'C': power.charger.setChargingEnabled(false); Serial.println("charger off"); break;
    case 'o': power.charger.setOtgEnabled(true); Serial.println("OTG on"); break;
    case 'O': power.charger.setOtgEnabled(false); Serial.println("OTG off"); break;
    case 'd': power.charger.forceDpdmDetection(); Serial.println("DPDM forced"); break;
    case 'w': power.charger.setWatchdogTimer(bq25::WatchdogOff); Serial.println("watchdog off"); break;
    case 'r':
      power.charger.resetRegisters();
      delay(100);
      applySafeSettings();
      Serial.println("BQ25895 reset and safe settings reapplied");
      break;
    case '5': power.charger.setInputCurrentLimit(500); Serial.println("input limit 500 mA"); break;
    case '9': power.charger.setInputCurrentLimit(900); Serial.println("input limit 900 mA"); break;
    case '1': power.charger.setChargeCurrent(1000); Serial.println("charge current 1000 mA"); break;
    case '2': power.charger.setChargeCurrent(2000); Serial.println("charge current 2000 mA"); break;
  }
}
