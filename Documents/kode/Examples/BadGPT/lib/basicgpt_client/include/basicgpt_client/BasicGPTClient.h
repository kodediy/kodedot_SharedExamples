#pragma once

#include <Arduino.h>
#include <vector>

// Simple message structure for conversation history
struct ChatMessage {
    String role;     // "user" or "assistant"
    String content;  // text content
};

// BasicGPTClient: minimal, dependency-light client for sending a WAV file
// to OpenAI's Chat Completions endpoint (with input_audio) and extracting
// the assistant text reply. All code is written to be portable across
// Arduino/ESP32 environments.
//
// Notes:
// - Networking (WiFi connect/retry) is intentionally left to the application.
// - This client only performs the HTTPS POST and response parsing.
// - Provide a valid API key, host and model in the Config struct.

class BasicGPTClient {
public:
    struct Config {
        const char* host = "api.openai.com";   // e.g. "api.openai.com"
        int         port = 443;                 // 443 for HTTPS
        const char* apiKey = nullptr;           // required
        const char* model = "gpt-4o-audio-preview"; // default model
        const char* systemPrompt = "";         // optional system prompt
        uint32_t    httpTimeoutMs = 60000;      // default 60s
    };

    explicit BasicGPTClient(const Config& config);

    void updateConfig(const Config& config);

    // Sends a WAV file located at wavPath and fills outText with assistant reply.
    // Returns true on success with non-empty text.
    bool askAudioFromWav(const char* wavPath, String& outText);

    // Sends raw PCM16 audio (mono/stereo) as a WAV envelope built in-memory
    // and fills outText with assistant reply. No SD card required.
    // - pcmData: pointer to PCM bytes (e.g., 16-bit signed little-endian)
    // - pcmBytes: size in bytes of the PCM buffer
    // - sampleRate: e.g., 32000
    // - bitsPerSample: e.g., 16
    // - channels: 1 (mono) or 2 (stereo)
    bool askAudioFromPCM(const uint8_t* pcmData,
                         size_t pcmBytes,
                         uint32_t sampleRate,
                         uint16_t bitsPerSample,
                         uint16_t channels,
                         String& outText);
    
    // Same as askAudioFromPCM but with conversation history for context
    // - conversationHistory: vector of previous messages (user + assistant pairs)
    bool askAudioFromPCMWithHistory(const uint8_t* pcmData,
                                    size_t pcmBytes,
                                    uint32_t sampleRate,
                                    uint16_t bitsPerSample,
                                    uint16_t channels,
                                    const std::vector<ChatMessage>& conversationHistory,
                                    String& outText);

private:
    Config cfg_;

    static size_t b64EncodeChunk(const uint8_t* in, size_t n, char* out, bool final = false);
    static String extractAssistantText(const String& body);
    static void buildWavHeader(uint8_t* hdr44,
                               uint32_t sampleRate,
                               uint16_t bitsPerSample,
                               uint16_t channels,
                               uint32_t dataBytes);

    // Internal validation helpers
    inline bool isConfigValid() const {
        return (cfg_.host && cfg_.host[0] != '\0') &&
               (cfg_.apiKey && cfg_.apiKey[0] != '\0') &&
               (cfg_.model && cfg_.model[0] != '\0') &&
               (cfg_.httpTimeoutMs >= 5000U);
    }
};


