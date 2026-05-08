#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "include/network_client.h"
#include "include/voice_processor.h"
#include "include/ui_controller.h"

static NetworkClient network;
static VoiceProcessor voice;
static UIController ui;

void displayBanner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                          ║\n";
    std::cout << "║     🔐 SmartLock Pro - Mobile Application Simulator     ║\n";
    std::cout << "║                                                          ║\n";
    std::cout << "║     Voice-Controlled Smart Lock System                   ║\n";
    std::cout << "║                                                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void displayMenu() {
    std::cout << "\n┌─────────────────────────────────────────────────────────┐\n";
    std::cout << "│                    MAIN MENU                            │\n";
    std::cout << "├─────────────────────────────────────────────────────────┤\n";
    std::cout << "│  1. 🔒 Lock Door                                        │\n";
    std::cout << "│  2. 🔓 Unlock Door                                      │\n";
    std::cout << "│  3. 📊 View Status                                      │\n";
    std::cout << "│  4. 📜 View Access Logs                                 │\n";
    std::cout << "│  5. 🎫 Create Guest Code                                │\n";
    std::cout << "│  6. 🎤 Voice Recognition (Demo)                         │\n";
    std::cout << "│  7. 👤 Enroll Voice Profile                             │\n";
    std::cout << "│  8. 🔄 Refresh                                          │\n";
    std::cout << "│  9. 🚪 Exit                                             │\n";
    std::cout << "└─────────────────────────────────────────────────────────┘\n";
    std::cout << "Choice: ";
}

void displayStatus(const LockStatus& status) {
    std::cout << "\n┌──────────────────┬─────────────────────────────────────┐\n";
    std::cout << "│     STATUS        │                                     │\n";
    std::cout << "├──────────────────┼─────────────────────────────────────┤\n";
    std::cout << "│ Door State       │ " << (status.locked ? "🔒 LOCKED" : "🔓 UNLOCKED") << "                   │\n";
    std::cout << "│ Battery          │ " << status.battery_percentage << "% " << ui.getBatteryIcon() << "                              │\n";
    std::cout << "│ WiFi Signal      │ " << status.wifi_rssi << " dBm " << ui.getWifiIcon() << "                          │\n";
    std::cout << "│ Tamper Status    │ " << (status.tamper_detected ? "⚠️ DETECTED" : "✓ OK") << "                      │\n";
    std::cout << "│ Door Open        │ " << (status.door_open ? "Open" : "Closed") << "                              │\n";
    std::cout << "└──────────────────┴─────────────────────────────────────┘\n";
}

void displayLogs(const std::vector<AccessEvent>& logs) {
    std::cout << "\n┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                           RECENT ACCESS LOGS                                │\n";
    std::cout << "├──────┬──────────────────┬────────────┬────────────┬─────────────────────────┤\n";
    std::cout << "│ Time │ User             │ Action     │ Method     │ Success                 │\n";
    std::cout << "├──────┼──────────────────┼────────────┼────────────┼─────────────────────────┤\n";
    
    int count = 0;
    for (const auto& log : logs) {
        if (count++ >= 10) break;
        std::string time_str = ui.formatTimestamp(log.timestamp);
        std::string success_str = log.success ? "✓ Yes" : "✗ No";
        
        printf("│ %-4s │ %-16s │ %-10s │ %-10s │ %-23s │\n",
               time_str.substr(11, 5).c_str(),
               log.username.substr(0, 16).c_str(),
               log.action.substr(0, 10).c_str(),
               log.method.substr(0, 10).c_str(),
               success_str.c_str());
    }
    
    std::cout << "└──────┴──────────────────┴────────────┴────────────┴─────────────────────────┘\n";
}

void displayGuestCodes(const std::vector<GuestCode>& codes) {
    std::cout << "\n┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                           ACTIVE GUEST CODES                               │\n";
    std::cout << "├──────────┬──────────────────┬─────────────────────┬──────────┬─────────────┤\n";
    std::cout << "│ Code     │ Created By       │ Expires             │ Uses Left │ Status      │\n";
    std::cout << "├──────────┼──────────────────┼─────────────────────┼──────────┼─────────────┤\n";
    
    for (const auto& code : codes) {
        std::string expires = ui.formatTimestamp(code.expires_at);
        std::string status = code.active ? "Active" : "Expired";
        
        printf("│ %-8s │ %-16s │ %-19s │ %-8d │ %-11s │\n",
               code.code.c_str(),
               code.created_by.substr(0, 16).c_str(),
               expires.substr(0, 19).c_str(),
               code.uses_remaining,
               status.c_str());
    }
    
    std::cout << "└──────────┴──────────────────┴─────────────────────┴──────────┴─────────────┘\n";
}

void onVoiceCommand(const std::string& command, float confidence) {
    std::cout << "\n🎤 Voice command detected: \"" << command 
              << "\" (confidence: " << (int)(confidence * 100) << "%)\n";
    
    if (command == "lock") {
        ui.lockDoor();
    } else if (command == "unlock") {
        ui.unlockDoor();
    } else if (command == "status") {
        ui.refreshStatus();
    }
}

void onStatusUpdate(const LockStatus& status) {
    displayStatus(status);
}

void onLogUpdate(const std::vector<AccessEvent>& logs) {
    displayLogs(logs);
}

void onNotification(const std::string& message, bool is_error) {
    if (is_error) {
        std::cout << "\n❌ " << message << std::endl;
    } else {
        std::cout << "\n✅ " << message << std::endl;
    }
}

void simulateVoiceListening() {
    std::cout << "\n🎤 Voice recognition active...\n";
    std::cout << "   Say 'Hey SmartLock' followed by:\n";
    std::cout << "   - 'lock' - to lock the door\n";
    std::cout << "   - 'unlock' - to unlock the door\n";
    std::cout << "   - 'status' - to check status\n";
    std::cout << "\n   Type 'stop' to exit voice mode\n\n";
    
    voice.startListening();
    
    std::string input;
    while (true) {
        std::cout << "Simulate voice command: ";
        std::getline(std::cin, input);
        
        if (input == "stop") {
            break;
        }
        
        // Simulate voice command
        float confidence = 0.85f;
        
        if (input == "lock" || input == "lock the door") {
            onVoiceCommand("lock", confidence);
        } else if (input == "unlock" || input == "unlock the door") {
            onVoiceCommand("unlock", confidence);
        } else if (input == "status") {
            onVoiceCommand("status", confidence);
        } else {
            std::cout << "   Unknown command: " << input << std::endl;
        }
    }
    
    voice.stopListening();
}

int main() {
    displayBanner();
    
    // Initialize components
    network.setApiBaseUrl("http://localhost:8080");
    voice.initialize();
    voice.setWakeWord("hey smartlock");
    voice.setCommandCallback(onVoiceCommand);
    
    ui.setStatusCallback(onStatusUpdate);
    ui.setLogCallback(onLogUpdate);
    ui.setNotificationCallback(onNotification);
    
    // Login
    std::string username, password;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);
    
    std::cout << "\nAuthenticating...\n";
    
    network.login(username, password, [&](const ApiResponse& response) {
        if (response.success) {
            std::cout << "✓ Login successful!\n";
            ui.setAuthToken(network.getAuthToken());
            ui.start();
        } else {
            std::cout << "✗ Login failed: " << response.error << "\n";
            std::cout << "Continuing in demo mode...\n";
        }
    });
    
    // Main loop
    bool running = true;
    while (running) {
        displayMenu();
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case 1:
                ui.lockDoor();
                break;
            case 2:
                ui.unlockDoor();
                break;
            case 3:
                ui.refreshStatus();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case 4:
                ui.refreshLogs();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case 5: {
                int hours, uses;
                std::cout << "Duration (hours): ";
                std::cin >> hours;
                std::cout << "Max uses: ";
                std::cin >> uses;
                std::cin.ignore();
                ui.createGuestCode(hours, uses);
                break;
            }
            case 6:
                simulateVoiceListening();
                break;
            case 7: {
                std::string user_id, wake_word;
                std::cout << "User ID: ";
                std::getline(std::cin, user_id);
                std::cout << "Wake word (e.g., 'hey smartlock'): ";
                std::getline(std::cin, wake_word);
                network.enrollVoice(user_id, wake_word, [](const ApiResponse& response) {
                    if (response.success) {
                        std::cout << "✓ Voice profile enrolled\n";
                    } else {
                        std::cout << "✗ Enrollment failed\n";
                    }
                });
                break;
            }
            case 8:
                ui.refreshStatus();
                ui.refreshLogs();
                std::cout << "Refreshed\n";
                break;
            case 9:
                running = false;
                std::cout << "Goodbye!\n";
                break;
            default:
                std::cout << "Invalid choice\n";
        }
    }
    
    ui.stop();
    return 0;
}
