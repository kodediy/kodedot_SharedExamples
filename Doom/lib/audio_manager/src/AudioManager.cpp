#include "audio_manager/AudioManager.h"
#include <esp_heap_caps.h>
#include <math.h>
#include <cstring>
#include <TCA9555.h>

// Use ESP-IDF I2S driver for speaker functionality only
extern "C" {
    #include <driver/i2s.h>
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioManager::AudioManager() 
    : state_(RecordingState::Idle)
    , stateCallback_(nullptr)
    , pcmBuffer_(nullptr)
    , pcmCapacity_(0)
    , pcmWritten_(0)
    , recordStartMs_(0)
    , captureTask_(nullptr)
    , speakerInitialized_(false)
    , toneSessionActive_(false) {
}

AudioManager::~AudioManager() {
    if (captureTask_) {
        vTaskDelete(captureTask_);
    }
    freePCMBuffer();
}

bool AudioManager::init(const AudioConfig& config) {
    config_ = config;
    
    // For now, just store configuration for speaker functionality
    Serial.println("[AudioManager] Initialized for speaker functionality");
    
    setState(RecordingState::Idle);
    return true;
}

void AudioManager::attachExpander(TCA9555* expander, int8_t ampPin) {
    expander_ = expander;
    ampPin_ = ampPin;
    if (expander_ && ampPin_ >= 0) {
    // Configure amplifier control pin as output and default LOW (disabled)
    expander_->pinMode1(ampPin_, OUTPUT);   // Library handles 0-15 internally
    expander_->write1(ampPin_, LOW);
    }
}

void AudioManager::ensureAmplifier(bool enable) {
    if (expander_ && ampPin_ >= 0) {
    expander_->write1(ampPin_, enable ? HIGH : LOW);
    }
}

void AudioManager::setStateCallback(StateCallback callback) {
    stateCallback_ = callback;
}

bool AudioManager::startRecording() {
    // Recording functionality disabled for now to avoid I2S conflicts
    Serial.println("[AudioManager] Recording not supported in current implementation");
    return false;
}

void AudioManager::stopRecording() {
    // Recording functionality disabled for now
    Serial.println("[AudioManager] Recording not supported in current implementation");
}

uint8_t* AudioManager::getPCMBuffer() const {
    return pcmBuffer_;
}

size_t AudioManager::getPCMSize() const {
    return pcmWritten_;
}

void AudioManager::releasePCMBuffer() {
    // Disabled for now
}

void AudioManager::resetToIdle() {
    setState(RecordingState::Idle);
}

void AudioManager::setState(RecordingState newState) {
    if (state_ != newState) {
        state_ = newState;
        Serial.printf("[AudioManager] State changed to %d\n", (int)state_);
        if (stateCallback_) {
            stateCallback_(state_);
        }
    }
}

void AudioManager::captureTaskImpl(void* param) {
    // Recording functionality disabled
}

void AudioManager::captureLoop() {
    // Recording functionality disabled
}

bool AudioManager::allocatePCMBuffer() {
    // Disabled for now
    return false;
}

void AudioManager::freePCMBuffer() {
    // Disabled for now  
}

void AudioManager::service() {
    // Service function - for future use
}

void AudioManager::normalizeAndLimitPCM16(uint8_t* data, size_t numBytes, 
                                        float targetPeakFS, float kneeFS, float ratio) {
    // Disabled for now - would be used for recording normalization
}

// Speaker playback functions
bool AudioManager::initSpeaker() {
    if (speakerInitialized_) {
        return true;
    }
    
    if (config_.spkSckPin == -1 || config_.spkWsPin == -1 || config_.spkDoutPin == -1) {
        Serial.println("[AudioManager] Speaker pins not configured");
        return false;
    }
    
    // Configure I2S for speaker output using ESP-IDF driver
    ::i2s_config_t i2s_config = {
        .mode = (::i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    ::i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = config_.spkSckPin,
        .ws_io_num = config_.spkWsPin,
        .data_out_num = config_.spkDoutPin,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t result = ::i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("[AudioManager] I2S driver install failed: %d\n", result);
        return false;
    }

    result = ::i2s_set_pin(I2S_NUM_0, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("[AudioManager] I2S pin config failed: %d\n", result);
        ::i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }

    speakerInitialized_ = true;
    Serial.println("[AudioManager] Speaker initialized");
    return true;
}

void AudioManager::deinitSpeaker() {
    if (!speakerInitialized_) {
        return;
    }
    
    ::i2s_driver_uninstall(I2S_NUM_0);
    speakerInitialized_ = false;
    Serial.println("[AudioManager] Speaker deinitialized");
}

bool AudioManager::playCompletionSound() {
    if (!initSpeaker()) {
        return false;
    }
    ensureAmplifier(true);
    delay(8); // small settle time
    
    const uint32_t sampleRate = 44100;
    const float duration = 0.5f; // 500ms
    const uint32_t samples = (uint32_t)(sampleRate * duration);
    const uint16_t amplitude = 8000; // Volume level
    
    // Generate a pleasant two-tone chime
    int16_t* audioBuffer = (int16_t*)malloc(samples * sizeof(int16_t));
    if (!audioBuffer) {
        Serial.println("[AudioManager] Failed to allocate audio buffer");
        return false;
    }
    
    for (uint32_t i = 0; i < samples; i++) {
        float t = (float)i / sampleRate;
        float envelope = exp(-t * 3.0f); // Exponential decay
        
        // Two-tone chime: 800Hz + 1200Hz
        float wave1 = sin(2.0f * M_PI * 800.0f * t);
        float wave2 = sin(2.0f * M_PI * 1200.0f * t);
        float combined = (wave1 + wave2 * 0.7f) * envelope;
        
        audioBuffer[i] = (int16_t)(combined * amplitude);
    }
    
    // Play the sound
    size_t bytes_written = 0;
    esp_err_t result = ::i2s_write(I2S_NUM_0, audioBuffer, samples * sizeof(int16_t), 
                                &bytes_written, pdMS_TO_TICKS(1000));
    
    free(audioBuffer);
    
    // Wait for playback to complete
    delay(600);
    
    ensureAmplifier(false);
    deinitSpeaker();
    
    if (result == ESP_OK) {
        Serial.println("[AudioManager] Completion sound played");
        return true;
    } else {
        Serial.printf("[AudioManager] Failed to play sound: %d\n", result);
        return false;
    }
}

bool AudioManager::playBeep(uint16_t freq, uint16_t durationMs, uint16_t amplitude) {
    if (!beginToneSession()) return false;
    bool ok = playToneChunk(freq, durationMs, amplitude);
    endToneSession();
    return ok;
}

bool AudioManager::beginToneSession() {
    if (!initSpeaker()) return false;
    ensureAmplifier(true);
    delay(2); // short settle
    toneSessionActive_ = true;
    return true;
}

bool AudioManager::playToneChunk(uint16_t freq, uint16_t durationMs, uint16_t amplitude) {
    if (!speakerInitialized_) return false;
    const uint32_t sampleRate = 44100;
    uint32_t totalSamples = (sampleRate * durationMs) / 1000;
    size_t bytesWritten = 0;
    // Larger chunk to reduce ISR scheduling overhead
    const uint16_t chunk = 512;
    int16_t buffer[chunk];
    uint32_t periodSamples = sampleRate / (freq * 2); // half period for square wave
    int16_t current = amplitude;
    uint32_t written = 0;
    while (written < totalSamples) {
        uint32_t toGen = std::min<uint32_t>(chunk, totalSamples - written);
        uint32_t local = 0;
        for (uint32_t i = 0; i < toGen; ++i) {
            if (local++ >= periodSamples) { local = 0; current = -current; }
            buffer[i] = current;
        }
        esp_err_t r = ::i2s_write(I2S_NUM_0, buffer, toGen * sizeof(int16_t), &bytesWritten, pdMS_TO_TICKS(50));
        if (r != ESP_OK) return false;
        written += toGen;
    }
    return true;
}

void AudioManager::endToneSession() {
    if (!toneSessionActive_) return;
    ensureAmplifier(false);
    deinitSpeaker();
    toneSessionActive_ = false;
}
