# Batthub Dokumentation / Batthub Documentation

Batthub ist ein Akku-Modul mit zwei TI-Chips:

- `BQ25895`: Charger, Power-Path und OTG-Boost.
- `BQ27441-G1`: Fuel Gauge fuer Akku-Prozent, Kapazitaet und Akku-Status.

Das Batthub-Modul hat keinen eigenen Mikrocontroller. Ein Nutzer verbindet seinen eigenen Host per I2C.

## Deutsch

| Seite | Inhalt |
| --- | --- |
| [BQ25/BQ27 fuer Beginner](BQ25_BQ27_BEGINNER.md#deutsch) | Kurze, einfache Erklaerung fuer Nutzer ohne Vorwissen. |
| [Web UI Doku](BATTHUB_WEB_UI.md#deutsch) | Alle sichtbaren Web-UI-Begriffe, Buttons, Werte und Risiken. |
| [Library Doku](BATTHUB_LIBRARY.md#deutsch---kurzfassung) | Batthub Library, Start, Pins, API und Begriffe. |
| [Produkt-Check](PRODUCT_READINESS.md#deutsch) | Kurze Sicherheits- und Verkaufsvorbereitung. |

## English

| Page | Content |
| --- | --- |
| [BQ25/BQ27 for Beginners](BQ25_BQ27_BEGINNER.md#english) | Short beginner explanation for users with no battery-chip background. |
| [Web UI Documentation](BATTHUB_WEB_UI.md#english) | Every visible Web UI term, button, value, and risk. |
| [Library Documentation](BATTHUB_LIBRARY.md#batthub-library) | Batthub library, startup, pins, API, and terms. |
| [Product Readiness](PRODUCT_READINESS.md#english) | Short safety and selling-preparation checklist. |

## Kurzer Start / Quick Start

Deutsch:

1. Batthub an SDA/SCL vom Host anschliessen.
2. Library installieren.
3. Beispiel `batthub_quick_start` oeffnen.
4. Board auswaehlen und hochladen.
5. Serial Monitor mit `115200` oeffnen.

English:

1. Connect Batthub to the host SDA/SCL pins.
2. Install the library.
3. Open the `batthub_quick_start` example.
4. Select the controller board and upload.
5. Open Serial Monitor at `115200`.
