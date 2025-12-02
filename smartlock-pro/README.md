# SmartLock Pro

A comprehensive smart lock system with mobile app control, voice recognition, and remote access management.

## Features
- **Remote Locking/Unlocking**: Control your lock from anywhere
- **Access Logs**: Track all lock activities
- **Temporary Guest Codes**: Generate time-limited access codes
- **Auto-lock**: Automatic locking after configurable delay
- **Voice Control**: Hands-free operation with voice recognition
- **Multi-user Support**: Recognize different family members' voices

## Tech Stack
- **Backend**: Node.js, Express, MongoDB, Socket.IO
- **Mobile App**: React Native, Expo
- **Hardware**: ESP32, Relay Module, Solenoid Lock
- **Voice Processing**: Expo Audio, Natural Language Processing

## Installation

### Backend Setup
```bash
cd backend
npm install
cp .env.example .env
# Configure environment variables
npm start

cd mobile-app
npm install
# Configure API endpoint in config.js
npm start
