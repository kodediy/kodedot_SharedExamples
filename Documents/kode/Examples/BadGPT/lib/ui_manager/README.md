# UI Manager Library

Library for managing LVGL user interface with thread-safe messaging.

## Features

- LVGL UI management
- Thread-safe message posting
- State-based visual feedback
- Custom brand styling

## Usage

```cpp
#include <ui_manager/UIManager.h>

UIManager uiManager;
uiManager.init();
uiManager.postStatus("Hello World");
uiManager.postStateChange(UIState::Ready);
```
