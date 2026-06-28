# Contributing

Thank you for helping improve Batthub.

## Scope

This repository contains firmware, Arduino examples, documentation, and hardware handoff notes for the Batthub smart battery module.

## Local Checks

Compile the examples before opening a pull request:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3 --library . examples/batthub_quick_start
arduino-cli compile --fqbn esp32:esp32:esp32c3 --library . examples/web_debug_ui
```

For broad changes, compile every example listed in `.github/workflows/arduino-compile.yml`.

## Safety Rules

- Do not commit private WiFi credentials, API keys, customer logs, or local machine paths.
- Do not raise current or voltage defaults without documenting the hardware reason.
- For charger or gauge changes, update the related docs under `docs/`.
- Hardware claims must be backed by real PCB tests.

## Pull Requests

Include:

- what changed
- which examples were compiled
- whether hardware was tested
- any remaining safety or validation gaps
