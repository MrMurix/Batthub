# Product Readiness / Produkt-Check

## Deutsch

Die Library kann die Hardware bedienen. Das bedeutet nicht automatisch, dass ein fertiges Produkt sicher oder zertifiziert ist.

Vor dem Verkauf testen:

- maximaler Ladestrom auf echter Platine
- Temperatur von IC, Induktor, Stecker und Leiterbahnen
- USB-Eingang mit schwachen und starken Netzteilen
- Kabel- und Stecker-Stromgrenze
- Verhalten mit leerem, normalem und vollem Akku
- Verhalten ohne Akku
- OTG-Boost-Spannung, Strom und Temperatur
- TS/NTC bei kalt, normal, warm, heiss und offen
- CE- und OTG-Pin-Polaritaet
- Akku-Profil im BQ27441
- ob der Akku eine eigene Schutzschaltung braucht
- gesetzliche Pflichten wie CE, RoHS, WEEE und Transportregeln

Wichtig:

- `inputCurrentLimitMa` darf nie hoeher sein als Adapter, Kabel, Stecker und PCB wirklich koennen.
- `chargeCurrentMa` muss zum Akku und zur Temperatur passen.
- `chargeVoltageMv` muss zum Akkutyp passen.
- Die Prozentanzeige vom BQ27441 ist nur gut, wenn das Akku-Profil passt.

## English

The library can control the hardware. That does not automatically make a finished product safe or certified.

Test before selling:

- maximum charge current on the real PCB
- temperature of IC, inductor, connector, and traces
- USB input with weak and strong adapters
- cable and connector current limits
- behavior with empty, normal, and full battery
- behavior without battery
- OTG boost voltage, current, and temperature
- TS/NTC at cold, normal, warm, hot, and open states
- CE and OTG pin polarity
- BQ27441 battery profile
- whether the battery needs its own protection circuit
- legal requirements such as CE, RoHS, WEEE, and transport rules

Important:

- `inputCurrentLimitMa` must never be higher than adapter, cable, connector, and PCB can really handle.
- `chargeCurrentMa` must match the battery and thermal design.
- `chargeVoltageMv` must match the battery type.
- BQ27441 percentage is only useful when the battery profile is correct.
