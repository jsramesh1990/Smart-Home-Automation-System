#include "include/voice_recognition.h"
#include "include/config.h"
#include <math.h>
#include <string.h>

#define PRE_EMPHASIS_COEFF 0.97f
#define FRAME_SIZE 512
#define HOP_SIZE 256
#define FFT_SIZE 512
#define NUM_MEL_FILTERS 26
#define NUM_CEPSTRAL 13

// Pre-computed mel filterbank (simplified)
static float mel_filterbank[NUM_MEL_FILTERS][FFT_SIZE/2 + 1];

VoiceRecognizer::VoiceRecognizer() {
    profileCount = 0;
    commandCount = 0;
    bufferIndex = 0;
    commands = nullptr;
    
    // Initialize all profiles
    for (int i = 0; i < MAX_VOICE_PROFILES; i++) {
        profiles[i].active = false;
    }
}

void VoiceRecognizer::begin() {
    // Initialize audio input
    // Setup I2S for microphone
    
    // Initialize mel filterbank
    for (int i = 0; i < NUM_MEL_FILTERS; i++) {
        for (int j = 0; j <= FFT_SIZE/2; j++) {
            mel_filterbank[i][j] = 1.0f / NUM_MEL_FILTERS;
        }
    }
    
    Serial.println("Voice recognizer initialized");
}

void VoiceRecognizer::processAudioSample(int16_t sample) {
    // Apply pre-emphasis
    static int16_t prevSample = 0;
    int16_t filtered = sample - PRE_EMPHASIS_COEFF * prevSample;
    prevSample = sample;
    
    // Store in buffer
    audioBuffer[bufferIndex++] = filtered;
    
    // Process when buffer is full
    if (bufferIndex >= VOICE_BUFFER_SIZE) {
        detectWakeWord();
        bufferIndex = 0;
    }
}

float VoiceRecognizer::calculateEnergy(const int16_t* samples, int count) {
    float energy = 0;
    for (int i = 0; i < count; i++) {
        energy += samples[i] * samples[i];
    }
    return energy / count;
}

float VoiceRecognizer::calculateZeroCrossingRate(const int16_t* samples, int count) {
    int crossings = 0;
    for (int i = 1; i < count; i++) {
        if ((samples[i] >= 0 && samples[i-1] < 0) || 
            (samples[i] < 0 && samples[i-1] >= 0)) {
            crossings++;
        }
    }
    return (float)crossings / count;
}

void VoiceRecognizer::extractMFCC(const int16_t* samples, float* features) {
    // Simplified MFCC extraction
    // In production, implement proper FFT and mel filtering
    
    float energy = calculateEnergy(samples, FRAME_SIZE);
    float zcr = calculateZeroCrossingRate(samples, FRAME_SIZE);
    
    features[0] = energy;
    features[1] = zcr;
    
    // Fill remaining with derived values
    for (int i = 2; i < VOICE_FEATURE_SIZE; i++) {
        features[i] = (energy * (i % 10) / 10.0f) + (zcr * ((i+5) % 10) / 10.0f);
    }
}

float VoiceRecognizer::calculateDistance(const float* f1, const float* f2, int size) {
    float distance = 0;
    for (int i = 0; i < size; i++) {
        float diff = f1[i] - f2[i];
        distance += diff * diff;
    }
    return sqrt(distance);
}

void VoiceRecognizer::detectWakeWord() {
    float features[VOICE_FEATURE_SIZE];
    extractMFCC(audioBuffer, features);
    
    for (int i = 0; i < profileCount; i++) {
        if (profiles[i].active) {
            float distance = calculateDistance(features, profiles[i].features, VOICE_FEATURE_SIZE);
            if (distance < profiles[i].threshold) {
                Serial.print("Wake word detected for user: ");
                Serial.println(profiles[i].userId);
                processCommands();
                break;
            }
        }
    }
}

bool VoiceRecognizer::enrollVoice(const char* userId, const char* wakeWord) {
    if (profileCount >= MAX_VOICE_PROFILES) {
        return false;
    }
    
    // Collect multiple samples for enrollment
    float aggregatedFeatures[VOICE_FEATURE_SIZE] = {0};
    int sampleCount = 3;
    
    for (int s = 0; s < sampleCount; s++) {
        // Wait for audio sample
        delay(1000);
        
        float features[VOICE_FEATURE_SIZE];
        extractMFCC(audioBuffer, features);
        
        for (int i = 0; i < VOICE_FEATURE_SIZE; i++) {
            aggregatedFeatures[i] += features[i];
        }
    }
    
    // Average the features
    for (int i = 0; i < VOICE_FEATURE_SIZE; i++) {
        profiles[profileCount].features[i] = aggregatedFeatures[i] / sampleCount;
    }
    
    strncpy(profiles[profileCount].userId, userId, 31);
    strncpy(profiles[profileCount].wakeWord, wakeWord, 15);
    profiles[profileCount].threshold = 0.75f;  // Default threshold
    profiles[profileCount].active = true;
    
    profileCount++;
    return true;
}

bool VoiceRecognizer::verifyVoice(const char* userId) {
    float features[VOICE_FEATURE_SIZE];
    extractMFCC(audioBuffer, features);
    
    for (int i = 0; i < profileCount; i++) {
        if (strcmp(profiles[i].userId, userId) == 0 && profiles[i].active) {
            float distance = calculateDistance(features, profiles[i].features, VOICE_FEATURE_SIZE);
            return distance < profiles[i].threshold;
        }
    }
    return false;
}

void VoiceRecognizer::registerCommand(const char* command, void (*callback)()) {
    VoiceCommand* newCommands = new VoiceCommand[commandCount + 1];
    
    if (commands) {
        memcpy(newCommands, commands, sizeof(VoiceCommand) * commandCount);
        delete[] commands;
    }
    
    strncpy(newCommands[commandCount].command, command, MAX_VOICE_COMMAND_LEN - 1);
    newCommands[commandCount].callback = callback;
    
    commands = newCommands;
    commandCount++;
}

void VoiceRecognizer::processCommands() {
    // Simplified command processing
    // In production, implement proper speech-to-text
    
    for (int i = 0; i < commandCount; i++) {
        if (commands[i].callback) {
            commands[i].callback();
        }
    }
}

void VoiceRecognizer::trainModel() {
    // Train voice recognition model
    // Implement machine learning model training
}

bool VoiceRecognizer::deleteProfile(const char* userId) {
    for (int i = 0; i < profileCount; i++) {
        if (strcmp(profiles[i].userId, userId) == 0) {
            profiles[i].active = false;
            return true;
        }
    }
    return false;
}

void VoiceRecognizer::listProfiles(char* output, int maxLen) {
    output[0] = '\0';
    for (int i = 0; i < profileCount; i++) {
        if (profiles[i].active) {
            char line[64];
            snprintf(line, sizeof(line), "%s:%s\n", profiles[i].userId, profiles[i].wakeWord);
            strncat(output, line, maxLen - strlen(output) - 1);
        }
    }
}
