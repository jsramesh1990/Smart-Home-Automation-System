#include "include/voice_processor.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>

VoiceProcessor::VoiceProcessor() {
    listening = false;
    sample_rate = 16000;
    frame_size = 512;
    hop_size = 256;
    wake_word_threshold = 0.7f;
    command_threshold = 0.6f;
    wake_word = "hey smartlock";
}

VoiceProcessor::~VoiceProcessor() {
    stopListening();
}

bool VoiceProcessor::initialize(int sample_rate_hz) {
    sample_rate = sample_rate_hz;
    audio_buffer.reserve(sample_rate * 2); // 2 seconds buffer
    
    // Initialize default commands
    VoiceCommand lock_cmd;
    lock_cmd.command = "lock";
    lock_cmd.action = "lock";
    lock_cmd.aliases = {"lock the door", "secure the door", "lock it"};
    commands.push_back(lock_cmd);
    
    VoiceCommand unlock_cmd;
    unlock_cmd.command = "unlock";
    unlock_cmd.action = "unlock";
    unlock_cmd.aliases = {"unlock the door", "open the door", "let me in"};
    commands.push_back(unlock_cmd);
    
    VoiceCommand status_cmd;
    status_cmd.command = "status";
    status_cmd.action = "status";
    status_cmd.aliases = {"what's the status", "is the door locked", "check status"};
    commands.push_back(status_cmd);
    
    return true;
}

void VoiceProcessor::startListening() {
    listening = true;
    audio_buffer.clear();
}

void VoiceProcessor::stopListening() {
    listening = false;
}

void VoiceProcessor::setWakeWord(const std::string& word, float threshold) {
    wake_word = word;
    wake_word_threshold = threshold;
}

void VoiceProcessor::setCommandCallback(VoiceCommandCallback cb) {
    callback = cb;
}

void VoiceProcessor::feedAudioSample(int16_t sample) {
    if (!listening) return;
    
    audio_buffer.push_back(sample);
    
    // Process when we have enough samples
    if (audio_buffer.size() >= (size_t)sample_rate) {  // 1 second of audio
        processAudio(audio_buffer.data(), audio_buffer.size());
        audio_buffer.clear();
    }
}

void VoiceProcessor::feedAudioBuffer(const int16_t* samples, size_t count) {
    if (!listening) return;
    
    for (size_t i = 0; i < count; i++) {
        audio_buffer.push_back(samples[i]);
        
        if (audio_buffer.size() >= (size_t)sample_rate) {
            processAudio(audio_buffer.data(), audio_buffer.size());
            audio_buffer.clear();
        }
    }
}

void VoiceProcessor::processAudio(const int16_t* samples, size_t count) {
    std::vector<float> features;
    extractFeatures(samples, count, features);
    
    // First detect wake word
    detectWakeWord(features);
}

void VoiceProcessor::extractFeatures(const int16_t* samples, size_t count, std::vector<float>& features) {
    features.clear();
    
    // Calculate energy
    float energy = 0;
    for (size_t i = 0; i < count; i++) {
        energy += samples[i] * samples[i];
    }
    energy /= count;
    features.push_back(energy);
    
    // Calculate zero crossing rate
    int crossings = 0;
    for (size_t i = 1; i < count; i++) {
        if ((samples[i] >= 0 && samples[i-1] < 0) || 
            (samples[i] < 0 && samples[i-1] >= 0)) {
            crossings++;
        }
    }
    float zcr = (float)crossings / count;
    features.push_back(zcr);
    
    // Calculate spectral centroid (simplified)
    float spectral_centroid = 0;
    for (size_t i = 0; i < count / 2; i++) {
        spectral_centroid += abs(samples[i]) * i;
    }
    features.push_back(spectral_centroid);
    
    // Add some random features for demo (in production, would use MFCC)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1, 1);
    
    for (int i = 0; i < 13; i++) {
        features.push_back(dis(gen));
    }
}

void VoiceProcessor::detectWakeWord(const std::vector<float>& features) {
    // Compare with enrolled profiles
    for (const auto& profile : profiles) {
        float distance = calculateDistance(features, profile.voice_features);
        float similarity = 1.0f / (1.0f + distance);
        
        if (similarity > wake_word_threshold) {
            // Wake word detected, now listen for command
            recognizeCommand(features);
            break;
        }
    }
}

void VoiceProcessor::recognizeCommand(const std::vector<float>& features) {
    // Simple simulation - in production would use actual speech recognition
    float best_confidence = 0;
    std::string best_command;
    
    for (const auto& cmd : commands) {
        // Simulated confidence based on command
        float confidence = 0.5f;
        
        // Random variation for demo
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-0.2, 0.2);
        confidence += dis(gen);
        
        if (confidence > best_confidence) {
            best_confidence = confidence;
            best_command = cmd.command;
        }
    }
    
    if (best_confidence > command_threshold && callback) {
        callback(best_command, best_confidence);
    }
}

float VoiceProcessor::calculateDistance(const std::vector<float>& a, const std::vector<float>& b) {
    float distance = 0;
    size_t min_size = std::min(a.size(), b.size());
    
    for (size_t i = 0; i < min_size; i++) {
        float diff = a[i] - b[i];
        distance += diff * diff;
    }
    
    return std::sqrt(distance);
}

std::string VoiceProcessor::normalizeCommand(const std::string& command) {
    std::string normalized = command;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove punctuation
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(),
        [](char c) { return std::ispunct(c); }), normalized.end());
    
    return normalized;
}

bool VoiceProcessor::addCommand(const VoiceCommand& command) {
    commands.push_back(command);
    return true;
}

bool VoiceProcessor::removeCommand(const std::string& command) {
    auto it = std::find_if(commands.begin(), commands.end(),
        [&command](const VoiceCommand& cmd) { return cmd.command == command; });
    
    if (it != commands.end()) {
        commands.erase(it);
        return true;
    }
    return false;
}

std::vector<VoiceCommand> VoiceProcessor::getCommands() {
    return commands;
}

bool VoiceProcessor::enrollProfile(const std::string& user_id, const std::string& username) {
    VoiceProfile profile;
    profile.user_id = user_id;
    profile.username = username;
    profile.enrolled_at = time(nullptr);
    profile.threshold = 0.75f;
    
    // Generate random voice features for demo
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1, 1);
    
    profile.voice_features.resize(128);
    for (size_t i = 0; i < profile.voice_features.size(); i++) {
        profile.voice_features[i] = dis(gen);
    }
    
    profiles.push_back(profile);
    return true;
}

bool VoiceProcessor::verifyProfile(const std::string& user_id, const std::vector<float>& features) {
    for (const auto& profile : profiles) {
        if (profile.user_id == user_id) {
            float distance = calculateDistance(features, profile.voice_features);
            float similarity = 1.0f / (1.0f + distance);
            return similarity > profile.threshold;
        }
    }
    return false;
}

bool VoiceProcessor::deleteProfile(const std::string& user_id) {
    auto it = std::find_if(profiles.begin(), profiles.end(),
        [&user_id](const VoiceProfile& profile) { return profile.user_id == user_id; });
    
    if (it != profiles.end()) {
        profiles.erase(it);
        return true;
    }
    return false;
}

std::vector<VoiceProfile> VoiceProcessor::getProfiles() {
    return profiles;
}

bool VoiceProcessor::loadAudioFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<int16_t> samples(size / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(samples.data()), size);
    file.close();
    
    feedAudioBuffer(samples.data(), samples.size());
    return true;
}
