#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>

// Forward declare TCA9555 to avoid pulling in header here
class TCA9555;

// Forward declarations to avoid header conflicts
class I2SClass;

enum class RecordingState { 
    Idle, 
    Recording, 
    Saving, 
    Saved, 
    Error 
};

struct AudioConfig {
    uint32_t sampleRate = 32000;
    uint16_t bitsPerSample = 16;
    uint16_t numChannels = 1;
    uint32_t maxRecordingMs = 5000;
    int8_t sckPin = -1;
    int8_t wsPin = -1;
    int8_t dinPin = -1;
    // Speaker pins for playback
    int8_t spkSckPin = -1;
    int8_t spkWsPin = -1; 
    int8_t spkDoutPin = -1;
    // Optional: expander-controlled amplifier pin
    int8_t ampExpanderPin = -1; // pin index on TCA9555 that must go HIGH to enable amp
};

typedef std::function<void(RecordingState)> StateCallback;

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    
    bool init(const AudioConfig& config);
    void setStateCallback(StateCallback callback);
    
    // Recording control
    bool startRecording();
    void stopRecording();
    RecordingState getState() const { return state_; }
    
    // Audio data access
    uint8_t* getPCMBuffer() const;
    size_t getPCMSize() const;
    void releasePCMBuffer();
    void resetToIdle(); // Reset state to idle after processing
    
    // Processing
    static void normalizeAndLimitPCM16(uint8_t* data, size_t numBytes, 
                                     float targetPeakFS = 0.89f, 
                                     float kneeFS = 0.707f, 
                                     float ratio = 3.0f);
    
    // Sound playback for timer completion
    bool playCompletionSound();
    bool playBeep(uint16_t freq = 1200, uint16_t durationMs = 80, uint16_t amplitude = 6000);
    bool initSpeaker();
    void deinitSpeaker();
    void attachExpander(TCA9555* expander, int8_t ampPin);

    // Low-latency tone session APIs for continuous short beeps without re-init/deinit overhead
    bool beginToneSession();              // init I2S once and enable amplifier
    bool playToneChunk(uint16_t freq, uint16_t durationMs, uint16_t amplitude);
    void endToneSession();                // disable amplifier and deinit I2S
    
    // Service function - call from main loop
    void service();
    
private:
    AudioConfig config_;
    RecordingState state_;
    StateCallback stateCallback_;
    
    // Recording data
    uint8_t* pcmBuffer_;
    size_t pcmCapacity_;
    size_t pcmWritten_;
    uint32_t recordStartMs_;
    
    // Task management
    TaskHandle_t captureTask_;
    
    // Speaker management
    bool speakerInitialized_;
    TCA9555* expander_ = nullptr;
    int8_t ampPin_ = -1;
    void ensureAmplifier(bool enable);
    bool toneSessionActive_ = false;
    
    // Internal methods
    void setState(RecordingState newState);
    static void captureTaskImpl(void* param);
    void captureLoop();
    bool allocatePCMBuffer();
    void freePCMBuffer();
};

#endif // AUDIO_MANAGER_H
