#include <Batthub.h>

/*
  Batthub All Features Console

  This sketch is for customers who want to test everything the board exposes:
  charger control, OTG, adapter detection, register dumps, fuel-gauge values,
  and one-time fuel-gauge battery configuration.
*/

Batthub batthub;

const uint16_t BATTERY_CAPACITY_MAH = 2500;
const uint16_t CHARGE_CURRENT_MA = 1000;
const uint16_t INPUT_LIMIT_MA = 500;
const uint16_t CHARGE_VOLTAGE_MV = 4208;
const uint16_t TERMINATE_VOLTAGE_MV = 3200;

// Optional host GPIOs. Leave -1 when CE, OTG, or INT are not connected
// to your microcontroller.
const int8_t CE_PIN = -1;
const int8_t OTG_PIN = -1;
const int8_t INT_PIN = -1;

void printHex8(uint8_t value) {
  if (value < 16) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void printHex16(uint16_t value) {
  if (value < 0x1000) Serial.print('0');
  if (value < 0x0100) Serial.print('0');
  if (value < 0x0010) Serial.print('0');
  Serial.print(value, HEX);
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h help");
  Serial.println("  s status");
  Serial.println("  r BQ25895 register dump");
  Serial.println("  a apply safe charger defaults");
  Serial.println("  g configure BQ27441 battery profile once");
  Serial.println("  c/C charge on/off");
  Serial.println("  o/O OTG on/off");
  Serial.println("  d force DPDM detection");
  Serial.println("  i force ICO");
  Serial.println("  w watchdog off");
  Serial.println("  x enter ship mode");
  Serial.println("  5/9 input limit 500/900 mA");
  Serial.println("  1/2 charge current 1000/2000 mA");
}

void printBeginReport(const Batthub::BeginReport &report) {
  Serial.print("BQ25895: ");
  Serial.print(report.chargerPresent ? "PASS" : "FAIL");
  Serial.print(" part=0x");
  Serial.print(report.chargerPartNumber, HEX);
  Serial.print(" rev=");
  Serial.print(report.chargerRevision);
  Serial.print(" i2cErr=");
  Serial.println(report.chargerI2cError);

  Serial.print("BQ27441: ");
  Serial.print(report.gaugePresent ? "PASS" : "FAIL");
  Serial.print(" device=0x");
  printHex16(report.gaugeDeviceType);
  Serial.print(" i2cErr=");
  Serial.println(report.gaugeI2cError);
}

void printStatus() {
  Batthub::Snapshot s = batthub.snapshot();

  Serial.println();
  Serial.print("Optional pins: CE=");
  Serial.print(batthub.cePinConnected() ? (s.cePinEnabled ? "on" : "off") : "not connected");
  Serial.print(" OTG=");
  Serial.print(batthub.otgPinConnected() ? (s.otgPinEnabled ? "on" : "off") : "not connected");
  Serial.print(" INT=");
  Serial.println(batthub.intPinConnected() ? (s.interruptAsserted ? "asserted" : "idle") : "not connected");

  Serial.print("BQ25895: VBUS=");
  Serial.print(bq25::vbusTypeName(s.chargerStatus.vbusType));
  Serial.print(" PG=");
  Serial.print(s.chargerStatus.powerGood ? "yes" : "no");
  Serial.print(" CHG=");
  Serial.print(bq25::chargeStateName(s.chargerStatus.chargeState));
  Serial.print(" FAULT=");
  Serial.println(bq25::chargeFaultName(s.chargerFaults.charge));

  Serial.print("Voltages: VBUS=");
  Serial.print(s.chargerMeasurements.vbusMv);
  Serial.print("mV VBAT=");
  Serial.print(s.chargerMeasurements.batteryMv);
  Serial.print("mV VSYS=");
  Serial.print(s.chargerMeasurements.systemMv);
  Serial.print("mV ICHG=");
  Serial.print(s.chargerMeasurements.chargeCurrentMa);
  Serial.println("mA");

  Serial.print("Limits: inputSet=");
  Serial.print(batthub.charger.inputCurrentLimit());
  Serial.print("mA effective=");
  Serial.print(s.chargerMeasurements.effectiveInputCurrentLimitMa);
  Serial.print("mA IINDPM=");
  Serial.print(s.chargerMeasurements.iindpm ? "yes" : "no");
  Serial.print(" VINDPM=");
  Serial.println(s.chargerMeasurements.vindpm ? "yes" : "no");

  Serial.print("BQ27441: SOC=");
  Serial.print(s.gauge.stateOfChargePercent);
  Serial.print("% V=");
  Serial.print(s.gauge.voltageMv);
  Serial.print("mV I=");
  Serial.print(s.gauge.averageCurrentMa);
  Serial.print("mA Rem=");
  Serial.print(s.gauge.remainingCapacityMah);
  Serial.print("mAh Full=");
  Serial.print(s.gauge.fullChargeCapacityMah);
  Serial.print("mAh SOH=");
  Serial.print(s.gauge.stateOfHealthPercent);
  Serial.println("%");
}

void dumpRegisters() {
  uint8_t regs[bq25::kRegisterCount];
  if (!batthub.readChargerRegisters(regs)) {
    Serial.print("Register read failed. I2C error ");
    Serial.println(batthub.charger.lastError());
    return;
  }

  for (uint8_t i = 0; i < bq25::kRegisterCount; ++i) {
    Serial.print("REG");
    printHex8(i);
    Serial.print(" = 0x");
    printHex8(regs[i]);
    Serial.println();
  }
}

void configureGaugeOnce() {
  Batthub::BatteryProfile profile;
  profile.designCapacityMah = BATTERY_CAPACITY_MAH;
  profile.designEnergyMwh = static_cast<uint16_t>((static_cast<uint32_t>(BATTERY_CAPACITY_MAH) * 37UL) / 10UL);
  profile.terminateVoltageMv = TERMINATE_VOLTAGE_MV;
  profile.taperRate = 250;
  Serial.println(batthub.configureBattery(profile) ? "Gauge configured." : "Gauge configuration failed.");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Batthub::BeginOptions options;
  options.pins.ce = CE_PIN;
  options.pins.otg = OTG_PIN;
  options.pins.intPin = INT_PIN;

  Batthub::BeginReport report = batthub.beginReport(Wire, options);
  printBeginReport(report);
  if (report.ok()) {
    batthub.configure1s(
      BATTERY_CAPACITY_MAH,
      CHARGE_CURRENT_MA,
      INPUT_LIMIT_MA,
      CHARGE_VOLTAGE_MV,
      TERMINATE_VOLTAGE_MV,
      false
    );
  }

  printHelp();
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  char cmd = Serial.read();
  switch (cmd) {
    case '\r':
    case '\n':
      break;
    case 'h': printHelp(); break;
    case 's': printStatus(); break;
    case 'r': dumpRegisters(); break;
    case 'a': Serial.println(batthub.applySafeDefaults() ? "Safe defaults applied." : "Apply failed."); break;
    case 'g': configureGaugeOnce(); break;
    case 'c': Serial.println(batthub.setChargeEnabled(true) ? "Charge on." : "Charge on failed."); break;
    case 'C': Serial.println(batthub.setChargeEnabled(false) ? "Charge off." : "Charge off failed."); break;
    case 'o': Serial.println(batthub.setOtgEnabled(true) ? "OTG on." : "OTG on failed."); break;
    case 'O': Serial.println(batthub.setOtgEnabled(false) ? "OTG off." : "OTG off failed."); break;
    case 'd': Serial.println(batthub.forceDpdmDetection() ? "DPDM forced." : "DPDM failed."); break;
    case 'i': Serial.println(batthub.forceIco() ? "ICO forced." : "ICO failed."); break;
    case 'w': Serial.println(batthub.charger.setWatchdogTimer(bq25::WatchdogOff) ? "Watchdog off." : "Watchdog command failed."); break;
    case 'x':
      Serial.println("Entering ship mode.");
      delay(100);
      batthub.enterShipMode();
      break;
    case '5': Serial.println(batthub.setInputCurrentLimit(500) ? "Input limit 500 mA." : "Input limit failed."); break;
    case '9': Serial.println(batthub.setInputCurrentLimit(900) ? "Input limit 900 mA." : "Input limit failed."); break;
    case '1': Serial.println(batthub.setChargeCurrent(1000) ? "Charge current 1000 mA." : "Charge current failed."); break;
    case '2': Serial.println(batthub.setChargeCurrent(2000) ? "Charge current 2000 mA." : "Charge current failed."); break;
    default:
      Serial.println("Unknown command.");
      printHelp();
      break;
  }
}
