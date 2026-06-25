# bq25-bq27

Arduino and PlatformIO library for a 1S smart charger module built around:

- `bq25`: TI BQ25895 I2C charger / power-path / OTG controller
- `bq27`: TI BQ27441-G1 fuel gauge
- `bq25bq27`: convenience wrapper for boards using both chips

The library is intentionally dependency-free except for Arduino `Wire`.

## Install

Remove any older `bq25-bq27` copy from your Arduino libraries folder first. Otherwise Arduino may compile the old library by accident.

Arduino CLI from this folder:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 --library "C:\Users\sma80\Documents\27 Modul" "C:\Users\sma80\Documents\27 Modul\examples\web_debug_ui"
```

## Quick Start

```cpp
#include <Wire.h>
#include <bq25_bq27.h>

bq25bq27 power;

void setup() {
  Serial.begin(115200);

  bq25bq27::BeginReport report = power.beginReport(Wire);
  if (!report.ok()) {
    Serial.println("BQ25895 or BQ27441 was not found.");
    while (true) {
      delay(1000);
    }
  }

  // capacity mAh, charge current mA, input limit mA, charge voltage mV, terminate voltage mV
  power.configure1s(2500, 1000, 500, 4208, 3200);
  power.charger.startAdc(true);
}

void loop() {
  bq25bq27::Snapshot s = power.snapshot();
  Serial.println(s.gauge.stateOfChargePercent);
  delay(1000);
}
```

## Examples

- `bq25_basic`: charger only.
- `bq27_basic`: fuel gauge only.
- `bq25_bq27_module`: both chips, small example.
- `quick_start_module`: best starter sketch.
- `serial_control_console`: test commands in Serial Monitor.
- `production_self_test`: board check before shipping.
- `fuel_gauge_setup_once`: write battery data to the BQ27441.
- `otg_and_ship_mode`: OTG boost and ship mode.
- `web_debug_ui`: ESP32 web dashboard with saved settings.

Useful Serial commands:

- `s`: status
- `c` / `C`: charger on / off
- `o` / `O`: OTG on / off
- `5` / `9`: input limit 500 mA / 900 mA
- `1` / `2`: charge current 1000 mA / 2000 mA

The web UI saves CE, charge enable, OTG, ADC, input limit, charge current, and charge voltage. After reboot, the ESP32 loads them again.

## Charging Current

The value you set is the maximum. The real current can be lower.

Common reasons:

- weak USB adapter
- thin or bad cable
- battery almost full
- chip is warm
- BQ25895 lowered the input limit

Web UI words:

- `Input Limit`: how much current the board may pull from USB.
- `Charge Current Set`: max current into the battery.
- `Charge Current`: real current now.
- `IINDPM`: USB current limit is being hit.
- `VINDPM`: USB voltage is dropping.
- `Auto DPDM`: chip checks D+/D- and may set 500 mA by itself.
- `HVDCP`: QuickCharge-style high-voltage detection, for example 9 V or 12 V adapters.
- `MaxCharge`: another D+/D- adapter detection mode.
- `ICO`: chip searches for a safe adapter current by itself.

Recommended default for this maker module:

- `Auto DPDM`: OFF
- `HVDCP`: ON
- `MaxCharge`: ON
- `ICO`: OFF
- ESP32 restores the saved input limit after USB plug-in

Why: otherwise the BQ25895 may detect an unknown USB source and silently fall back to 500 mA.

For orange QuickCharge-style USB ports: enable `Auto DPDM`, `HVDCP`, and `MaxCharge`, then press `QC/HVDCP Detect` or replug USB. If the adapter supports it, VBUS can rise to 9 V or 12 V.

Only allow high input current when your connector, cable, adapter, PCB, and thermal design are made for it.

## bq25 API

`bq25` covers the BQ25895 register map from `0x00` through `0x14`.

Common controls:

- `setInputCurrentLimit(mA)`
- `setInputVoltageLimit(mV)`
- `setChargeCurrent(mA)`
- `setPrechargeCurrent(mA)`
- `setTerminationCurrent(mA)`
- `setChargeVoltage(mV)`
- `setChargingEnabled(bool)`
- `setOtgEnabled(bool)`
- `setBoostVoltage(mV)`
- `setWatchdogTimer(...)`
- `setSafetyTimer(...)`
- `setIcoEnabled(bool)`
- `forceDpdmDetection()`
- `forceIco()`
- `enterShipMode()`

Telemetry:

- `status()`
- `faults()`
- `measurements()`
- `deviceInfo()`
- `readRegister()` / `writeRegister()` / `updateRegister()`

Note: BQ25895 `REG0A[3:0]` is reserved, so this library does not expose a fake boost-current-limit setter. Some older BQ2589x snippets online do, but that is not correct for BQ25895.

## bq27 API

`bq27` covers the BQ27441-G1 standard commands, control subcommands, and BlockData data-memory access.

Common readings:

- `voltage()`
- `averageCurrent()`
- `averagePower()`
- `stateOfCharge()`
- `stateOfHealthPercent()`
- `capacity(...)`
- `temperatureCelsiusX10()`
- `flags()`
- `snapshot()`

Configuration:

- `configureBattery(BatteryConfig)`
- `setDesignCapacity(mAh)`
- `setDesignEnergy(mWh)`
- `setTerminateVoltage(mV)`
- `setTaperRate(rate)`
- `setSoc1Thresholds(set, clear)`
- `setSocfThresholds(set, clear)`
- `setSocIntDelta(percent)`
- `setGpoutMode(...)`
- `readDataMemory(...)`
- `writeDataMemory(...)`

The raw data-memory functions make advanced BQ27441 parameters reachable without waiting for a wrapper method.

## Hardware Defaults

- BQ25895 I2C address: `0x6A`
- BQ27441-G1 I2C address: `0x55`
- Logic rail assumed by the examples: `3.3 V`
- Default BQ27441 chemistry: BQ27441-G1A style 4.2 V Li-ion/LiPo profile

For a sellable module, document battery type, max charge current, temperature limits, and whether the cell needs its own protection PCB.

## Library Release Status

Software side: ready for product integration.

Still your job before selling hardware: thermal test, battery safety limits, USB/current limits, regulatory checks, and user docs.

## Safety Notes

This library is firmware support. It is not a safety certificate.

Test the real PCB before selling it.

## References

- TI BQ25895 datasheet: https://www.ti.com/lit/ds/symlink/bq25895.pdf
- TI BQ27441-G1 datasheet: https://www.ti.com/lit/ds/symlink/bq27441-g1.pdf
- TI BQ27441-G1 technical reference manual: https://www.ti.com/lit/ug/sluuac9/sluuac9.pdf

## Files

- `src/bq25.h` / `src/bq25.cpp`: BQ25895 driver
- `src/bq27.h` / `src/bq27.cpp`: BQ27441-G1 driver
- `src/bq25_bq27.h` / `src/bq25_bq27.cpp`: combined module helper
- `examples/bq25_basic`
- `examples/bq27_basic`
- `examples/bq25_bq27_module`
- `examples/quick_start_module`
- `examples/serial_control_console`
- `examples/production_self_test`
- `examples/fuel_gauge_setup_once`
- `examples/otg_and_ship_mode`
- `examples/web_debug_ui`
