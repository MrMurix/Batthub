# Hardware Handoff

Use this checklist when uploading the schematic and final hardware files. The firmware side is prepared; this file defines what must be true for the hardware side.

## Upload Targets

Put hardware release files in `hardware/`:

- `hardware/schematic.pdf`: readable exported schematic.
- `hardware/bom.csv`: BOM export with manufacturer part numbers when possible.
- `hardware/pinout.md`: user-facing pinout and optional host pins.
- `hardware/limits.md`: tested current, voltage, temperature, and battery limits.
- source CAD files if you want the design to be open hardware.

## Schematic Review

Check these before publishing:

- BQ25895 and BQ27441 use the expected I2C addresses: `0x6A` and `0x55`.
- SDA and SCL have pullups suitable for the selected logic rail.
- CE, OTG, and INT polarity matches `Batthub::Pins` defaults or is documented.
- VBUS connector current rating matches `inputCurrentLimitMa`.
- SYS output current limit and use case are documented.
- BAT path, sense path, and protection strategy are documented.
- TS/NTC wiring matches the battery and charger settings.
- ILIM resistor and `ilimPinEnabled` behavior are documented.
- Inductor, input capacitor, output capacitor, and layout follow TI guidance.
- Grounding, thermal vias, and high-current traces are reviewed.

## Firmware Values To Publish

Document the recommended default values for the released board:

- battery type and cell count
- `inputCurrentLimitMa`
- `chargeCurrentMa`
- `chargeVoltageMv`
- `terminationCurrentMa`
- `prechargeCurrentMa`
- `terminateVoltageMv`
- BQ27441 `designCapacityMah`
- BQ27441 `designEnergyMwh`
- whether CE, OTG, INT, TS/NTC, and ILIM are populated

## Validation Evidence

Before tagging a public hardware release, keep or upload evidence for:

- max-charge thermal test
- weak and strong adapter test
- USB cable voltage-drop test
- full battery charge termination
- empty battery precharge
- no-battery behavior
- OTG boost voltage, current, and temperature
- TS/NTC cold, room, hot, and open states
- ship mode and wake behavior
- BQ27441 battery profile write/read test

## Release Rule

Do not mark the hardware as ready until the schematic, BOM, pinout, tested limits, and product-readiness checklist are all updated together.
