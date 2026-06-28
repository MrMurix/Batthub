# Product Readiness

Short version: the library can be product-ready. The charger module is product-ready only after real hardware tests.

## Library Done

- `Batthub` module-level API exists for customers.
- No MCU-specific pins are built into the library.
- CE, OTG, and INT are optional host GPIOs through `Batthub::BeginOptions`.
- Low-level `bq25` and `bq27` APIs remain public through `batthub.charger` and `batthub.gauge`.
- All examples compile for ESP32-C3 as one CI target.
- Web UI saves settings and reloads them after reboot.
- Missing BQ25895/BQ27441 does not crash the UI.
- `beginReport()` shows which chip is missing.
- Saved input limit is restored after USB plug-in.
- Default for maker use: `Auto DPDM OFF`, `HVDCP ON`, `MaxCharge ON`, `ICO OFF`.

## Examples

- `batthub_quick_start`: start here for customer code.
- `batthub_all_features`: customer-facing Serial console for nearly every board feature.
- `quick_start_module`: start here.
- `web_debug_ui`: best for testing.
- `production_self_test`: use before shipping boards.
- `fuel_gauge_setup_once`: write the battery profile.
- `serial_control_console`: manual charger commands.

## Public Library API Added

- `src/Batthub.h` provides the module API buyers should use.
- `examples/batthub_quick_start` shows the minimal customer flow.
- `examples/batthub_all_features` exposes charge, OTG, adapter detection, register dump, ship mode, and gauge setup.
- `docs/BATTHUB_LIBRARY.md` documents the customer-facing library layer.
- GitHub Actions compiles all examples for ESP32-C3 on push and pull request.

## Hardware Must Test

- heat at max charge current
- inductor current and temperature
- USB input with weak and strong adapters
- QuickCharge/HVDCP behavior if 9 V or 12 V input is advertised
- cable and connector current limit
- SYS behavior with and without battery
- OTG voltage, current, and heat
- CE and OTG pin polarity
- TS/NTC cold, normal, warm, hot, and open
- empty, normal, and full battery
- reverse battery and short-circuit strategy

## Fuel Gauge Must Test

- chemistry matches your battery
- capacity and energy are set correctly
- charge/discharge cycles match real capacity
- settings survive full power loss

## Before Selling

- schematic reviewed
- layout reviewed against TI guidelines
- thermal test done
- firmware compiled and flashed
- max current and battery type documented
- CE/RoHS/WEEE checked for your sales region

## GitHub Release Gate

- `hardware/schematic.pdf` or an equivalent schematic export is uploaded.
- `hardware/bom.csv` or a BOM export is uploaded.
- `hardware/pinout.md` lists SDA, SCL, CE, OTG, INT, VBUS, SYS, BAT, GND, TS/NTC, and any not-connected pins.
- `docs/HARDWARE_HANDOFF.md` is checked against the real schematic.
- GitHub Actions is green for all examples.
- No private WiFi credentials, local paths, or test-only secrets are present in examples or docs.
