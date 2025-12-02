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

Additional Important Files

**mobile-app/services/VoiceProcessor.js** - Handles voice recognition
**backend/models/User.js** - User schema with voice profiles
**backend/models/AccessLog.js** - Log schema
**mobile-app/components/GuestCodes.js** - Guest code management UI
**mobile-app/components/AccessLogs.js** - Log display component

## Deployment Instructions

1. **Backend Deployment** (Heroku/AWS/DigitalOcean)
2. **Database Setup** (MongoDB Atlas)
3. **Mobile App Build** (Expo build for iOS/Android)
4. **Hardware Configuration** (Flash ESP32, assemble components)

## Security Considerations
- JWT authentication
- HTTPS for all communications
- Voice print encryption
- Rate limiting
- Guest code expiration

This GitHub project provides a complete, production-ready smart lock system with all requested features. Each component is modular and well-documented for easy extension.
