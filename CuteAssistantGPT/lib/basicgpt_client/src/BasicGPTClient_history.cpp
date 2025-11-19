// Extension of BasicGPTClient to support conversation history
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include "basicgpt_client/BasicGPTClient.h"

// Defined in BasicGPTClient.cpp
extern const char B64T[];

bool BasicGPTClient::askAudioFromPCMWithHistory(const uint8_t* pcmData,
                                                size_t pcmBytes,
                                                uint32_t sampleRate,
                                                uint16_t bitsPerSample,
                                                uint16_t channels,
                                                const std::vector<ChatMessage>& conversationHistory,
                                                String& outText) {
    outText = "";
    if (!pcmData || pcmBytes == 0) {
        Serial.println("[BasicGPTClient] ERROR: Empty PCM buffer");
        return false;
    }
    if (!isConfigValid()) {
        Serial.println("[BasicGPTClient] ERROR: Invalid config");
        return false;
    }

    // Build WAV header
    uint8_t hdr[44];
    buildWavHeader(hdr, sampleRate, bitsPerSample, channels, (uint32_t)pcmBytes);
    const size_t wavBytes = sizeof(hdr) + pcmBytes;
    const size_t b64Bytes = 4 * ((wavBytes + 2) / 3);

    // JSON escape helper
    auto jsonEscape = [](const String& s) {
        String o; o.reserve(s.length() + 16);
        for (size_t i = 0; i < s.length(); ++i) {
            char c = s[i];
            switch (c) {
                case '\\': o += "\\\\"; break;
                case '"' : o += "\\\""; break;
                case '\n': o += "\\n"; break;
                case '\r': o += "\\r"; break;
                case '\t': o += "\\t"; break;
                default: o += c; break;
            }
        }
        return o;
    };

    // Build JSON with system prompt + conversation history + current audio
    String json = String("{\"model\":\"") + cfg_.model + "\"," +
                  "\"modalities\":[\"text\"]," +
                  "\"messages\":[";
    
    // System message
    const String sys = jsonEscape(String(cfg_.systemPrompt ? cfg_.systemPrompt : ""));
    json += "{\"role\":\"system\",\"content\":[{\"type\":\"text\",\"text\":\"" + sys + "\"}]}";
    
    // Add conversation history
    for (size_t i = 0; i < conversationHistory.size(); ++i) {
        const ChatMessage& msg = conversationHistory[i];
        String escapedContent = jsonEscape(msg.content);
        json += ",{\"role\":\"" + msg.role + "\",\"content\":[{\"type\":\"text\",\"text\":\"" + escapedContent + "\"}]}";
    }
    
    // Current user audio message
    json += ",{\"role\":\"user\",\"content\":[";
    json += "{\"type\":\"input_audio\",\"input_audio\":{\"data\":\"";
    
    String suf = String("\",\"format\":\"wav\"}}]}]}");
    
    const size_t contentLen = json.length() + b64Bytes + suf.length();

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(cfg_.httpTimeoutMs / 1000);

    if (!client.connect(cfg_.host, cfg_.port)) {
        Serial.println("[BasicGPTClient] ERROR: TLS connect failed");
        return false;
    }

    // HTTP headers
    client.printf("POST /v1/chat/completions HTTP/1.1\r\n");
    client.printf("Host: %s\r\n", cfg_.host);
    client.print ("Content-Type: application/json\r\n");
    client.print ("Accept: application/json\r\n");
    client.print ("Accept-Encoding: identity\r\n");
    client.printf("Authorization: Bearer %s\r\n", cfg_.apiKey ? cfg_.apiKey : "");
    client.printf("Content-Length: %u\r\n", (unsigned)contentLen);
    client.print ("Connection: close\r\n\r\n");

    // Body prefix
    client.print(json);

    // Base64 streaming
    const size_t IN_CHUNK = 12288;
    uint8_t* inBuf = (uint8_t*)heap_caps_malloc(IN_CHUNK + 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char*    outBuf = (char*)heap_caps_malloc(16384 + 16, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!inBuf) inBuf = (uint8_t*)malloc(IN_CHUNK + 2);
    if (!outBuf) outBuf = (char*)malloc(16384 + 16);
    if (!inBuf || !outBuf) {
        if (inBuf) free(inBuf);
        if (outBuf) free(outBuf);
        client.stop();
        Serial.println("[BasicGPTClient] ERROR: Out of memory");
        return false;
    }

    size_t carry = 0;
    // WAV header
    memcpy(inBuf + carry, hdr, sizeof(hdr));
    size_t n = carry + sizeof(hdr);
    size_t proc = (n / 3) * 3;
    size_t wrote = b64EncodeChunk(inBuf, proc, outBuf, false);
    if (wrote) client.write((const uint8_t*)outBuf, wrote);
    carry = n - proc;
    if (carry) {
        inBuf[0] = inBuf[proc];
        if (carry == 2) inBuf[1] = inBuf[proc + 1];
    }

    // PCM data
    size_t offset = 0;
    while (offset < pcmBytes) {
        size_t chunk = (pcmBytes - offset > IN_CHUNK) ? IN_CHUNK : (pcmBytes - offset);
        memcpy(inBuf + carry, pcmData + offset, chunk);
        n = carry + chunk;
        proc = (n / 3) * 3;
        wrote = b64EncodeChunk(inBuf, proc, outBuf, false);
        if (wrote) client.write((const uint8_t*)outBuf, wrote);
        carry = n - proc;
        if (carry) {
            inBuf[0] = inBuf[proc];
            if (carry == 2) inBuf[1] = inBuf[proc + 1];
        }
        offset += chunk;
        yield();
    }
    if (carry) {
        wrote = b64EncodeChunk(inBuf, carry, outBuf, true);
        if (wrote) client.write((const uint8_t*)outBuf, wrote);
    }

    client.print(suf);
    free(inBuf);
    free(outBuf);

    // Read response
    String status = client.readStringUntil('\n');
    bool httpOk = (status.indexOf(" 200 ") >= 0);
    if (!httpOk) {
        Serial.print("[BasicGPTClient] HTTP status: "); Serial.println(status);
    }
    while (client.connected()) {
        String h = client.readStringUntil('\n');
        if (h == "\r") break;
    }
    String body; body.reserve(65536);
    while (client.connected() || client.available()) {
        String chunk = client.readString();
        if (!chunk.length()) break;
        body += chunk;
        if (body.length() > 65536) break;
        yield();
    }
    client.stop();

    outText = extractAssistantText(body);
    if (outText.length() == 0) {
        Serial.println("[BasicGPTClient] ERROR: Could not parse assistant text");
    }
    return outText.length() > 0;
}
