# BQ25895 und BQ27441 fuer Beginner / BQ25895 and BQ27441 for Beginners

## Deutsch

Diese Seite ist fuer Nutzer, die noch keine Erfahrung mit Akku-Chips haben.

### Was ist Batthub?

Batthub ist ein kleines Akku-Modul. Es nimmt einen 1S Li-Ion/LiPo Akku, einen USB/VBUS-Eingang und gibt deinem Projekt eine versorgte Systemspannung. Das Modul hat zwei wichtige Chips:

- `BQ25895`: kuemmert sich um Laden, USB-Eingang, Power-Path und OTG-Boost.
- `BQ27441-G1`: misst den Akku und schaetzt Prozent, Kapazitaet und Zustand.

Das Modul hat keinen eigenen ESP32 und keinen eigenen Arduino. Dein Projekt braucht einen Host-Controller, der per I2C mit den Chips spricht.

### Was macht der BQ25895?

Der `BQ25895` ist der Charger und Power-Manager.

| Aufgabe | Einfach erklaert |
| --- | --- |
| Akku laden | Er laedt den Akku mit einem eingestellten Strom und einer eingestellten Zielspannung. |
| USB-Eingang begrenzen | Er verhindert, dass dein Modul zu viel Strom aus USB/VBUS zieht. |
| Power-Path | Er versorgt dein System aus USB, Akku oder beidem, je nach Zustand. |
| OTG-Boost | Er kann aus dem Akku wieder eine VBUS/USB-Ausgangsspannung machen. |
| Temperatur beachten | Er nutzt TS/NTC und eigene Temperaturgrenzen, um Laden sicherer zu machen. |
| Fehler melden | Er meldet zum Beispiel Input-Fehler, Temperaturfehler, Watchdog und Battery-Fault. |

### Wichtige BQ25895 Begriffe

| Begriff | Bedeutung |
| --- | --- |
| `inputCurrentLimitMa` | Maximaler Strom vom USB/VBUS-Eingang. |
| `chargeCurrentMa` | Maximaler Strom in den Akku. |
| `chargeVoltageMv` | Spannung, bei der der Akku voll sein soll. |
| `terminationCurrentMa` | Wenn der Ladestrom darunter faellt, gilt der Akku als voll. |
| `Power Good` | Eingang ist gut genug, um das System zu versorgen. |
| `Input Current Limited` | Der Eingangsstrom ist am Limit; realer Ladestrom kann kleiner sein. |
| `Input Voltage Limited` | Die Eingangsspannung bricht ein; der Chip reduziert den Strom. |
| `Thermal Regulation` | Der Chip ist warm und reduziert den Ladestrom. |
| `High Impedance` | Eingangspfad wird stark reduziert; Laden kann stoppen. |
| `OTG Boost` | Akku versorgt den VBUS-Ausgang; normales Laden ist dann nicht der Fokus. |

### Was macht der BQ27441?

Der `BQ27441-G1` ist der Fuel Gauge. Er ist kein Charger. Er laedt den Akku nicht. Er misst und berechnet Akku-Daten.

| Aufgabe | Einfach erklaert |
| --- | --- |
| Akku-Prozent | Schaetzt den Ladezustand in Prozent. |
| Akku-Spannung | Misst die Akku-Spannung. |
| Akku-Strom | Misst Lade- und Entladestrom. |
| Kapazitaet | Schaetzt verbleibende und volle Kapazitaet. |
| Akku-Gesundheit | Schaetzt State of Health. |
| Akku-Profil | Braucht passende Akku-Daten, damit die Prozentanzeige sinnvoll ist. |

### Wichtige BQ27441 Begriffe

| Begriff | Bedeutung |
| --- | --- |
| `State Of Charge` | Akku-Prozent. |
| `Remaining Capacity` | Noch verfuegbare Kapazitaet in mAh. |
| `Full Capacity` | Gelernte volle Kapazitaet in mAh. |
| `Average Current` | Durchschnittlicher Akku-Strom. |
| `Average Power` | Durchschnittliche Akku-Leistung. |
| `State Of Health` | Akku-Gesundheit in Prozent. |
| `designCapacityMah` | Nennkapazitaet deines Akkus. |
| `designEnergyMwh` | Nennenergie deines Akkus. |
| `terminateVoltageMv` | Entladeschluss-Spannung fuer die Berechnung. |
| `taperRate` | Wert fuer Ladeende-Erkennung im Gauge. |

### Warum zeigt der Ladestrom manchmal weniger?

Der eingestellte Strom ist nur ein Maximum. Der echte Strom kann kleiner sein, wenn:

- USB-Netzteil oder Kabel schwach sind.
- `inputCurrentLimitMa` klein ist.
- Akku fast voll ist.
- Chip oder Akku warm sind.
- `Input Current Limited`, `Input Voltage Limited` oder `Thermal Regulation` aktiv ist.
- `Auto DPDM` oder `ICO` den Eingang automatisch begrenzt.

### Welche Werte muss ein Nutzer kennen?

Minimal:

- Akkutyp: 1S Li-Ion/LiPo.
- Akku-Kapazitaet in mAh.
- erlaubter Ladestrom vom Akku.
- erlaubte Ladeschlussspannung, meistens 4200 mV bei normaler Li-Ion/LiPo-Zelle.
- ob ein NTC/Temperatursensor angeschlossen ist.
- wie viel Strom USB, Kabel, Stecker und PCB wirklich aushalten.

### Wichtig fuer die Web UI

Die Web UI ist ein Debug-Werkzeug. Sie zeigt sehr viele BQ25895- und BQ27441-Funktionen, auch solche, die Laden absichtlich stoppen koennen.

| Web-UI-Hilfe | Bedeutung |
| --- | --- |
| `Safe Charge` | Stabiler Startpunkt zum Laden und Testen. Nutzt konservative Werte und schaltet riskante Lade-Blocker aus. |
| `Expert Locked` | Normale Schutzstufe. Riskante Funktionen fragen nach. |
| `Charge Diagnosis` | Kurzer Text, warum gerade geladen, langsam geladen oder nicht geladen wird. |
| `Actual BQ25895 Settings` | Echte Werte im Chip. Diese sind wichtiger als Wunschwerte in Eingabefeldern. |

Als Beginner: erst `Safe Charge`, dann `Charge Diagnosis` lesen, dann `BQ27441 Average current` ansehen.

Mehr Details: [Charging Troubleshooting](CHARGING_TROUBLESHOOTING.md#deutsch).

### Sicherheit

Diese Library macht die Chips bedienbar. Sie macht den Akku nicht automatisch sicher. Hardware, Akku, Stecker, Leiterbahnen, Temperatur und Schutzschaltung muessen real getestet werden.

## English

This page is for users who have no background with battery-management chips.

### What is Batthub?

Batthub is a small battery module. It takes a 1S Li-Ion/LiPo cell, a USB/VBUS input, and provides power for a project. The module has two important chips:

- `BQ25895`: handles charging, USB input, power-path, and OTG boost.
- `BQ27441-G1`: measures the battery and estimates percentage, capacity, and state.

The module has no built-in ESP32 and no built-in Arduino. The user's project needs a host controller that talks to both chips over I2C.

### What does the BQ25895 do?

The `BQ25895` is the charger and power manager.

| Job | Simple explanation |
| --- | --- |
| Battery charging | Charges the cell with a configured current and target voltage. |
| USB input limiting | Prevents the module from drawing too much current from USB/VBUS. |
| Power-path | Powers the system from USB, battery, or both, depending on state. |
| OTG boost | Can turn battery voltage back into a VBUS/USB output voltage. |
| Temperature handling | Uses TS/NTC and internal temperature limits to reduce unsafe charging. |
| Fault reporting | Reports input faults, temperature faults, watchdog, boost, and battery faults. |

### Important BQ25895 terms

| Term | Meaning |
| --- | --- |
| `inputCurrentLimitMa` | Maximum current from the USB/VBUS input. |
| `chargeCurrentMa` | Maximum current into the battery. |
| `chargeVoltageMv` | Voltage where the battery is considered full. |
| `terminationCurrentMa` | If charge current falls below this, the battery can be considered full. |
| `Power Good` | Input is good enough to power the system. |
| `Input Current Limited` | Input current limit is active; real charge current can be lower. |
| `Input Voltage Limited` | Input voltage is dropping; the chip reduces current. |
| `Thermal Regulation` | The chip is warm and reduces charge current. |
| `High Impedance` | Input path is reduced; charging can stop. |
| `OTG Boost` | Battery powers the VBUS output; normal charging is not the main mode then. |

### What does the BQ27441 do?

The `BQ27441-G1` is the fuel gauge. It is not a charger. It does not charge the battery. It measures and estimates battery data.

| Job | Simple explanation |
| --- | --- |
| Battery percentage | Estimates state of charge in percent. |
| Battery voltage | Measures battery voltage. |
| Battery current | Measures charge and discharge current. |
| Capacity | Estimates remaining and full capacity. |
| Battery health | Estimates state of health. |
| Battery profile | Needs matching battery data so the percentage is useful. |

### Important BQ27441 terms

| Term | Meaning |
| --- | --- |
| `State Of Charge` | Battery percentage. |
| `Remaining Capacity` | Remaining capacity in mAh. |
| `Full Capacity` | Learned full capacity in mAh. |
| `Average Current` | Average battery current. |
| `Average Power` | Average battery power. |
| `State Of Health` | Battery health in percent. |
| `designCapacityMah` | Rated capacity of the battery. |
| `designEnergyMwh` | Rated energy of the battery. |
| `terminateVoltageMv` | End-of-discharge voltage used for calculations. |
| `taperRate` | Gauge value used for charge-termination detection. |

### Why is real charge current sometimes lower?

The configured current is only a maximum. The real current can be lower when:

- USB adapter or cable is weak.
- `inputCurrentLimitMa` is low.
- Battery is almost full.
- Chip or battery is warm.
- `Input Current Limited`, `Input Voltage Limited`, or `Thermal Regulation` is active.
- `Auto DPDM` or `ICO` automatically changed the input limit.

### What values does a user need?

Minimum:

- Battery type: 1S Li-Ion/LiPo.
- Battery capacity in mAh.
- Allowed charge current for the battery.
- Allowed charge voltage, often 4200 mV for a normal Li-Ion/LiPo cell.
- Whether an NTC/temperature sensor is connected.
- How much current the USB adapter, cable, connector, and PCB can really handle.

### Important for the Web UI

The Web UI is a debug tool. It exposes many BQ25895 and BQ27441 functions, including functions that can intentionally stop charging.

| Web UI help | Meaning |
| --- | --- |
| `Safe Charge` | Stable starting point for charging and testing. Uses conservative values and disables risky charge blockers. |
| `Expert Locked` | Normal guarded mode. Risky functions ask for confirmation. |
| `Charge Diagnosis` | Short text explaining why charging is active, slow, or blocked. |
| `Actual BQ25895 Settings` | Real values inside the chip. These matter more than desired values in input fields. |

As a beginner: press `Safe Charge`, read `Charge Diagnosis`, then check `BQ27441 Average current`.

More detail: [Charging Troubleshooting](CHARGING_TROUBLESHOOTING.md#english).

### Safety

This library makes the chips usable. It does not make the battery automatically safe. Hardware, battery, connector, PCB traces, temperature, and protection circuits must be tested on real hardware.
