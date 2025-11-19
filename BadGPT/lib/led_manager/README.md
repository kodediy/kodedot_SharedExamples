# LED Manager Library

Library for managing NeoPixel LED states and visual feedback.

## Features

- NeoPixel control
- State-based LED colors
- Brightness management
- Custom color support

## Usage

```cpp
#include <led_manager/LEDManager.h>

LEDManager ledManager;
LEDConfig config;
config.pin = 8;
config.count = 1;

ledManager.init(config);
ledManager.setState(LEDState::Recording);
```
