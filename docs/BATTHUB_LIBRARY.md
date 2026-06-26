# Batthub Library

`Batthub` is the public module API for the Batthub PCB. The module has no MCU. A buyer connects it to their own controller over I2C.

For the ESP32 web dashboard terms, buttons, and settings, see `docs/BATTHUB_WEB_UI.md`.

## Deutsch - Kurzfassung

`Batthub` ist die einfache Modul-API fuer deine Batthub-Platine. Die Platine hat keinen eigenen Mikrocontroller. Der Nutzer verbindet seinen eigenen Controller per I2C mit dem Modul.

Start fuer normale Nutzer:

1. Library installieren.
2. Beispiel `batthub_quick_start` oeffnen.
3. SDA und SCL mit dem eigenen Controller verbinden.
4. Board auswaehlen und hochladen.
5. Serial Monitor mit `115200` oeffnen.

Wichtig:

- Die Library setzt keine festen ESP32-Pins voraus.
- CE, OTG und INT sind optionale Host-Pins.
- Die Standard-I2C-Adressen von TI werden automatisch benutzt.
- `Batthub` ist fuer normale Nutzer gedacht.
- `bq25` und `bq27` bleiben fuer Fortgeschrittene erreichbar.

Kurzes Beispiel:

```cpp
#include <Batthub.h>

Batthub batthub;

void setup() {
  Serial.begin(115200);

  if (!batthub.begin()) {
    Serial.println("Batthub nicht gefunden");
    return;
  }

  batthub.applySafeDefaults();
}

void loop() {
  Serial.println(batthub.stateOfChargePercent());
  delay(1000);
}
```

Mehr fuer Beginner:

- [BQ25895 und BQ27441 fuer Beginner](BQ25_BQ27_BEGINNER.md#deutsch)
- [Web UI Doku](BATTHUB_WEB_UI.md#deutsch)

## English

```cpp
#include <Batthub.h>

Batthub batthub;

void setup() {
  Serial.begin(115200);

  if (!batthub.begin()) {
    Serial.println("Batthub not found");
    return;
  }

  batthub.applySafeDefaults();
}
```

## Beginner Flow

For normal buyers:

1. Install the library.
2. Open `File > Examples > Batthub > batthub_quick_start`.
3. Connect SDA/SCL to their controller's I2C pins.
4. Upload to their controller.
5. Open Serial Monitor at `115200`.

They do not need to know the BQ25895 or BQ27441 addresses. If their controller uses non-default I2C pins, they start `Wire` themselves before calling `batthub.begin(Wire)`.

Simple readings:

```cpp
Serial.println(batthub.stateOfChargePercent());
Serial.println(batthub.batteryVoltageMv());
Serial.println(batthub.chargeStateText());
```

## Optional Host Pins

The Batthub module exposes CE, OTG, and INT. These are optional connections to the user's controller.

By default the library assumes those pins are not connected. If a project wires them to GPIOs, pass them in `Batthub::BeginOptions`:

```cpp
Batthub::BeginOptions options;
options.pins.ce = 8;
options.pins.otg = 9;
options.pins.intPin = 10;
batthub.begin(Wire, options);
```

## API Layers

The library has three layers:

- `Batthub`: module-level API for buyers of your PCB.
- `bq25`: full BQ25895 charger/power-path API.
- `bq27`: full BQ27441-G1 fuel-gauge API.

`Batthub` exposes the low-level drivers as public members:

```cpp
batthub.charger.setSafetyTimer(bq25::SafetyTimer12h);
batthub.gauge.setGpoutMode(bq27::GpoutSocInt);
```

That keeps all chip features reachable without forcing every user to work at register level.

## Main Module Features

- Probe both chips with `beginReport()`.
- Enable or disable charge with `setChargeEnabled()`.
- Enable or disable OTG boost with `setOtgEnabled()`.
- Enter ship mode with `enterShipMode()`.
- Apply safe charger defaults with `applySafeDefaults()`.
- Configure a 1S battery profile with `configure1s()`.
- Configure the BQ27441 fuel gauge with `configureBattery()`.
- Read complete board telemetry with `snapshot()`.
- Dump BQ25895 registers with `readChargerRegisters()`.
- Control adapter detection with `setAdapterDetection()`, `forceDpdmDetection()`, and `forceIco()`.

## Begriffserklaerung

Kurzregel fuer Namen:

- `Ma` bedeutet mA, also Milliampere.
- `Mv` bedeutet mV, also Millivolt.
- `Mw` bedeutet mW, also Milliwatt.
- `Mwh` bedeutet mWh, also Milliwattstunden.
- `Mohm` bedeutet mOhm, also Milliohm.
- `Percent` bedeutet Prozent.
- `X10` bedeutet Wert mal 10, zum Beispiel `253` ist `25.3 C`.
- `X100` bedeutet Wert mal 100, zum Beispiel `5050` ist `50.50 percent`.
- `Enabled` bedeutet ein- oder ausgeschaltet.

### Grundbegriffe

| Begriff | Kurz erklaert |
| --- | --- |
| `Batthub` | Die einfache Modul-API fuer dein PCB. |
| `bq25` / `BQ25895` | Ladechip fuer USB-Eingang, Akku-Laden, Power-Path und OTG-Boost. |
| `bq27` / `BQ27441-G1` | Fuel-Gauge-Chip fuer Akku-Prozent, Kapazitaet, Strom, Spannung und Status. |
| `Host` | Der Controller vom Nutzer, zum Beispiel ESP32, Arduino, STM32 oder Raspberry Pi Pico. |
| `I2C` | Zwei-Draht-Bus, ueber den der Host mit beiden Chips spricht. |
| `Wire` | Das Arduino-I2C-Objekt. |
| `SDA` | I2C-Datenleitung. |
| `SCL` | I2C-Taktleitung. |
| `CE` | Charger-Enable-Pin vom BQ25895; meistens aktiv-low. |
| `OTG` | Pin oder Funktion fuer Boost-Ausgang von Akku nach VBUS. |
| `INT` | Interrupt-Pin fuer Statusmeldung an den Host. |
| `VBUS` | Eingangsspannung vom USB-Netzteil oder Host-Port. |
| `BAT` | Akku-Anschluss am Modul. |
| `SYS` | System-Ausgang vom Power-Path. |
| `TS` | Temperatur-Eingang vom BQ25895 fuer Akku-NTC. |
| `NTC` | Temperatur-Widerstand am Akku. |
| `ADC` | Messwandler im Chip fuer Spannung, Strom und Temperatur. |
| `DPM` | Dynamic Power Management; der Chip reduziert Leistung, wenn Eingang oder Temperatur an Grenzen kommen. |

### Start und Pins

| Begriff | Kurz erklaert |
| --- | --- |
| `begin()` | Startet die Library und prueft die Standard-Adressen. |
| `beginReport()` | Startet beide Chips und sagt genau, welcher Chip gefunden wurde. |
| `BeginOptions` | Optionale Start-Einstellungen fuer Pins, Adressen und I2C-Speed. |
| `pins` | Optionale Host-GPIOs fuer CE, OTG und INT. |
| `PinNotConnected` | Dieser Modul-Pin ist nicht mit dem Host verbunden. |
| `chargerAddress` | I2C-Adresse vom BQ25895; normalerweise Default lassen. |
| `gaugeAddress` | I2C-Adresse vom BQ27441; normalerweise Default lassen. |
| `i2cClockHz` | I2C-Geschwindigkeit. |
| `startWire` | Wenn true, ruft die Library `Wire.begin()` auf. |
| `setupPins` | Wenn true, richtet die Library CE, OTG und INT als GPIOs ein. |
| `initialCeEnabled` | Startzustand fuer den CE-Pin. |
| `initialOtgEnabled` | Startzustand fuer den OTG-Pin. |
| `ceActiveLow` | CE ist aktiv, wenn der Host-Pin LOW ist. |
| `otgActiveHigh` | OTG ist aktiv, wenn der Host-Pin HIGH ist. |
| `intActiveLow` | INT ist aktiv, wenn der Chip den Pin LOW zieht. |

### BQ25895 Charger Settings

| Begriff | Kurz erklaert |
| --- | --- |
| `cePinEnabled` | Schaltet nur den optionalen CE-Host-Pin. |
| `chargingEnabled` | Schaltet das Laden im BQ25895 ein oder aus. |
| `otgEnabled` | Schaltet den Boost-Modus ein oder aus. |
| `highImpedanceMode` | Trennt den Eingangspfad weitgehend und reduziert Verbrauch. |
| `ilimPinEnabled` | Nutzt den physischen ILIM-Pin als zusaetzliche Stromgrenze. |
| `inputCurrentLimitMa` | Maximaler Strom, den das Modul vom Eingang ziehen darf. |
| `inputVoltageLimitMv` | Eingangsspannung, unter der der Chip den Strom reduziert. |
| `vindpmOffsetMv` | Zusatz-Abstand fuer die Eingangsspannungs-Regelung. |
| `chargeCurrentMa` | Maximaler Ladestrom in den Akku. |
| `prechargeCurrentMa` | Kleiner Ladestrom fuer tief entladene Akkus. |
| `terminationCurrentMa` | Stromgrenze, ab der der Akku als voll gilt. |
| `chargeVoltageMv` | Zielspannung fuer vollen 1S-Li-Ion-Akku. |
| `minSystemVoltageMv` | Mindestspannung am SYS-Ausgang. |
| `boostVoltageMv` | Ausgangsspannung im OTG-Boost-Modus. |
| `irCompResistanceMohm` | Akku-Leitungswiderstand fuer Spannungs-Kompensation. |
| `irCompVoltageClampMv` | Maximal erlaubte Zusatzspannung durch IR-Kompensation. |
| `watchdog` | Timer, der Einstellungen zuruecksetzen kann, wenn er nicht bedient wird. |
| `safetyTimer` | Maximale Ladezeit, bevor der Chip einen Fehler setzt. |
| `thermalRegulation` | Chip-Temperatur, ab der der Ladestrom reduziert wird. |
| `boostFrequency` | Schaltfrequenz im OTG-Boost-Modus. |
| `boostHotThreshold` | Warme TS-Grenze fuer Boost-Betrieb. |
| `boostColdThreshold` | Kalte TS-Grenze fuer Boost-Betrieb. |
| `batteryLowThreshold` | Akku-Spannung, ab der Low-Battery gilt. |
| `rechargeThreshold` | Spannungsabfall unter voll, ab dem Nachladen startet. |
| `batteryLoadEnabled` | Aktiviert einen kurzen internen Akku-Lasttest. |
| `pumpXEnabled` | Aktiviert PumpX-Spannungsverhandlung. |
| `terminationEnabled` | Erlaubt automatisches Ladeende bei kleinem Strom. |
| `safetyTimerEnabled` | Aktiviert den Sicherheits-Timer. |
| `safetyTimerSlowedInDpm` | Verlangsamt den Timer, wenn DPM den Ladevorgang begrenzt. |
| `icoEnabled` | Optimiert den Eingangsstrom automatisch. |
| `autoDpdmEnabled` | Erkennt USB-Adapter ueber D+ und D- automatisch. |
| `hvdcpEnabled` | Erlaubt QuickCharge-artige Hochspannungs-Erkennung. |
| `maxChargeEnabled` | Erlaubt MaxCharge-Adaptererkennung. |
| `adcContinuous` | Misst Spannung, Strom und Temperatur dauerhaft. |
| `absoluteVindpm` | Nutzt absolute Eingangsspannungsgrenze statt relativer Grenze. |
| `batfetResetEnabled` | Erlaubt BATFET-Reset-Funktion vom Chip. |

### BQ25895 Status und Befehle

| Begriff | Kurz erklaert |
| --- | --- |
| `powerGood` | Eingang ist gueltig und kann das System versorgen. |
| `inputTypeText()` | Text fuer erkannten Eingangstyp, zum Beispiel USB oder OTG. |
| `chargeStateText()` | Text fuer Ladezustand: nicht laden, precharge, fast charge oder done. |
| `chargeFaultText()` | Text fuer Ladefehler. |
| `ntcFaultText()` | Text fuer Akku-Temperaturfehler. |
| `systemInRegulation` | SYS wird aktiv geregelt. |
| `thermalRegulation` | Chip ist warm und reduziert Strom. |
| `vbusGood` | Eingangsspannung ist vorhanden und nutzbar. |
| `vindpm` | Eingangsspannung bricht ein, Chip reduziert Strom. |
| `iindpm` | Eingangsstromlimit ist erreicht, Chip begrenzt Strom. |
| `effectiveInputCurrentLimitMa` | Aktuell wirksames Eingangsstromlimit. |
| `tsPercentX100` | TS-Pin als Prozentwert fuer NTC-Auswertung. |
| `icoOptimized` | ICO hat einen optimierten Eingangsstrom gefunden. |
| `forceDpdmDetection()` | Startet USB-Adaptererkennung sofort. |
| `forceIco()` | Startet Eingangsstrom-Optimierung sofort. |
| `pumpXIncrease()` | Fordert hoehere Eingangsspannung ueber PumpX an. |
| `pumpXDecrease()` | Fordert niedrigere Eingangsspannung ueber PumpX an. |
| `enterShipMode()` | Schaltet BATFET fuer Lagerung/Transport ab. |
| `resetCharger()` | Setzt den BQ25895 auf Default-Werte zurueck. |

### BQ27441 Battery Profile

| Begriff | Kurz erklaert |
| --- | --- |
| `designCapacityMah` | Nennkapazitaet vom Akku in mAh. |
| `designEnergyMwh` | Nennenergie vom Akku in mWh. |
| `terminateVoltageMv` | Entladeschluss-Spannung fuer die Kapazitaetsrechnung. |
| `taperRate` | Verhaeltnis, ab wann das Ladeende fuer den Gauge erkannt wird. |
| `soc1SetPercent` | Prozentwert, bei dem Low-Battery gesetzt wird. |
| `soc1ClearPercent` | Prozentwert, bei dem Low-Battery wieder geloescht wird. |
| `socfSetPercent` | Prozentwert, bei dem Final-Low-Battery gesetzt wird. |
| `socfClearPercent` | Prozentwert, bei dem Final-Low-Battery wieder geloescht wird. |
| `socIntDeltaPercent` | Prozent-Schritt, ab dem GPOUT bei SOC-Aenderung ausloest. |

### BQ27441 Options

| Begriff | Kurz erklaert |
| --- | --- |
| `gpoutMode` | Funktion vom GPOUT-Pin: Akku niedrig oder SOC-Interrupt. |
| `temperatureSource` | Temperatur kommt vom internen Sensor oder vom Host. |
| `gpoutActiveHigh` | GPOUT ist aktiv bei HIGH statt LOW. |
| `batteryInsertionEnabled` | Gauge erkennt Akku-Einsetzen automatisch. |
| `binPullupEnabled` | Aktiviert Pullup am BIN-Pin. |
| `sleepEnabled` | Erlaubt Schlafmodus fuer weniger Verbrauch. |
| `rmFccSmoothingEnabled` | Glaettet Remaining- und Full-Charge-Capacity. |
| `pulseGpout()` | Gibt einen kurzen Puls am GPOUT-Pin aus. |
| `batteryInsert()` | Meldet dem Gauge manuell, dass ein Akku eingesetzt ist. |
| `batteryRemove()` | Meldet dem Gauge manuell, dass der Akku entfernt ist. |
| `unseal()` | Oeffnet geschuetzte Gauge-Konfiguration. |
| `seal()` | Sperrt die Gauge-Konfiguration wieder. |
| `setHibernate()` | Setzt den Gauge in Hibernate. |
| `clearHibernate()` | Weckt den Gauge aus Hibernate. |
| `shutdownEnable()` | Erlaubt den naechsten Shutdown-Befehl. |
| `shutdown()` | Schaltet den Gauge ab. |
| `softReset()` | Startet den Gauge logisch neu. |
| `resetGauge()` | Fuehrt einen Gauge-Reset aus. |

### BQ27441 Readings und Flags

| Begriff | Kurz erklaert |
| --- | --- |
| `stateOfChargePercent` | Akku-Prozent vom Gauge. |
| `stateOfChargeUnfilteredPercent` | Akku-Prozent ohne Glaettung. |
| `voltageMv` | Akku-Spannung. |
| `averageCurrentMa` | Durchschnittlicher Akku-Strom. |
| `standbyCurrentMa` | Erwarteter Strom im Standby. |
| `maxLoadCurrentMa` | Geschaetzter maximaler Laststrom. |
| `averagePowerMw` | Durchschnittliche Akku-Leistung. |
| `remainingCapacityMah` | Noch verfuegbare Kapazitaet. |
| `fullChargeCapacityMah` | Vom Gauge gelernte volle Kapazitaet. |
| `stateOfHealthPercent` | Akku-Gesundheit in Prozent. |
| `stateOfHealthStatus` | Zusatzstatus zur Akku-Gesundheit. |
| `temperatureCelsiusX10` | Akku-Temperatur mal 10. |
| `internalTemperatureCelsiusX10` | Interne Gauge-Temperatur mal 10. |
| `batteryDetected` | Gauge erkennt einen Akku. |
| `charging` | Gauge sieht Ladestrom in den Akku. |
| `discharging` | Gauge sieht Entladestrom aus dem Akku. |
| `fullCharge` | Gauge meldet Akku voll. |
| `overTemperature` | Temperatur ist zu hoch. |
| `underTemperature` | Temperatur ist zu niedrig. |
| `ocvTaken` | Gauge hat eine Leerlaufspannungs-Messung genommen. |
| `powerOnReset` | Gauge hatte einen Reset seit letztem Clear. |
| `configUpdateMode` | Gauge ist im Konfigurationsmodus. |
| `soc1` | Low-Battery-Schwelle ist aktiv. |
| `socFinal` | Final-Low-Battery-Schwelle ist aktiv. |
| `sealed` | Geschuetzte Konfiguration ist gesperrt. |
| `hibernate` | Gauge ist im Hibernate-Modus. |
| `sleep` | Gauge ist im Sleep-Modus. |
| `initComplete` | Gauge-Start ist fertig. |
| `voltageOk` | Akku-Spannung ist fuer den Gauge gueltig. |
| `shutdownEnabled` | Shutdown wurde freigegeben. |
| `watchdogReset` | Gauge-Watchdog hat einen Reset erkannt. |
| `qmaxUpdated` | Gauge hat die gelernte Akku-Kapazitaet aktualisiert. |
| `resistanceUpdated` | Gauge hat das Akku-Widerstandsmodell aktualisiert. |

### Modul-API Kurzbefehle

| Begriff | Kurz erklaert |
| --- | --- |
| `applySafeDefaults()` | Setzt sinnvolle sichere Standardwerte fuer den Charger. |
| `applyChargerSettings()` | Schreibt alle BQ25895-Einstellungen aus `ChargerSettings`. |
| `applyGaugeSettings()` | Schreibt Gauge-Optionen und optional das Akku-Profil. |
| `configureBattery()` | Schreibt nur das BQ27441-Akku-Profil. |
| `readBatteryProfile()` | Liest das aktuelle BQ27441-Akku-Profil. |
| `configure1s()` | Schnelle Standardkonfiguration fuer einen 1S-Li-Ion-Akku. |
| `setChargeEnabled()` | Schaltet Laden ueber CE-Pin und BQ25895-Software ein oder aus. |
| `setOtgEnabled()` | Schaltet OTG ueber Host-Pin und BQ25895-Software ein oder aus. |
| `startAdc()` | Startet Messungen im BQ25895. |
| `stopAdc()` | Stoppt Messungen im BQ25895. |
| `snapshot()` | Liest Status, Fehler und Messwerte von beiden Chips. |
| `resetBoth()` | Setzt BQ25895 und BQ27441 zurueck. |

## Important Product Note

The library exposes the hardware. It does not certify the hardware.

Before selling a Batthub module, validate charge current, thermals, NTC behavior, input connector rating, OTG load, battery profile, and regulatory requirements on real PCBs.
