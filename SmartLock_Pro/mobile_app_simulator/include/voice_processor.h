#ifndef VOICE_PROCESSOR_H
#define VOICE_PROCESSOR_H

#include <string>
#include <vector>
#include <functional>
#include <mutex>

struct VoiceCommand {
    std::string command;
    std::string action;
    std::vector<std::string> aliases;
    float confidence;
};

struct VoiceProfile {
    std::string user_id;
    std::string username;
    std::vector<float> voice_features;
    float threshold;
    time_t enrolled_at;
};

typedef std::function<void(const std::string& command, float confidence)> VoiceCommandCallback;

class VoiceProcessor {
private:
    std::vector<VoiceCommand> commands;
    std::vector<VoiceProfile> profiles;
    VoiceCommandCallback callback;
    
    bool listening;
    std::string wake_word;
    float wake_word_threshold;
    float command_threshold;
    
    std::mutex processor_mutex;
    
    // Audio processing
    std::vector<int16_t> audio_buffer;
    int sample_rate;
    int frame_size;
    int hop_size;
    
    void processAudio(const int16_t* samples, size_t count);
    void extractFeatures(const int16_t* samples, size_t count, std::vector<float>& features);
    void detectWakeWord(const std::vector<float>& features);
    void recognizeCommand(const std::vector<float>& features);
    float calculateDistance(const std::vector<float>& a, const std::vector<float>& b);
    std::string normalizeCommand(const std::string& command);
    
public:
    VoiceProcessor();
    ~VoiceProcessor();
    
    bool initialize(int sample_rate_hz = 16000);
    void startListening();
    void stopListening();
    void setWakeWord(const std::string& word, float threshold = 0.7f);
    void setCommandCallback(VoiceCommandCallback cb);
    
    // Command management
    bool addCommand(const VoiceCommand& command);
    bool removeCommand(const std::string& command);
    std::vector<VoiceCommand> getCommands();
    
    // Profile management
    bool enrollProfile(const std::string& user_id, const std::string& username);
    bool verifyProfile(const std::string& user_id, const std::vector<float>& features);
    bool deleteProfile(const std::string& user_id);
    std::vector<VoiceProfile> getProfiles();
    
    // Audio input (simulated for testing)
    void feedAudioSample(int16_t sample);
    void feedAudioBuffer(const int16_t* samples, size_t count);
    bool loadAudioFromFile(const std::string& filename);
    
    // Status
    bool isListening() const { return listening; }
    std::string getWakeWord() const { return wake_word; }
};

#endif
