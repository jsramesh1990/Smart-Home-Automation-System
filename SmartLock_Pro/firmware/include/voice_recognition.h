#ifndef VOICE_RECOGNITION_H
#define VOICE_RECOGNITION_H

#include <Arduino.h>

#define MAX_VOICE_PROFILES 10
#define VOICE_FEATURE_SIZE 128
#define MAX_VOICE_COMMAND_LEN 32

struct VoiceProfile {
    char userId[32];
    char wakeWord[16];
    float features[VOICE_FEATURE_SIZE];
    float threshold;
    bool active;
};

struct VoiceCommand {
    char command[MAX_VOICE_COMMAND_LEN];
    void (*callback)();
};

class VoiceRecognizer {
private:
    VoiceProfile profiles[MAX_VOICE_PROFILES];
    int profileCount;
    VoiceCommand* commands;
    int commandCount;
    
    int16_t audioBuffer[VOICE_BUFFER_SIZE];
    int bufferIndex;
    
    float calculateEnergy(const int16_t* samples, int count);
    float calculateZeroCrossingRate(const int16_t* samples, int count);
    void extractMFCC(const int16_t* samples, float* features);
    float calculateDistance(const float* f1, const float* f2, int size);
    void detectWakeWord();
    
public:
    VoiceRecognizer();
    void begin();
    void processAudioSample(int16_t sample);
    bool enrollVoice(const char* userId, const char* wakeWord);
    bool verifyVoice(const char* userId);
    void registerCommand(const char* command, void (*callback)());
    void processCommands();
    void trainModel();
    bool deleteProfile(const char* userId);
    void listProfiles(char* output, int maxLen);
};

#endif
