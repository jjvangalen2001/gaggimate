# UI optimization — 2026-10-08

## Scope

Only ConceptUI rendering and obsolete generated assets changed. Controller logic,
pump/heater control, communication, display driver, fonts, layouts, animation timing,
pressure sampling and the live gradient pixel algorithm remain unchanged.

## Changes

- Cache quick-settings availability and volumetric visual states: styles are updated
  only on a real transition. Weight, warnings and flush progress still update live.
- Skip repeated writes of unchanged image colors, opacity, text fonts/colors and
  graph colors. LVGL 8.4 refreshes object styles even when a setter repeats a value.
- Do not regenerate invisible gradients behind menu, standby or opaque quick settings.
  The current gradient is generated again when the underlying screen becomes visible.
- Remove eight unreferenced `concept_bg_*.c` bitmaps and their declarations. Stop
  regenerating them. The current UI generates its gradients at runtime, not from these
  images. Deleted tracked files remain recoverable through Git.

## Verification

- `display-sim` build succeeded; `git diff --check` passed.
- Menu, brew overview, steam, water, grind, preinfusion, brewing and complete screenshots
  before/after are pixel-identical. Quick settings is also pixel-identical.
- Emulator: unavailable controls do not execute actions; flush stays in quick settings;
  progress fills and completes; other actions become unavailable during flushing and
  become available again afterward.
- Native binary allocated sections (`size`: text + data + bss):
  5,654,473 -> 3,811,913 bytes, a reduction of 1,842,560 bytes. `nm` confirms the
  obsolete bitmap symbols are gone. This is the emulator result, not a measured
  ESP32 firmware-size or RAM reduction.
- Three short headless brew runs per binary did not establish a reliable CPU speedup.
  Before user CPU seconds: 0.248 / 0.230 / 0.229; after: 0.214 / 0.243 / 0.252.
  Do not infer a hardware responsiveness percentage from these noisy startup-inclusive runs.

## Not yet verified

No firmware was flashed or pushed. This container has the native build toolchain only;
an ESP32 display build and on-device latency/stability testing remain necessary before
claiming measured performance improvements on the physical machine.

## Hardware build follow-up — 2026-10-09

Both ESP32-S3 builds passed using PlatformIO espressif32 6.12.0.
Display: 93,952 bytes static RAM, 3,898,721 bytes flash allocation.
Controller: 38,300 bytes static RAM, 701,601 bytes flash allocation.
The display app was flashed over USB and its written hash verified; its web server
responded and reported healthy UI/controller tasks. No on-device latency or long-run
stability benchmark has been performed. The controller initially reported
v1.8.2-ui2 and requires the matching protocol-5 firmware before machine testing.
