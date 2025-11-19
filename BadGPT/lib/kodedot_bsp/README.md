# KodeDotBSP

Board Support Package for Kode Dot: display bring-up (Arduino_GFX), LVGL integration and capacitive touch.

## Usage

Include the public headers with the `kodedot/` prefix:

```cpp
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
```

Initialize once in `setup()` and update in `loop()`:

```cpp
DisplayManager display;

void setup() {
  Serial.begin(115200);
  if (!display.init()) {
    while (1) delay(1000);
  }
}

void loop() {
  display.update();
  delay(5);
}
```

`pin_config.h` exposes board pins and constants for the Kode Dot.

## Notes
- Prefers PSRAM for LVGL draw buffers; falls back to internal SRAM.
- Registers LVGL display and input drivers.
- Provides simple brightness and touch helpers.
