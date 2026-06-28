# Batthub Charging Troubleshooting

## Deutsch

Diese Seite ist fuer Nutzer, die nicht wissen, ob das Problem am USB-Netzteil, am Akku, am BQ25895, am BQ27441 oder an einer Web-UI-Einstellung liegt.

## Erstes Vorgehen

1. `Safe Charge` druecken.
2. Browser neu laden.
3. Im Tab `BQ25895 Charger` die Karte `Charge Diagnosis` lesen.
4. Im Tab `BQ27441 Gauge` `Average current` ansehen.

Wenn `Average current` positiv ist, sieht der BQ27441 Strom in den Akku. Dann wird geladen, auch wenn der BQ25895-ADC zeitweise `0 mA` zeigt.

## Safe-Charge-Startwerte

| Wert | Safe-Charge-Start |
| --- | --- |
| `CE Pin Enabled` | ON |
| `Charging Enabled` | ON |
| `High Impedance` | OFF |
| `OTG Boost Enabled` | OFF |
| `inputCurrentLimitMa` | 1000 mA |
| `chargeCurrentMa` | 500 mA |
| `prechargeCurrentMa` | 128 mA |
| `chargeVoltageMv` | 4208 mV |
| `Auto DPDM` | OFF |
| `ICO` | OFF |
| `HVDCP` | OFF |
| `MaxCharge` | OFF |
| `Absolute VINDPM` | ON |
| `inputVoltageLimitMv` | 4400 mV |
| `watchdog` | Off |

Diese Werte sind nicht fuer maximale Ladegeschwindigkeit gedacht. Sie sind ein stabiler Ausgangspunkt zum Testen.

## Warum laedt er langsam?

| Anzeige | Bedeutung |
| --- | --- |
| `precharge` | Akku ist tief entladen. Der BQ25895 nimmt absichtlich nur kleinen Strom. |
| `fast charge` | Normaler Ladebereich. Der gesetzte `chargeCurrentMa` kann erreicht werden, wenn Eingang, Akku und Temperatur passen. |
| `done` | Ladeende erkannt. |
| `Input Current Limited: Yes` | Der gesetzte oder automatisch erkannte Eingangsstrom ist erreicht. |
| `Input Voltage Limited: Yes` | VBUS faellt an die VINDPM-Grenze; der BQ reduziert Strom. |
| `Thermal Regulation: Yes` | Der Chip ist warm und reduziert Strom. |
| `NTC Fault` | Akku-Temperaturpfad ist ausserhalb des erlaubten Bereichs. |
| `Watchdog Fault` | Watchdog ist abgelaufen; Einstellungen koennen zurueckgesetzt worden sein. |

## BQ25895 und BQ27441 zeigen verschiedene Stroeme

Der `BQ25895` misst Ladestrom mit seinem Charger-ADC. Der Wert kann klein, langsam oder zeitweise `0 mA` sein.

Der `BQ27441` misst den Akku-Strom als Fuel Gauge. Fuer die Frage "geht Strom in die Batterie?" ist `BQ27441 Average current` oft die bessere Anzeige:

- positiv: Strom geht in den Akku.
- negativ: Strom kommt aus dem Akku.
- nahe 0: praktisch kein Akku-Strom.

## Was kann Laden direkt stoppen?

| Einstellung | Risiko |
| --- | --- |
| `CE Pin Enabled` OFF | Hardware-Enable blockiert Laden. |
| `Charging Enabled` OFF | Software-Laden ist aus. |
| `High Impedance` ON | Eingangspfad wird reduziert oder getrennt. |
| `OTG Boost Enabled` ON | Akku versorgt VBUS; normales Laden ist nicht aktiv. |
| `Ship Mode` | BATFET kann abgeschaltet werden; Wake/Replug kann noetig sein. |
| `Safety Timer Enabled` OFF | Sicherheits-Ladezeit ist deaktiviert. |
| `Charge Termination` OFF | Normales Ladeende kann ausbleiben. |
| `Auto DPDM` / `ICO` ON | Chip darf Eingangslimit selbst aendern. |
| `HVDCP` / `MaxCharge` / `PumpX` | Adapter-Verhandlung kann Eingangsspannung oder Erkennung aendern. |

## Was bedeutet `Input Voltage Limited` bei gutem Netzteil?

`Input Voltage Limited` bedeutet nicht automatisch, dass das Netzteil schlecht ist. Der BQ25895 meldet nur: "Ich bin an meiner VINDPM-Regelgrenze."

Pruefe diese Werte zusammen:

| Wert | Erwartung fuer normale 5-V-Ladung |
| --- | --- |
| `Input Voltage` | deutlich ueber `inputVoltageLimitMv`, z. B. 5100 mV |
| `Actual Input Voltage Limit` | z. B. 4400 mV |
| `Actual Absolute VINDPM` | Yes |
| `Input Voltage Limited` | No |

Wenn `Input Voltage` deutlich ueber `Actual Input Voltage Limit` liegt und trotzdem `Input Voltage Limited` aktiv bleibt, dann ist es kein normales Netzteil-Limit. Dann koennen ein kurzer Spannungseinbruch, alte/stale Messwerte, falsche VINDPM-Schreibreihenfolge oder ein Softwarefehler die Ursache sein. In der Web UI: `Safe Charge` druecken und die `Actual`-Werte erneut pruefen.

## Warum ist `precharge` kein Fehler?

Bei tief entladenem Akku laedt der BQ25895 absichtlich mit kleinem Strom. Das schuetzt die Zelle. Erst wenn die Akku-Spannung hoch genug ist, wechselt der Chip nach `fast charge`.

Typisches Bild:

| Anzeige | Beispiel |
| --- | --- |
| `Battery Voltage` | 2500 bis 3000 mV |
| `Charge State` | `precharge` |
| `BQ27441 Average current` | positiv, aber klein |

Das ist normal, solange die Zellspannung langsam steigt und kein NTC-/Battery-/Safety-Fehler aktiv ist.

## Deutsch: schnelle Fehlerreihenfolge

1. `BQ25895 present` und `BQ27441 present` pruefen.
2. `Safe Charge` druecken.
3. `Power Good` muss `Yes` sein.
4. `CE Pin Enabled`, `Charging Enabled` ON.
5. `High Impedance`, `OTG Boost Enabled` OFF.
6. `Charge Fault` und `NTC Fault` muessen `normal` sein.
7. `Actual Absolute VINDPM` muss `Yes` sein.
8. `Actual Input Voltage Limit` sollte bei 5-V-USB z. B. `4400 mV` sein.
9. Bei tiefem Akku ist `precharge` normal.
10. Zum echten Akku-Strom den `BQ27441 Average current` ansehen.

## English

This page is for users who do not yet know whether a charging issue comes from the USB adapter, battery, BQ25895, BQ27441, or a Web UI setting.

## First Steps

1. Press `Safe Charge`.
2. Reload the browser.
3. Read `Charge Diagnosis` in the `BQ25895 Charger` tab.
4. Check `Average current` in the `BQ27441 Gauge` tab.

If `Average current` is positive, the BQ27441 sees current going into the battery. Charging is happening, even if the BQ25895 charger ADC sometimes shows `0 mA`.

## Safe Charge Start Values

| Value | Safe charge start |
| --- | --- |
| `CE Pin Enabled` | ON |
| `Charging Enabled` | ON |
| `High Impedance` | OFF |
| `OTG Boost Enabled` | OFF |
| `inputCurrentLimitMa` | 1000 mA |
| `chargeCurrentMa` | 500 mA |
| `prechargeCurrentMa` | 128 mA |
| `chargeVoltageMv` | 4208 mV |
| `Auto DPDM` | OFF |
| `ICO` | OFF |
| `HVDCP` | OFF |
| `MaxCharge` | OFF |
| `Absolute VINDPM` | ON |
| `inputVoltageLimitMv` | 4400 mV |
| `watchdog` | Off |

These values are not meant for maximum charging speed. They are a stable debug baseline.

## Why Charging Can Be Slow

| Display | Meaning |
| --- | --- |
| `precharge` | Battery is deeply discharged. The BQ25895 intentionally uses a small current. |
| `fast charge` | Normal charge range. The configured `chargeCurrentMa` can be reached if input, battery, and temperature allow it. |
| `done` | Charge termination detected. |
| `Input Current Limited: Yes` | The configured or detected input current limit has been reached. |
| `Input Voltage Limited: Yes` | VBUS reached the VINDPM limit; the BQ reduces current. |
| `Thermal Regulation: Yes` | The charger is warm and reduces current. |
| `NTC Fault` | Battery temperature path is outside the allowed range. |
| `Watchdog Fault` | Watchdog expired; settings may have been reset. |

## BQ25895 and BQ27441 Currents Are Different

The `BQ25895` measures charge current with the charger ADC. This value can be small, slow, or sometimes `0 mA`.

The `BQ27441` measures battery current as the fuel gauge. For "is current going into the battery?", `BQ27441 Average current` is often the better indicator:

- positive: current goes into the battery.
- negative: current comes out of the battery.
- near 0: almost no battery current.

## Settings That Can Stop Charging

| Setting | Risk |
| --- | --- |
| `CE Pin Enabled` OFF | Hardware enable blocks charging. |
| `Charging Enabled` OFF | Software charging is off. |
| `High Impedance` ON | Input path is reduced or disconnected. |
| `OTG Boost Enabled` ON | Battery powers VBUS; normal charging is not active. |
| `Ship Mode` | BATFET can be turned off; wake/replug may be needed. |
| `Safety Timer Enabled` OFF | Charge-time safety limit is disabled. |
| `Charge Termination` OFF | Normal end-of-charge can be disabled. |
| `Auto DPDM` / `ICO` ON | Chip may change input limit by itself. |
| `HVDCP` / `MaxCharge` / `PumpX` | Adapter negotiation can change voltage or detection. |

## What `Input Voltage Limited` Means With a Good Adapter

`Input Voltage Limited` does not automatically mean the adapter is bad. The BQ25895 is saying: "I am at my VINDPM regulation limit."

Check these values together:

| Value | Expected for normal 5-V charging |
| --- | --- |
| `Input Voltage` | clearly above `inputVoltageLimitMv`, for example 5100 mV |
| `Actual Input Voltage Limit` | for example 4400 mV |
| `Actual Absolute VINDPM` | Yes |
| `Input Voltage Limited` | No |

If `Input Voltage` is clearly above `Actual Input Voltage Limit` and `Input Voltage Limited` still remains active, that is not a normal adapter limit. It can be a short voltage dip, stale status, wrong VINDPM write order, or a software bug. In the Web UI: press `Safe Charge` and check the `Actual` values again.

## Why `precharge` Is Not a Fault

With a deeply discharged battery, the BQ25895 intentionally charges with a small current. This protects the cell. The chip changes to `fast charge` only after the battery voltage rises enough.

Typical picture:

| Display | Example |
| --- | --- |
| `Battery Voltage` | 2500 to 3000 mV |
| `Charge State` | `precharge` |
| `BQ27441 Average current` | positive, but small |

This is normal while the cell voltage slowly rises and no NTC, battery, or safety fault is active.

## English Quick Debug Order

1. Check `BQ25895 present` and `BQ27441 present`.
2. Press `Safe Charge`.
3. `Power Good` must be `Yes`.
4. `CE Pin Enabled` and `Charging Enabled` must be ON.
5. `High Impedance` and `OTG Boost Enabled` must be OFF.
6. `Charge Fault` and `NTC Fault` must be `normal`.
7. `Actual Absolute VINDPM` must be `Yes`.
8. `Actual Input Voltage Limit` should be around `4400 mV` for 5-V USB.
9. With a deeply discharged battery, `precharge` is normal.
10. Use `BQ27441 Average current` for real battery-current direction.
