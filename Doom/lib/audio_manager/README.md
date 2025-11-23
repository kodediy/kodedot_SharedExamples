# Audio Manager Library

Library for managing I2S audio recording and PCM processing on ESP32-S3.

## Features

- I2S audio recording
- PCM normalization and limiting
- Thread-safe audio capture
- Configurable audio parameters

## Usage

```cpp
#include <audio_manager/AudioManager.h>

AudioManager audioManager;
AudioConfig config;
config.sampleRate = 32000;
config.bitsPerSample = 16;
config.numChannels = 1;

audioManager.init(config);
audioManager.startRecording();
```
