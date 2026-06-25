#include <Wire.h>
#include <bq25.h>

/*
  OTG and Ship Mode Example

  Serial commands:
    h  help
    s  status
    o  OTG boost ON
    O  OTG boost OFF
    v  set OTG boost voltage to about 5.1 V
    i  set input limit to 500 mA
    c  charger ON
    C  charger OFF
    x  enter ship mode

  Ship mode warning:
  Once the BATFET is turned off, the board may shut down immediately.
  You usually need VBUS or a hardware wake condition to recover.
*/

bq25 charger;

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h help");
  Serial.println("  s status");
  Serial.println("  o/O OTG on/off");
  Serial.println("  v set boost voltage near 5.1 V");
  Serial.println("  i set input current limit 500 mA");
  Serial.println("  c/C charger on/off");
  Serial.println("  x enter ship mode");
}

void printStatus() {
  bq25::Status st = charger.status();
  bq25::Measurements m = charger.measurements();
  bq25::Faults f = charger.faults();

  Serial.print("VBUS=");
  Serial.print(bq25::vbusTypeName(st.vbusType));
  Serial.print(" charge=");
  Serial.print(bq25::chargeStateName(st.chargeState));
  Serial.print(" BAT=");
  Serial.print(m.batteryMv);
  Serial.print("mV SYS=");
  Serial.print(m.systemMv);
  Serial.print("mV fault=");
  Serial.println(bq25::chargeFaultName(f.charge));
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  Wire.begin();
  Wire.setClock(100000);

  if (!charger.begin(Wire)) {
    Serial.println("BQ25895 not found.");
    while (true) {
      delay(1000);
    }
  }

  charger.setWatchdogTimer(bq25::WatchdogOff);
  charger.startAdc(true);
  charger.setBoostVoltage(5126);
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
    case 'o': charger.setOtgEnabled(true); Serial.println("OTG enabled."); break;
    case 'O': charger.setOtgEnabled(false); Serial.println("OTG disabled."); break;
    case 'v': charger.setBoostVoltage(5126); Serial.println("Boost voltage set near 5.1 V."); break;
    case 'i': charger.setInputCurrentLimit(500); Serial.println("Input limit set to 500 mA."); break;
    case 'c': charger.setChargingEnabled(true); Serial.println("Charger enabled."); break;
    case 'C': charger.setChargingEnabled(false); Serial.println("Charger disabled."); break;
    case 'x':
      Serial.println("Entering ship mode now.");
      delay(100);
      charger.enterShipMode();
      break;
  }
}
