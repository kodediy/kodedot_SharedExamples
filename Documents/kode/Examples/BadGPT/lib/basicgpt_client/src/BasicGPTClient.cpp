#include <Arduino.h>
#include <FS.h>
#include <WiFiClientSecure.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>
#include "basicgpt_client/BasicGPTClient.h"

// Internal static base64 alphabet
static const char B64T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

BasicGPTClient::BasicGPTClient(const Config& config) : cfg_(config) {}

void BasicGPTClient::updateConfig(const Config& config) {
    cfg_ = config;
}

size_t BasicGPTClient::b64EncodeChunk(const uint8_t* in, size_t n, char* out, bool final) {
    size_t i = 0, j = 0;
    while (i + 3 <= n) {
        uint32_t v = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8) | in[i + 2];
        out[j++] = B64T[(v >> 18) & 0x3F];
        out[j++] = B64T[(v >> 12) & 0x3F];
        out[j++] = B64T[(v >> 6) & 0x3F];
        out[j++] = B64T[v & 0x3F];
        i += 3;
    }
    size_t rem = n - i;
    if (final && rem) {
        uint8_t a = in[i];
        uint8_t b = (rem == 2) ? in[i + 1] : 0;
        uint32_t v = ((uint32_t)a << 16) | ((uint32_t)b << 8);
        out[j++] = B64T[(v >> 18) & 0x3F];
        out[j++] = B64T[(v >> 12) & 0x3F];
        out[j++] = (rem == 2) ? B64T[(v >> 6) & 0x3F] : '=';
        out[j++] = '=';
    }
    return j;
}

String BasicGPTClient::extractAssistantText(const String& body) {
    // Minimal resilient parser for {"output_text": {"text": "..."}} or legacy "content"
    int pos = body.indexOf("\"output_text\"");
    if (pos >= 0) {
        int t = body.indexOf("\"text\"", pos);
        if (t >= 0) {
            int q1 = body.indexOf('"', t + 6);
            int q2 = -1;
            if (q1 >= 0) {
                String out; out.reserve(512);
                bool esc = false;
                for (int k = q1 + 1; k < (int)body.length(); ++k) {
                    char c = body[k];
                    if (esc) {
                        if (c == 'n') out += '\n';
                        else if (c == 't') out += '\t';
                        else if (c == 'r') out += '\r';
                        else if (c == '"') out += '"';
                        else if (c == '\\') out += '\\';
                        else out += c;
                        esc = false;
                    } else if (c == '\\') {
                        esc = true;
                    } else if (c == '"') {
                        q2 = k; break;
                    } else out += c;
                }
                if (q2 > q1) return out;
            }
        }
    }
    int cpos = body.indexOf("\"content\"");
    if (cpos >= 0) {
        int q1 = body.indexOf('"', cpos + 9);
        int q2 = -1;
        if (q1 >= 0) {
            String out; out.reserve(512);
            bool esc = false;
            for (int k = q1 + 1; k < (int)body.length(); ++k) {
                char c = body[k];
                if (esc) {
                    if (c == 'n') out += '\n';
                    else if (c == 't') out += '\t';
                    else if (c == 'r') out += '\r';
                    else if (c == '"') out += '"';
                    else if (c == '\\') out += '\\';
                    else out += c;
                    esc = false;
                } else if (c == '\\') { esc = true; }
                else if (c == '"') { q2 = k; break; }
                else out += c;
            }
            if (q2 > q1) return out;
        }
    }
    return "";
}

void BasicGPTClient::buildWavHeader(uint8_t* hdr44,
                                    uint32_t sampleRate,
                                    uint16_t bitsPerSample,
                                    uint16_t channels,
                                    uint32_t dataBytes) {
    memcpy(hdr44 + 0, "RIFF", 4);
    uint32_t riffSize = 36 + dataBytes;
    hdr44[4]  = (uint8_t)(riffSize & 0xFF);
    hdr44[5]  = (uint8_t)((riffSize >> 8) & 0xFF);
    hdr44[6]  = (uint8_t)((riffSize >> 16) & 0xFF);
    hdr44[7]  = (uint8_t)((riffSize >> 24) & 0xFF);
    memcpy(hdr44 + 8, "WAVEfmt ", 8);
    hdr44[16] = 16; hdr44[17] = 0; hdr44[18] = 0; hdr44[19] = 0;
    hdr44[20] = 1;  hdr44[21] = 0;
    hdr44[22] = (uint8_t)(channels & 0xFF);
    hdr44[23] = (uint8_t)((channels >> 8) & 0xFF);
    hdr44[24] = (uint8_t)(sampleRate & 0xFF);
    hdr44[25] = (uint8_t)((sampleRate >> 8) & 0xFF);
    hdr44[26] = (uint8_t)((sampleRate >> 16) & 0xFF);
    hdr44[27] = (uint8_t)((sampleRate >> 24) & 0xFF);
    uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    hdr44[28] = (uint8_t)(byteRate & 0xFF);
    hdr44[29] = (uint8_t)((byteRate >> 8) & 0xFF);
    hdr44[30] = (uint8_t)((byteRate >> 16) & 0xFF);
    hdr44[31] = (uint8_t)((byteRate >> 24) & 0xFF);
    uint16_t blockAlign = channels * (bitsPerSample / 8);
    hdr44[32] = (uint8_t)(blockAlign & 0xFF);
    hdr44[33] = (uint8_t)((blockAlign >> 8) & 0xFF);
    hdr44[34] = (uint8_t)(bitsPerSample & 0xFF);
    hdr44[35] = (uint8_t)((bitsPerSample >> 8) & 0xFF);
    memcpy(hdr44 + 36, "data", 4);
    hdr44[40] = (uint8_t)(dataBytes & 0xFF);
    hdr44[41] = (uint8_t)((dataBytes >> 8) & 0xFF);
    hdr44[42] = (uint8_t)((dataBytes >> 16) & 0xFF);
    hdr44[43] = (uint8_t)((dataBytes >> 24) & 0xFF);
}

bool BasicGPTClient::askAudioFromPCM(const uint8_t* pcmData,
                                     size_t pcmBytes,
                                     uint32_t sampleRate,
                                     uint16_t bitsPerSample,
                                     uint16_t channels,
                                     String& outText) {
    outText = "";
    if (!pcmData || pcmBytes == 0) {
        Serial.println("[BasicGPTClient] ERROR: Empty PCM buffer");
        return false;
    }
    if (!isConfigValid()) {
        Serial.println("[BasicGPTClient] ERROR: Invalid config (host/model/apikey/timeout)");
        return false;
    }
    if (!(bitsPerSample == 16 || bitsPerSample == 24 || bitsPerSample == 32)) {
        Serial.println("[BasicGPTClient] ERROR: Unsupported bitsPerSample (expected 16/24/32)");
        return false;
    }
    if (!(channels == 1 || channels == 2)) {
        Serial.println("[BasicGPTClient] ERROR: Unsupported channels (expected 1 or 2)");
        return false;
    }
    if (sampleRate < 8000 || sampleRate > 48000) {
        Serial.println("[BasicGPTClient] WARNING: sampleRate out of typical voice range (8k-48k)");
    }

    // Prepare JSON envelope sizes
    uint8_t hdr[44];
    buildWavHeader(hdr, sampleRate, bitsPerSample, channels, (uint32_t)pcmBytes);
    const size_t wavBytes = sizeof(hdr) + pcmBytes;
    const size_t b64Bytes = 4 * ((wavBytes + 2) / 3);

    // Build JSON with system as text block and user containing guidelines + audio
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

    const String sys = jsonEscape(String(cfg_.systemPrompt ? cfg_.systemPrompt : ""));
    const String userGuidelines = sys; // mirror system rules into user to reinforce

    String pre = String("{\"model\":\"") + cfg_.model + "\"," \
                 "\"modalities\":[\"text\"]," \
                 "\"messages\":[" \
                 "{\"role\":\"system\",\"content\":[{\"type\":\"text\",\"text\":\"" + sys + "\"}]}," \
                 "{\"role\":\"user\",\"content\":[" \
                 "{\"type\":\"text\",\"text\":\"" + userGuidelines + "\"}," \
                 "{\"type\":\"input_audio\",\"input_audio\":{\"data\":\"";
    String suf = String("\",\"format\":\"wav\"}}]}]}");

    const size_t contentLen = pre.length() + b64Bytes + suf.length();

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
    client.print(pre);

    // Base64 streaming: header then PCM
    const size_t IN_CHUNK = 12288;
    uint8_t* inBuf = (uint8_t*)heap_caps_malloc(IN_CHUNK + 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char*    outBuf = (char*)heap_caps_malloc(16384 + 16, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!inBuf) inBuf = (uint8_t*)malloc(IN_CHUNK + 2);
    if (!outBuf) outBuf = (char*)malloc(16384 + 16);
    if (!inBuf || !outBuf) {
        if (inBuf) free(inBuf);
        if (outBuf) free(outBuf);
        client.stop();
        Serial.println("[BasicGPTClient] ERROR: Out of memory for base64 buffers");
        return false;
    }

    size_t carry = 0;
    // Feed WAV header
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

    // Feed PCM in chunks
    size_t offset = 0;
    while (offset < pcmBytes) {
        size_t tocopy = pcmBytes - offset;
        if (tocopy > IN_CHUNK) tocopy = IN_CHUNK;
        memcpy(inBuf + carry, pcmData + offset, tocopy);
        offset += tocopy;
        n = carry + tocopy;
        proc = (n / 3) * 3;
        wrote = b64EncodeChunk(inBuf, proc, outBuf, false);
        if (wrote) client.write((const uint8_t*)outBuf, wrote);
        carry = n - proc;
        if (carry) {
            inBuf[0] = inBuf[proc];
            if (carry == 2) inBuf[1] = inBuf[proc + 1];
        }
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
    if (status.length() == 0) {
        Serial.println("[BasicGPTClient] ERROR: Empty HTTP status line");
    }
    bool httpOk = (status.indexOf(" 200 ") >= 0 || status.startsWith("HTTP/1.1 200") || status.startsWith("HTTP/1.0 200"));
    if (!httpOk) {
        Serial.print("[BasicGPTClient] HTTP status: "); Serial.println(status);
    }
    while (client.connected()) {
        String hline = client.readStringUntil('\n');
        if (hline == "\r") break;
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
        Serial.println("[BasicGPTClient] ERROR: Could not parse assistant text from response body");
    }
    return outText.length() > 0;
}

bool BasicGPTClient::askAudioFromWav(const char* wavPath, String& outText) {
    outText = "";
    File f = SD_MMC.open(wavPath, FILE_READ);
    if (!f) { Serial.println("[BasicGPTClient] ERROR: Cannot open WAV file"); return false; }

    const size_t wavBytes = f.size();
    const size_t b64Bytes = 4 * ((wavBytes + 2) / 3);

    String pre = String("{\"model\":\"") + cfg_.model +
                 "\",\"modalities\":[\"text\"],"
                 "\"messages\":[{\"role\":\"system\",\"content\":\"" + String(cfg_.systemPrompt ? cfg_.systemPrompt : "") + "\"},"
                 "{\"role\":\"user\",\"content\":[{\"type\":\"input_audio\",\"input_audio\":{\"data\":\"";
    String suf = String("\",\"format\":\"wav\"}}]}]}");

    const size_t contentLen = pre.length() + b64Bytes + suf.length();

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(cfg_.httpTimeoutMs / 1000);

    if (!client.connect(cfg_.host, cfg_.port)) {
        Serial.println("[BasicGPTClient] ERROR: TLS connect failed");
        f.close();
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

    // Body: JSON prefix + base64-streamed WAV + suffix
    client.print(pre);

    const size_t IN_CHUNK = 12288; // 12KB -> 16384 base64
    uint8_t* inBuf = (uint8_t*)heap_caps_malloc(IN_CHUNK + 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char*    outBuf = (char*)heap_caps_malloc(16384 + 16, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!inBuf) inBuf = (uint8_t*)malloc(IN_CHUNK + 2);
    if (!outBuf) outBuf = (char*)malloc(16384 + 16);
    if (!inBuf || !outBuf) {
        if (inBuf) free(inBuf);
        if (outBuf) free(outBuf);
        f.close();
        client.stop();
        Serial.println("[BasicGPTClient] ERROR: Out of memory for base64 buffers");
        return false;
    }
    size_t carry = 0;

    while (f.available()) {
        size_t n = f.read(inBuf + carry, IN_CHUNK);
        n += carry;
        size_t proc = (n / 3) * 3;
        size_t wrote = b64EncodeChunk(inBuf, proc, outBuf, false);
        if (wrote) client.write((const uint8_t*)outBuf, wrote);
        carry = n - proc;
        if (carry) {
            inBuf[0] = inBuf[proc];
            if (carry == 2) inBuf[1] = inBuf[proc + 1];
        }
        yield();
    }
    if (carry) {
        size_t wrote = b64EncodeChunk(inBuf, carry, outBuf, true);
        if (wrote) client.write((const uint8_t*)outBuf, wrote);
    }

    client.print(suf);
    f.close();
    free(inBuf);
    free(outBuf);

    // Read response
    String status = client.readStringUntil('\n');
    if (status.length() == 0) {
        Serial.println("[BasicGPTClient] ERROR: Empty HTTP status line");
    }
    bool httpOk = (status.indexOf(" 200 ") >= 0 || status.startsWith("HTTP/1.1 200") || status.startsWith("HTTP/1.0 200"));
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
        Serial.println("[BasicGPTClient] ERROR: Could not parse assistant text from response body");
    }
    return outText.length() > 0;
}


