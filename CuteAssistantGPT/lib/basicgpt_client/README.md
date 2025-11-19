# BasicGPT Client

A minimal, production-friendly client to send WAV audio files to OpenAI and receive a concise text reply. It is designed for ESP32/Arduino environments and aims to keep responsibilities clear:

- WiFi connectivity and display/UI are managed by your application.
- This library only handles HTTPS request/response and response parsing.

## Features

- Streams WAV file as Base64 without loading the entire file in memory
- Small and dependency-light (uses `WiFiClientSecure`, `SD_MMC`, `FS`)
- Resilient text extraction for current `chat.completions`-style responses
- Fully commented in English for easy maintenance

## Installation

Place this folder under `lib/basicgpt_client/` of your PlatformIO project. Ensure your environment has PSRAM enabled if you plan to use larger buffers.

## API Overview

```cpp
#include <basicgpt_client/BasicGPTClient.h>

BasicGPTClient::Config cfg;
cfg.host = "api.openai.com";
cfg.port = 443;
cfg.apiKey = "sk-..."; // Your API key
cfg.model = "gpt-4o-audio-preview"; // Or another compatible model
cfg.systemPrompt = "Answer in Spanish, very concise.";
cfg.httpTimeoutMs = 60000; // Optional

BasicGPTClient client(cfg);

String reply;
bool ok = client.askAudioFromWav("/rec.wav", reply);
if (ok) {
    // Use the assistant reply text
}
```

### Class: `BasicGPTClient`

- **Config**: runtime configuration
  - **host**: OpenAI host, default `api.openai.com`
  - **port**: TLS port, default `443`
  - **apiKey**: your API key (required)
  - **model**: model identifier, default `gpt-4o-audio-preview`
  - **systemPrompt**: optional system instruction
  - **httpTimeoutMs**: socket timeout in milliseconds (default 60000)

- **Constructor**: `BasicGPTClient(const Config& config)`
  - Stores config for later use.

- **updateConfig(const Config& config)**
  - Update client configuration at runtime.

- **askAudioFromWav(const char* wavPath, String& outText) -> bool**
  - Sends a single request with the WAV file at `wavPath` encoded as Base64.
  - Returns true on success with non-empty `outText`.

## Responsibilities and Assumptions

- The application is responsible for:
  - Ensuring WiFi is connected and stable
  - Recording and saving the WAV file to a filesystem accessible as `SD_MMC`
  - Any UI updates
- This library only performs the HTTP transaction and extracts the assistant text.

## Error Handling Tips

- If `askAudioFromWav` returns false:
  - Check SD card and path validity
  - Verify TLS connectivity and DNS
  - Confirm the API key and model are valid
  - Inspect serial logs for HTTP status lines and errors

## License

MIT
