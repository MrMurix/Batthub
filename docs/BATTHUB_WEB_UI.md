# Batthub Web UI Doku

## Deutsch

Diese Doku gehoert zum Beispiel `examples/web_debug_ui`. Das Beispiel laeuft auf einem ESP32 und benutzt den ESP32 nur als Host fuer die Batthub-Platine. Die Batthub-Platine selbst hat keinen ESP32.

## Bedienlogik

| Web UI Begriff | Was es bedeutet |
| --- | --- |
| `Batthub Web UI` | Browser-Oberflaeche fuer Live-Test und Konfiguration von BQ25895 und BQ27441. |
| `Connecting...` | Browser wartet auf Daten vom ESP32. |
| `Connection error` | Browser erreicht den ESP32-Webserver gerade nicht. |
| `Not found` | Der jeweilige BQ-Chip antwortet nicht auf I2C. |
| `ON` / `OFF` | Schalterzustand; gruen ist ON, rot ist OFF. |
| `Yes` / `No` | Statuszustand; kein Schalter. |
| `Expert Locked` | Normale Schutzstufe. Riskante Aktionen fragen nach, bevor sie geschrieben werden. |
| `Expert Unlocked` | Expertenmodus. Riskante Aktionen bleiben erreichbar und fragen weniger nach. |
| `Safe Charge` | Schreibt einen konservativen, stabilen Lade-Startzustand: Laden an, OTG/HIZ aus, feste VINDPM-Grenze, keine automatische Adapter-Optimierung. |
| `Reset Both` | Setzt BQ25895 und BQ27441 zurueck und schreibt danach die gespeicherten Charger-Settings wieder. |
| `BQ25895 Charger` | Tab fuer Laden, USB-Eingang, Power-Path, OTG und Charger-Fehler. |
| `BQ27441 Gauge` | Tab fuer Akku-Prozent, Akku-Profil, Fuel-Gauge-Status und Gauge-Optionen. |

## Einheiten

| Endung | Einheit |
| --- | --- |
| `Ma` | mA, Milliampere. |
| `Mv` | mV, Millivolt. |
| `Mw` | mW, Milliwatt. |
| `Mwh` | mWh, Milliwattstunden. |
| `Mohm` | mOhm, Milliohm. |
| `Percent` | Prozent. |
| `X10` | Wert mal 10; `253` bedeutet `25.3 C`. |
| `X100` | Wert mal 100; `5050` bedeutet `50.50 percent`. |

## Live-Verhalten

| Bereich | Wann wird geschrieben? |
| --- | --- |
| BQ25895 rote/gruene Schalter | Sofort beim Klick. |
| BQ25895 Zahlenfelder | Live, wenn das Feld verlassen wird oder Enter/Change ausgeloest wird. |
| BQ25895 Select-Felder | Live beim Aendern. |
| `Apply BQ25895 Settings` | Schreibt nur die geaenderten BQ25895-Werte. |
| BQ27441 rote/gruene Schalter | Sofort beim Klick. |
| `gpoutMode` und `temperatureSource` | Mit `Apply BQ27441 Options`. |
| BQ27441 Battery Profile | Nur mit `Write Battery Profile`. |
| Action-Buttons | Sofort beim Klick. |
| Riskante Actions bei `Expert Locked` | Erst nach Bestaetigung. |

## BQ25895 Uebersichtskarten

| Sichtbarer Begriff | Was es zeigt |
| --- | --- |
| `Charge State` | Aktueller Ladezustand vom BQ25895. |
| `Charge fault: ...` | Aktueller Ladefehler als Kurztext. |
| `Input Voltage` | Aktuelle Spannung am USB/VBUS-Eingang. |
| `Input type` | Erkanntes Eingangstyp-Ergebnis, zum Beispiel USB oder OTG. |
| `Battery Voltage` | Aktuelle Akku-Spannung, gemessen vom Charger. |
| `System: ... mV` | Aktuelle SYS-Ausgangsspannung vom Power-Path. |
| `Charge Current` | Aktueller Ladestrom, gemessen vom Charger-ADC. |
| `Measured by charger ADC` | Hinweis: Wert kommt vom BQ25895-ADC, nicht vom BQ27441. |

## Charge Diagnosis

| Anzeige | Was es bedeutet |
| --- | --- |
| `BQ25895 not found` | Charger antwortet nicht auf I2C. |
| `Charging blocked by CE` | `CE Pin Enabled` ist OFF und blockiert Laden. |
| `Charging disabled in software` | `Charging Enabled` ist OFF. |
| `Input is in high impedance` | `High Impedance` ist ON und reduziert/trennt den Eingang. |
| `OTG boost is active` | OTG ist ON; Akku versorgt VBUS statt normal zu laden. |
| `No valid USB input` | `Power Good` ist No. |
| `Charge fault`, `NTC temperature fault`, `Watchdog fault`, `Battery fault` | Der jeweilige Fehler blockiert oder stoert Laden. |
| `Input voltage limit is active` | VINDPM regelt; der Chip reduziert Strom, um VBUS zu halten. |
| `Input current limit is active` | Eingangsstromlimit ist erreicht. |
| `Precharge is working` | Akku ist tief, BQ27441 sieht Strom in den Akku. Das ist normal. |
| `Fast charge is active` | Normaler Ladebereich ist aktiv. |
| `Charge complete` | Ladeende erkannt. |

## BQ25895 Charge State Werte

| Wert | Bedeutung |
| --- | --- |
| `not charging` | Akku wird gerade nicht geladen. |
| `precharge` | Akku ist tief entladen und wird mit kleinem Strom vorgeladen. |
| `fast charge` | Akku wird normal geladen. |
| `done` | Akku ist voll und Ladeende wurde erkannt. |

## BQ25895 Input Type Werte

| Wert | Bedeutung |
| --- | --- |
| `none` | Kein gueltiger Eingang erkannt. |
| `USB SDP` | Standard-USB-Port, meistens niedriger Strom. |
| `USB CDP` | Charging Downstream Port, USB mit mehr Strom. |
| `USB DCP` | Dedicated Charging Port, Ladegeraet. |
| `MaxCharge` | MaxCharge-Adaptererkennung aktiv/erkannt. |
| `unknown` | Eingang vorhanden, Typ nicht eindeutig erkannt. |
| `non-standard` | Nicht-Standard-Adapter erkannt. |
| `OTG` | Chip ist im Boost/OTG-Modus. |

## BQ25895 Readings

| Sichtbarer Begriff | Was es zeigt |
| --- | --- |
| `Power Good` | Eingang ist gueltig und kann den Power-Path versorgen. |
| `Input Current Limit` | Effektiv aktives Eingangsstromlimit. |
| `Input Current Limited` | Eingangsstromlimit wird gerade erreicht; der Chip begrenzt Strom. |
| `Input Voltage Limited` | Eingangsspannung faellt; der Chip reduziert Strom. |
| `Thermal Regulation` | Der BQ25895 ist warm und reduziert den Ladestrom. |
| `TS / NTC` | Wert vom Akku-Temperatur-Eingang TS als Prozentwert. |

## BQ25895 Faults

| Sichtbarer Begriff | Was es bedeutet |
| --- | --- |
| `Charge Fault` | Ladefehler vom BQ25895. |
| `normal` | Kein Ladefehler. |
| `input` | Eingangsproblem, zum Beispiel VBUS schlecht oder nicht stabil. |
| `thermal shutdown` | Chip hat wegen Temperatur abgeschaltet. |
| `safety timer` | Sicherheits-Ladezeit wurde ueberschritten. |
| `NTC Fault` | Akku-Temperatur ueber TS/NTC ist ausserhalb der erlaubten Grenze. |
| `buck cold` | Akku ist fuer Laden im Buck-Modus zu kalt. |
| `buck hot` | Akku ist fuer Laden im Buck-Modus zu warm. |
| `boost cold` | Akku ist fuer OTG/Boost zu kalt. |
| `boost hot` | Akku ist fuer OTG/Boost zu warm. |
| `Watchdog Fault` | Watchdog ist abgelaufen. |
| `Boost Fault` | Fehler im OTG-Boost-Betrieb. |
| `Battery Fault` | Akku-Fehler vom BQ25895. |

## Actual BQ25895 Settings

| Sichtbarer Begriff | Was es zeigt |
| --- | --- |
| `Charge Current` | Wirklich im Chip gesetzter maximaler Ladestrom. |
| `Charge Voltage` | Wirklich im Chip gesetzte Ziel-Ladespannung. |
| `Input Limit` | Wirklich im Chip gesetztes Eingangsstromlimit. |
| `Input Voltage Limit` | Wirklich im Chip gesetzte VINDPM-Eingangsspannungsgrenze. |
| `Absolute VINDPM` | Zeigt, ob der BQ25895 eine feste VINDPM-Schwelle nutzt. |
| `VINDPM Offset` | Wirklich gesetzter Offset fuer dynamische VINDPM-Regelung. |
| `Watchdog` | Wirklich im Chip gesetzter Watchdog-Timer. |
| `Safety Timer` | Wirklich im Chip gesetzte maximale Ladezeit. |
| `Thermal Limit` | Wirklich im Chip gesetzte Temperatur-Regelgrenze. |

## BQ25895 Settings: Schalter

| Sichtbarer Begriff | Variablenname | Was es macht |
| --- | --- | --- |
| `CE Pin Enabled` | `cePinEnabled` | Schaltet den optional angeschlossenen CE-Host-Pin frei. OFF kann Laden hardwareseitig sperren. |
| `Charging Enabled` | `chargingEnabled` | Schaltet das Laden im BQ25895 per Software ein oder aus. |
| `OTG Boost Enabled` | `otgEnabled` | Macht aus Akku-Spannung eine VBUS/OTG-Ausgangsspannung. Beim Laden normalerweise OFF. |
| `ADC Continuous` | `adcContinuous` | Laesst den BQ25895 dauerhaft Messwerte aktualisieren. |
| `High Impedance` | `highImpedanceMode` | Trennt/reduziert den Eingangspfad. ON kann Laden sofort stoppen. |
| `ILIM Pin Enabled` | `ilimPinEnabled` | Nutzt den ILIM-Pin als zusaetzliche analoge Eingangsstromgrenze. |
| `Charge Termination` | `terminationEnabled` | Erlaubt Ladeende, wenn der Strom unter `terminationCurrentMa` faellt. |
| `Safety Timer Enabled` | `safetyTimerEnabled` | Aktiviert den maximalen Ladezeit-Timer. |
| `Slow Timer In DPM` | `safetyTimerSlowedInDpm` | Verlangsamt den Safety Timer, wenn Input/Voltage/Thermal-Limits aktiv sind. |
| `Auto DPDM` | `autoDpdmEnabled` | Chip erkennt USB-Adapter automatisch ueber D+ und D-. Kann Stromlimit automatisch aendern. |
| `HVDCP` | `hvdcpEnabled` | Erlaubt QuickCharge-artige Hochspannungs-Erkennung. |
| `MaxCharge` | `maxChargeEnabled` | Erlaubt MaxCharge-Adaptererkennung. |
| `ICO` | `icoEnabled` | Chip sucht automatisch ein stabiles Eingangsstromlimit. Kann dein gesetztes Limit ueberstimmen. |
| `Battery Load Test` | `batteryLoadEnabled` | Aktiviert einen kurzen internen Akku-Lasttest. |
| `PumpX` | `pumpXEnabled` | Erlaubt PumpX-Spannungsverhandlung mit kompatiblen Adaptern. |
| `Absolute VINDPM` | `absoluteVindpm` | Nutzt eine feste Eingangsspannungsgrenze statt dynamischer/relativer Grenze. |
| `BATFET Reset Enable` | `batfetResetEnabled` | Erlaubt BATFET-Reset-Verhalten vom Chip. |

## BQ25895 Current And Voltage

| Sichtbarer Begriff | Wertebereich im Web UI | Was es macht |
| --- | --- | --- |
| `inputCurrentLimitMa` | 100 bis 3250 mA | Maximaler Strom, den das Modul aus VBUS ziehen darf. |
| `inputVoltageLimitMv` | 2600 bis 15300 mV | Unter diese Eingangsspannung soll VBUS nicht fallen; der Chip reduziert dann Strom. |
| `vindpmOffsetMv` | 0 bis 3100 mV | Zusatz-Abstand fuer dynamische Eingangsspannungs-Regelung. |
| `chargeCurrentMa` | 0 bis 5056 mA | Maximaler Ladestrom in den Akku. 0 bedeutet praktisch kein Fast-Charge-Strom. |
| `prechargeCurrentMa` | 64 bis 1024 mA | Kleiner Strom fuer tief entladene Akkus. |
| `terminationCurrentMa` | 64 bis 1024 mA | Unter diesem Ladestrom gilt der Akku als voll, wenn Termination aktiv ist. |
| `chargeVoltageMv` | 3840 bis 4608 mV | Zielspannung fuer den vollen Akku. Fuer normale 1S Li-Ion meistens ca. 4200 mV. |
| `minSystemVoltageMv` | 3000 bis 3700 mV | Mindestspannung am SYS-Ausgang vom Power-Path. |

## BQ25895 Boost And Thermal

| Sichtbarer Begriff | Werte | Was es macht |
| --- | --- | --- |
| `boostVoltageMv` | 4550 bis 5510 mV | Ausgangsspannung im OTG-Boost-Modus. |
| `boostFrequency` | `1500 kHz`, `500 kHz` | Schaltfrequenz im OTG-Boost-Modus. |
| `boostHotThreshold` | `34.75 percent`, `37.75 percent`, `31.25 percent`, `Disabled` | Warme TS/NTC-Grenze fuer Boost. |
| `boostColdThreshold` | `77 percent`, `80 percent` | Kalte TS/NTC-Grenze fuer Boost. |
| `thermalRegulation` | `60 C`, `80 C`, `100 C`, `120 C` | Chip-Temperatur, ab der der Ladestrom reduziert wird. |
| `batteryLowThreshold` | `2800 mV`, `3000 mV` | Akku-Spannung fuer Low-Battery-Entscheidungen im Charger. |
| `rechargeThreshold` | `100 mV below full`, `200 mV below full` | Wie weit Akku-Spannung unter Vollspannung fallen muss, bevor Nachladen startet. |

## BQ25895 Safety And Compensation

| Sichtbarer Begriff | Wertebereich | Was es macht |
| --- | --- | --- |
| `watchdog` | `Off`, `40 seconds`, `80 seconds`, `160 seconds` | Wenn aktiv, muss der Host den Chip regelmaessig bedienen, sonst setzt der BQ25895 Einstellungen zurueck. |
| `safetyTimer` | `5 hours`, `8 hours`, `12 hours`, `20 hours` | Maximale Ladezeit, bevor ein Safety-Timer-Fehler gesetzt wird. |
| `irCompResistanceMohm` | 0 bis 140 mOhm | Geschaetzter Widerstand von Akku-Pfad/Kabel fuer Spannungs-Kompensation. |
| `irCompVoltageClampMv` | 0 bis 224 mV | Maximale Zusatzspannung, die durch IR-Kompensation erlaubt ist. |

## BQ25895 Buttons

| Sichtbarer Begriff | Was es macht |
| --- | --- |
| `Apply BQ25895 Settings` | Schreibt die geaenderten Charger-Werte in den BQ25895 und speichert sie im ESP32. |
| `Force DPDM` | Startet USB-Adaptererkennung sofort. |
| `Force ICO` | Startet automatische Eingangsstrom-Optimierung sofort. |
| `PumpX Up` | Fordert bei kompatiblem Adapter eine hoehere Eingangsspannung an. |
| `PumpX Down` | Fordert bei kompatiblem Adapter eine niedrigere Eingangsspannung an. |
| `Ship Mode` | Schaltet BATFET fuer Lagerung/Transport ab; Akku-Pfad kann abgeschaltet werden. |
| `Reset BQ25895` | Setzt den Charger zurueck und schreibt gespeicherte Charger-Settings neu. |

## Safe Charge Startwerte

| Einstellung | Wert |
| --- | --- |
| `CE Pin Enabled` | ON |
| `Charging Enabled` | ON |
| `High Impedance` | OFF |
| `OTG Boost Enabled` | OFF |
| `inputCurrentLimitMa` | 1000 mA |
| `chargeCurrentMa` | 500 mA |
| `prechargeCurrentMa` | 128 mA |
| `chargeVoltageMv` | 4208 mV |
| `Auto DPDM` / `ICO` / `HVDCP` / `MaxCharge` | OFF |
| `Absolute VINDPM` | ON |
| `inputVoltageLimitMv` | 4400 mV |
| `watchdog` | Off |

`Safe Charge` ist ein Debug-Startpunkt, nicht die maximal moegliche Ladegeschwindigkeit.

## BQ27441 Uebersichtskarten

| Sichtbarer Begriff | Was es zeigt |
| --- | --- |
| `State Of Charge` | Akku-Prozent laut Fuel Gauge. |
| `Unfiltered` | Akku-Prozent ohne Gauge-Glaettung. |
| `Battery Voltage` | Akku-Spannung laut Fuel Gauge. |
| `Average current` | Durchschnittlicher Akku-Strom laut Fuel Gauge. |
| `Temperature` | Akku-Temperatur laut aktivierter Temperaturquelle. |
| `Internal` | Interne Temperatur vom Gauge-Chip. |
| `Health` | Akku-Gesundheit in Prozent. |
| `State of health` | Hinweistext fuer Health. |

## BQ27441 Readings

| Sichtbarer Begriff | Was es zeigt |
| --- | --- |
| `Remaining Capacity` | Noch verfuegbare Kapazitaet in mAh. |
| `Full Capacity` | Vom Gauge gelernte volle Kapazitaet in mAh. |
| `Average Power` | Durchschnittliche Akku-Leistung in mW. |
| `Standby Current` | Geschaetzter Strom fuer Standby-Zustand. |
| `Max Load Current` | Geschaetzter maximaler Laststrom. |

## BQ27441 Flags

| Sichtbarer Begriff | Was es bedeutet |
| --- | --- |
| `Battery Detected` | Gauge erkennt, dass ein Akku eingesetzt ist. |
| `Charging` | Gauge sieht Strom in den Akku. |
| `Discharging` | Gauge sieht Strom aus dem Akku. |
| `Full Charge` | Gauge meldet Akku voll. |
| `Low Battery` | SOC1-Schwelle ist aktiv. |
| `Final Low Battery` | SOCF-Schwelle ist aktiv; kritischer Low-Battery-Status. |
| `Power On Reset` | Gauge hatte seit letztem Clear einen Reset/Power-On. |

## BQ27441 State

| Sichtbarer Begriff | Was es bedeutet |
| --- | --- |
| `Sealed` | Gauge-Konfiguration ist geschuetzt/gesperrt. |
| `Hibernate` | Gauge ist im Hibernate-Modus. |
| `Sleep` | Gauge ist im Sleep-Modus. |
| `Ready` | Gauge-Initialisierung ist fertig. |
| `Voltage OK` | Akku-Spannung ist fuer den Gauge plausibel/gueltig. |
| `Shutdown Enabled` | Shutdown wurde freigegeben; naechster Shutdown-Befehl kann abschalten. |

## BQ27441 Battery Profile

Diese Werte beschreiben den Akku fuer die Kapazitaetsrechnung vom BQ27441. Falsche Werte machen die Prozentanzeige falsch.

| Sichtbarer Begriff | Wertebereich im Web UI | Was es macht |
| --- | --- | --- |
| `designCapacityMah` | 1 bis 30000 mAh | Nennkapazitaet vom Akku. |
| `designEnergyMwh` | 1 bis 60000 mWh | Nennenergie vom Akku. |
| `terminateVoltageMv` | 2500 bis 3700 mV | Untere Entladespannung fuer die Gauge-Rechnung. |
| `taperRate` | 1 bis 2000 | Gauge-Parameter fuer Ladeende-Erkennung. |
| `soc1SetPercent` | 0 bis 100 % | Prozentwert, bei dem `Low Battery` gesetzt wird. |
| `soc1ClearPercent` | 0 bis 100 % | Prozentwert, bei dem `Low Battery` wieder geloescht wird. |
| `socfSetPercent` | 0 bis 100 % | Prozentwert, bei dem `Final Low Battery` gesetzt wird. |
| `socfClearPercent` | 0 bis 100 % | Prozentwert, bei dem `Final Low Battery` wieder geloescht wird. |
| `socIntDeltaPercent` | 1 bis 100 % | Prozent-Schritt fuer SOC-Interrupt am GPOUT. |

## BQ27441 Battery Profile Buttons

| Sichtbarer Begriff | Was es macht |
| --- | --- |
| `Write Battery Profile` | Schreibt das Akku-Profil in den BQ27441 und speichert es im ESP32. |
| `Read Battery Profile` | Liest das aktuell im BQ27441 gespeicherte Akku-Profil. |
| `No profile read yet.` | Es wurde seit Start noch kein Profil aus dem Gauge gelesen. |
| `Read from gauge: ...` | Zeigt das zuletzt aus dem Gauge gelesene Profil. |

## BQ27441 Options

| Sichtbarer Begriff | Variablenname | Was es macht |
| --- | --- | --- |
| `gpoutMode` | `gpoutMode` | Waehlt die Funktion vom GPOUT-Pin. |
| `Battery low` | `GpoutBatLow` | GPOUT meldet Low-Battery. |
| `SOC interrupt` | `GpoutSocInt` | GPOUT pulst/meldet bei SOC-Aenderung. |
| `temperatureSource` | `temperatureSource` | Waehlt, woher der Gauge seine Temperatur bekommt. |
| `Internal sensor` | `TemperatureFromInternal` | Gauge nutzt seinen internen Temperatursensor. |
| `Host provided` | `TemperatureFromHost` | Host muss Temperatur liefern; im Web-UI-Beispiel gibt es dafuer keinen Extra-Eingabewert. |
| `GPOUT Active High` | `gpoutActiveHigh` | GPOUT ist aktiv bei HIGH statt LOW. |
| `Battery Insertion Detection` | `batteryInsertionEnabled` | Gauge erkennt Akku-Einsetzen automatisch. |
| `BIN Pullup` | `binPullupEnabled` | Aktiviert Pullup am BIN-Pin. |
| `Sleep Enabled` | `sleepEnabled` | Erlaubt dem Gauge Sleep-Modus fuer weniger Verbrauch. |
| `Capacity Smoothing` | `rmFccSmoothingEnabled` | Glaettet Remaining Capacity und Full Capacity. |

## BQ27441 Option Buttons

| Sichtbarer Begriff | Was es macht |
| --- | --- |
| `Apply BQ27441 Options` | Schreibt `gpoutMode`, `temperatureSource` und die Option-Schalter in den Gauge. |
| `Pulse GPOUT` | Gibt einen kurzen Puls am GPOUT-Pin aus. |
| `Battery Insert` | Meldet dem Gauge manuell, dass ein Akku eingesetzt ist. |
| `Battery Remove` | Meldet dem Gauge manuell, dass der Akku entfernt ist. |

## BQ27441 Reset And Power

| Sichtbarer Begriff | Was es macht |
| --- | --- |
| `Clear Hibernate` | Holt den Gauge aus Hibernate. |
| `Set Hibernate` | Setzt den Gauge in Hibernate. |
| `Unseal` | Entsperrt die geschuetzte Gauge-Konfiguration mit Default-Key. |
| `Seal` | Sperrt die Gauge-Konfiguration wieder. |
| `Enable Shutdown` | Erlaubt einen folgenden Shutdown-Befehl. |
| `Shutdown` | Schaltet den Gauge ab. |
| `Soft Reset` | Startet den Gauge logisch neu. |
| `Reset BQ27441` | Fuehrt einen Gauge-Reset aus. |

## Was Laden stoppen kann

Diese Web-UI-Funktionen koennen Laden absichtlich stoppen oder veraendern:

- `CE Pin Enabled` auf OFF.
- `Charging Enabled` auf OFF.
- `High Impedance` auf ON.
- `OTG Boost Enabled` auf ON, weil Boost und Laden nicht gleichzeitig normal laufen.
- `Ship Mode`, weil BATFET abgeschaltet werden kann.
- Zu kleines `inputCurrentLimitMa`.
- Zu hohes `chargeCurrentMa` fuer Adapter, Kabel, PCB oder Temperatur.
- `Auto DPDM` oder `ICO`, weil der Chip dann Limits selbst anpassen kann.
- `Thermal Regulation`, `Input Current Limited` oder `Input Voltage Limited`, weil der Chip dann den Ladestrom reduziert.

## Sinnvolle Test-Reihenfolge

1. `Power Good` muss `Yes` sein.
2. `Charging Enabled` und `CE Pin Enabled` muessen ON sein.
3. `High Impedance` muss OFF sein.
4. `OTG Boost Enabled` muss OFF sein, wenn geladen werden soll.
5. `Input Current Limit` pruefen.
6. `Charge Fault` und `NTC Fault` muessen `normal` sein.
7. Wenn der Adapter staerker sein soll: `HVDCP` und `MaxCharge` aktivieren, dann `Force DPDM` testen.

## English

This documentation belongs to the `examples/web_debug_ui` example. The example runs on an ESP32 and uses the ESP32 only as a host for the Batthub board. The Batthub board itself has no ESP32.

## Control Logic

| Web UI term | Meaning |
| --- | --- |
| `Batthub Web UI` | Browser interface for live testing and configuration of BQ25895 and BQ27441. |
| `Connecting...` | Browser is waiting for data from the ESP32. |
| `Connection error` | Browser cannot currently reach the ESP32 web server. |
| `Not found` | The selected BQ chip does not answer on I2C. |
| `ON` / `OFF` | Switch state; green is ON, red is OFF. |
| `Yes` / `No` | Status state, not a switch. |
| `Expert Locked` | Normal guarded mode. Risky actions ask for confirmation before they are written. |
| `Expert Unlocked` | Expert mode. Risky actions remain available and ask less often. |
| `Safe Charge` | Writes a conservative stable charging baseline: charging on, OTG/HIZ off, fixed VINDPM, no automatic adapter optimization. |
| `Reset Both` | Resets BQ25895 and BQ27441 and then writes saved charger settings again. |
| `BQ25895 Charger` | Tab for charging, USB input, power-path, OTG, and charger faults. |
| `BQ27441 Gauge` | Tab for battery percent, battery profile, fuel-gauge status, and gauge options. |

## Units

| Suffix | Unit |
| --- | --- |
| `Ma` | mA, milliamps. |
| `Mv` | mV, millivolts. |
| `Mw` | mW, milliwatts. |
| `Mwh` | mWh, milliwatt-hours. |
| `Mohm` | mOhm, milliohms. |
| `Percent` | Percent. |
| `X10` | Value times 10; `253` means `25.3 C`. |
| `X100` | Value times 100; `5050` means `50.50 percent`. |

## Live Behavior

| Area | When is it written? |
| --- | --- |
| BQ25895 red/green switches | Immediately on click. |
| BQ25895 number fields | Live when the field loses focus or triggers Enter/change. |
| BQ25895 select fields | Live when changed. |
| `Apply BQ25895 Settings` | Writes only changed BQ25895 values. |
| BQ27441 red/green switches | Immediately on click. |
| `gpoutMode` and `temperatureSource` | With `Apply BQ27441 Options`. |
| BQ27441 Battery Profile | Only with `Write Battery Profile`. |
| Action buttons | Immediately on click. |
| Risky actions while `Expert Locked` | Only after confirmation. |

## BQ25895 Overview Cards

| Visible term | Meaning |
| --- | --- |
| `Charge State` | Current BQ25895 charging state. |
| `Charge fault: ...` | Current charge fault as short text. |
| `Input Voltage` | Current USB/VBUS input voltage. |
| `Input type` | Detected input type, for example USB or OTG. |
| `Battery Voltage` | Current battery voltage measured by the charger. |
| `System: ... mV` | Current SYS output voltage from the power-path. |
| `Charge Current` | Current charge current measured by the charger ADC. |
| `Measured by charger ADC` | The value comes from the BQ25895 ADC, not from the BQ27441. |

## Charge Diagnosis

| Display | Meaning |
| --- | --- |
| `BQ25895 not found` | Charger does not answer on I2C. |
| `Charging blocked by CE` | `CE Pin Enabled` is OFF and blocks charging. |
| `Charging disabled in software` | `Charging Enabled` is OFF. |
| `Input is in high impedance` | `High Impedance` is ON and reduces/disconnects the input path. |
| `OTG boost is active` | OTG is ON; battery powers VBUS instead of normal charging. |
| `No valid USB input` | `Power Good` is No. |
| `Charge fault`, `NTC temperature fault`, `Watchdog fault`, `Battery fault` | The shown fault is blocking or disturbing charge. |
| `Input voltage limit is active` | VINDPM is regulating; the chip reduces current to hold VBUS. |
| `Input current limit is active` | Input current limit has been reached. |
| `Precharge is working` | Battery is low and BQ27441 sees current into the battery. This is normal. |
| `Fast charge is active` | Normal charging range is active. |
| `Charge complete` | Charge termination was detected. |

## BQ25895 Charge State Values

| Value | Meaning |
| --- | --- |
| `not charging` | Battery is not charging right now. |
| `precharge` | Battery is deeply discharged and is charged with a small current. |
| `fast charge` | Battery is charging normally. |
| `done` | Battery is full and termination was detected. |

## BQ25895 Input Type Values

| Value | Meaning |
| --- | --- |
| `none` | No valid input detected. |
| `USB SDP` | Standard USB port, usually low current. |
| `USB CDP` | Charging Downstream Port, USB with more current. |
| `USB DCP` | Dedicated Charging Port, charger. |
| `MaxCharge` | MaxCharge adapter detection active/detected. |
| `unknown` | Input is present but type is not clear. |
| `non-standard` | Non-standard adapter detected. |
| `OTG` | Chip is in boost/OTG mode. |

## BQ25895 Readings and Faults

| Visible term | Meaning |
| --- | --- |
| `Power Good` | Input is valid and can power the power-path. |
| `Input Current Limit` | Effective active input current limit. |
| `Input Current Limited` | Input current limit is reached; the chip limits current. |
| `Input Voltage Limited` | Input voltage is dropping; the chip reduces current. |
| `Thermal Regulation` | The BQ25895 is warm and reduces charge current. |
| `TS / NTC` | Value from the battery temperature input as percent. |
| `Charge Fault` | Charge fault reported by the BQ25895. |
| `normal` | No charge fault. |
| `input` | Input problem, for example unstable VBUS. |
| `thermal shutdown` | Chip shut down because of temperature. |
| `safety timer` | Maximum charge time was exceeded. |
| `NTC Fault` | Battery temperature through TS/NTC is outside allowed range. |
| `buck cold` / `buck hot` | Battery is too cold or too hot for charging. |
| `boost cold` / `boost hot` | Battery is too cold or too hot for OTG boost. |
| `Watchdog Fault` | Watchdog expired. |
| `Boost Fault` | Fault during OTG boost. |
| `Battery Fault` | Battery fault from BQ25895. |

## Actual BQ25895 Settings

| Visible term | Meaning |
| --- | --- |
| `Charge Current` | Maximum charge current actually set in the chip. |
| `Charge Voltage` | Target charge voltage actually set in the chip. |
| `Input Limit` | Input current limit actually set in the chip. |
| `Input Voltage Limit` | VINDPM input-voltage threshold actually set in the chip. |
| `Absolute VINDPM` | Shows whether the BQ25895 uses a fixed VINDPM threshold. |
| `VINDPM Offset` | Actual offset for dynamic VINDPM regulation. |
| `Watchdog` | Watchdog timer actually set in the chip. |
| `Safety Timer` | Maximum charge time actually set in the chip. |
| `Thermal Limit` | Temperature regulation limit actually set in the chip. |

## BQ25895 Settings

| Visible term | Variable | What it does |
| --- | --- | --- |
| `CE Pin Enabled` | `cePinEnabled` | Controls the optional host CE pin. OFF can block charging in hardware. |
| `Charging Enabled` | `chargingEnabled` | Enables or disables charging in the BQ25895. |
| `OTG Boost Enabled` | `otgEnabled` | Turns battery voltage into a VBUS/OTG output voltage. Normally OFF while charging. |
| `ADC Continuous` | `adcContinuous` | Keeps BQ25895 measurements updating continuously. |
| `High Impedance` | `highImpedanceMode` | Reduces/disconnects the input path. ON can stop charging immediately. |
| `ILIM Pin Enabled` | `ilimPinEnabled` | Uses the ILIM pin as an extra analog input-current limit. |
| `Charge Termination` | `terminationEnabled` | Allows charge termination when current falls below `terminationCurrentMa`. |
| `Safety Timer Enabled` | `safetyTimerEnabled` | Enables the maximum charge-time timer. |
| `Slow Timer In DPM` | `safetyTimerSlowedInDpm` | Slows the safety timer while input/voltage/thermal limits are active. |
| `Auto DPDM` | `autoDpdmEnabled` | Lets the chip detect USB adapters through D+ and D-. Can change input limit. |
| `HVDCP` | `hvdcpEnabled` | Allows QuickCharge-style high-voltage adapter detection. |
| `MaxCharge` | `maxChargeEnabled` | Allows MaxCharge adapter detection. |
| `ICO` | `icoEnabled` | Lets the chip search for a stable input current limit. Can override your limit. |
| `Battery Load Test` | `batteryLoadEnabled` | Enables a short internal battery load test. |
| `PumpX` | `pumpXEnabled` | Allows PumpX voltage negotiation with compatible adapters. |
| `Absolute VINDPM` | `absoluteVindpm` | Uses a fixed input-voltage limit instead of a relative/dynamic limit. |
| `BATFET Reset Enable` | `batfetResetEnabled` | Allows BATFET reset behavior from the chip. |

## BQ25895 Numeric and Select Settings

| Visible term | Web UI range | What it does |
| --- | --- | --- |
| `inputCurrentLimitMa` | 100 to 3250 mA | Maximum current the module may draw from VBUS. |
| `inputVoltageLimitMv` | 2600 to 15300 mV | Input voltage floor; the chip reduces current below this. |
| `vindpmOffsetMv` | 0 to 3100 mV | Extra margin for dynamic input-voltage regulation. |
| `chargeCurrentMa` | 0 to 5056 mA | Maximum charge current into the battery. |
| `prechargeCurrentMa` | 64 to 1024 mA | Small current for deeply discharged batteries. |
| `terminationCurrentMa` | 64 to 1024 mA | Charge can finish below this current when termination is enabled. |
| `chargeVoltageMv` | 3840 to 4608 mV | Target full battery voltage. Normal 1S Li-Ion/LiPo is often around 4200 mV. |
| `minSystemVoltageMv` | 3000 to 3700 mV | Minimum SYS output voltage from the power-path. |
| `boostVoltageMv` | 4550 to 5510 mV | Output voltage in OTG boost mode. |
| `boostFrequency` | `1500 kHz`, `500 kHz` | Switching frequency in OTG boost mode. |
| `boostHotThreshold` | Listed percent values or disabled | Hot TS/NTC threshold for boost. |
| `boostColdThreshold` | `77 percent`, `80 percent` | Cold TS/NTC threshold for boost. |
| `thermalRegulation` | `60 C`, `80 C`, `100 C`, `120 C` | Chip temperature where charge current is reduced. |
| `batteryLowThreshold` | `2800 mV`, `3000 mV` | Battery voltage used for charger low-battery decisions. |
| `rechargeThreshold` | `100 mV below full`, `200 mV below full` | Voltage drop below full before recharge starts. |
| `watchdog` | `Off`, `40 seconds`, `80 seconds`, `160 seconds` | If active, the host must service the chip or settings reset. |
| `safetyTimer` | `5 hours`, `8 hours`, `12 hours`, `20 hours` | Maximum charge time before safety timer fault. |
| `irCompResistanceMohm` | 0 to 140 mOhm | Estimated battery-path resistance for voltage compensation. |
| `irCompVoltageClampMv` | 0 to 224 mV | Maximum voltage added by IR compensation. |

## BQ25895 Buttons

| Visible term | Meaning |
| --- | --- |
| `Apply BQ25895 Settings` | Writes changed charger values to the BQ25895 and saves them on the ESP32. |
| `Force DPDM` | Starts USB adapter detection immediately. |
| `Force ICO` | Starts input-current optimization immediately. |
| `PumpX Up` | Requests higher input voltage from a compatible adapter. |
| `PumpX Down` | Requests lower input voltage from a compatible adapter. |
| `Ship Mode` | Turns off BATFET for storage/transport; battery path can be disabled. |
| `Reset BQ25895` | Resets the charger and writes saved charger settings again. |

## Safe Charge Start Values

| Setting | Value |
| --- | --- |
| `CE Pin Enabled` | ON |
| `Charging Enabled` | ON |
| `High Impedance` | OFF |
| `OTG Boost Enabled` | OFF |
| `inputCurrentLimitMa` | 1000 mA |
| `chargeCurrentMa` | 500 mA |
| `prechargeCurrentMa` | 128 mA |
| `chargeVoltageMv` | 4208 mV |
| `Auto DPDM` / `ICO` / `HVDCP` / `MaxCharge` | OFF |
| `Absolute VINDPM` | ON |
| `inputVoltageLimitMv` | 4400 mV |
| `watchdog` | Off |

`Safe Charge` is a debug baseline, not the maximum possible charging speed.

## BQ27441 Cards, Flags, and State

| Visible term | Meaning |
| --- | --- |
| `State Of Charge` | Battery percent from the fuel gauge. |
| `Unfiltered` | Battery percent without gauge smoothing. |
| `Battery Voltage` | Battery voltage from the fuel gauge. |
| `Average current` | Average battery current from the fuel gauge. |
| `Temperature` | Battery temperature from the selected temperature source. |
| `Internal` | Internal gauge chip temperature. |
| `Health` / `State of health` | Battery health in percent. |
| `Remaining Capacity` | Remaining capacity in mAh. |
| `Full Capacity` | Learned full capacity in mAh. |
| `Average Power` | Average battery power in mW. |
| `Standby Current` | Estimated standby current. |
| `Max Load Current` | Estimated maximum load current. |
| `Battery Detected` | Gauge detects a battery. |
| `Charging` | Gauge sees current into the battery. |
| `Discharging` | Gauge sees current out of the battery. |
| `Full Charge` | Gauge reports battery full. |
| `Low Battery` | SOC1 threshold is active. |
| `Final Low Battery` | SOCF threshold is active; critical low-battery state. |
| `Power On Reset` | Gauge had a reset/power-on. |
| `Sealed` | Gauge configuration is protected/locked. |
| `Hibernate` | Gauge is in hibernate mode. |
| `Sleep` | Gauge is in sleep mode. |
| `Ready` | Gauge initialization is complete. |
| `Voltage OK` | Battery voltage is valid for the gauge. |
| `Shutdown Enabled` | Shutdown was armed; the next shutdown command can turn it off. |

## BQ27441 Battery Profile

Wrong profile values make battery percentage wrong.

| Visible term | Web UI range | What it does |
| --- | --- | --- |
| `designCapacityMah` | 1 to 30000 mAh | Rated battery capacity. |
| `designEnergyMwh` | 1 to 60000 mWh | Rated battery energy. |
| `terminateVoltageMv` | 2500 to 3700 mV | End-of-discharge voltage used by the gauge. |
| `taperRate` | 1 to 2000 | Gauge parameter for charge-end detection. |
| `soc1SetPercent` | 0 to 100 % | Percent where `Low Battery` is set. |
| `soc1ClearPercent` | 0 to 100 % | Percent where `Low Battery` is cleared. |
| `socfSetPercent` | 0 to 100 % | Percent where `Final Low Battery` is set. |
| `socfClearPercent` | 0 to 100 % | Percent where `Final Low Battery` is cleared. |
| `socIntDeltaPercent` | 1 to 100 % | Percent step for SOC interrupt on GPOUT. |

## BQ27441 Options and Buttons

| Visible term | Variable | What it does |
| --- | --- | --- |
| `gpoutMode` | `gpoutMode` | Selects the GPOUT pin function. |
| `Battery low` | `GpoutBatLow` | GPOUT reports low battery. |
| `SOC interrupt` | `GpoutSocInt` | GPOUT pulses/reports SOC changes. |
| `temperatureSource` | `temperatureSource` | Selects where the gauge temperature comes from. |
| `Internal sensor` | `TemperatureFromInternal` | Gauge uses its internal temperature sensor. |
| `Host provided` | `TemperatureFromHost` | Host must provide temperature; this Web UI has no extra input for it. |
| `GPOUT Active High` | `gpoutActiveHigh` | GPOUT is active HIGH instead of LOW. |
| `Battery Insertion Detection` | `batteryInsertionEnabled` | Gauge detects battery insertion automatically. |
| `BIN Pullup` | `binPullupEnabled` | Enables pullup on the BIN pin. |
| `Sleep Enabled` | `sleepEnabled` | Allows gauge sleep mode for lower power. |
| `Capacity Smoothing` | `rmFccSmoothingEnabled` | Smooths Remaining Capacity and Full Capacity. |
| `Write Battery Profile` | button | Writes battery profile to the BQ27441 and saves it on the ESP32. |
| `Read Battery Profile` | button | Reads the current battery profile from the BQ27441. |
| `Apply BQ27441 Options` | button | Writes gauge options to the BQ27441. |
| `Pulse GPOUT` | button | Outputs a short pulse on GPOUT. |
| `Battery Insert` | button | Tells the gauge a battery is inserted. |
| `Battery Remove` | button | Tells the gauge the battery was removed. |

## BQ27441 Reset and Power Buttons

| Visible term | Meaning |
| --- | --- |
| `Clear Hibernate` | Leaves hibernate mode. |
| `Set Hibernate` | Enters hibernate mode. |
| `Unseal` | Unlocks protected gauge configuration with the default key. |
| `Seal` | Locks protected gauge configuration again. |
| `Enable Shutdown` | Arms the next shutdown command. |
| `Shutdown` | Turns the gauge off. |
| `Soft Reset` | Restarts the gauge logically. |
| `Reset BQ27441` | Performs a gauge reset. |

## What Can Stop Charging

These Web UI functions can intentionally stop or change charging:

- `CE Pin Enabled` OFF.
- `Charging Enabled` OFF.
- `High Impedance` ON.
- `OTG Boost Enabled` ON, because boost and normal charging do not run together.
- `Ship Mode`, because BATFET can be turned off.
- `inputCurrentLimitMa` too low.
- `chargeCurrentMa` too high for adapter, cable, PCB, or temperature.
- `Auto DPDM` or `ICO`, because the chip can change limits automatically.
- `Thermal Regulation`, `Input Current Limited`, or `Input Voltage Limited`, because the chip reduces current.

## Useful Test Order

1. `Power Good` must be `Yes`.
2. `Charging Enabled` and `CE Pin Enabled` must be ON.
3. `High Impedance` must be OFF.
4. `OTG Boost Enabled` must be OFF when charging is wanted.
5. Check `Input Current Limit`.
6. `Charge Fault` and `NTC Fault` should be `normal`.
7. For stronger adapters: enable `HVDCP` and `MaxCharge`, then test `Force DPDM`.
