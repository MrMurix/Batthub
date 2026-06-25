# Product Readiness

Short version: the library can be product-ready. The charger module is product-ready only after real hardware tests.

## Library Done

- All examples compile for the target ESP32 board.
- Web UI saves settings and reloads them after reboot.
- Missing BQ25895/BQ27441 does not crash the UI.
- `beginReport()` shows which chip is missing.
- Saved input limit is restored after USB plug-in.
- Default for maker use: `Auto DPDM OFF`, `HVDCP ON`, `MaxCharge ON`, `ICO OFF`.

## Examples

- `quick_start_module`: start here.
- `web_debug_ui`: best for testing.
- `production_self_test`: use before shipping boards.
- `fuel_gauge_setup_once`: write the battery profile.
- `serial_control_console`: manual charger commands.

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
