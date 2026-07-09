# SmartLock Pro - Configuration Guide

## Configuration Overview

SmartLock Pro can be configured through multiple interfaces:
1. **Hardware Settings** (jumpers, dip switches)
2. **Firmware Configuration** (config.h file)
3. **Web Dashboard** (GUI configuration)
4. **Mobile App** (user-friendly settings)
5. **MQTT Commands** (remote configuration)
6. **API Calls** (programmatic configuration)

## Firmware Configuration

### Main Configuration File (`config.h`)

```cpp
// config.h - Main configuration file
#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// DEVICE IDENTIFICATION
// ============================================================
#define DEVICE_ID "DEV_001"                    // Unique device identifier
#define DEVICE_NAME "Front Door"               // Human-readable name
#define DEVICE_LOCATION "Main Entrance"        // Physical location
#define DEVICE_TYPE "smartlock"                // Device type

// ============================================================
// WIFI CONFIGURATION
// ============================================================
#define WIFI_SSID "HomeNetwork"                // WiFi network name
#define WIFI_PASSWORD "SecurePass123"         // WiFi password
#define WIFI_HOSTNAME "smartlock-001"          // mDNS name
#define WIFI_CONNECTION_TIMEOUT 30            // Connection timeout (seconds)
#define WIFI_RECONNECT_INTERVAL 10            // Reconnect interval (seconds)
#define WIFI_MAX_RETRIES 5                    // Maximum retry attempts

// ============================================================
// MQTT CONFIGURATION
// ============================================================
#define MQTT_BROKER "192.168.1.100"           // MQTT broker IP
#define MQTT_PORT 1883                        // MQTT port (8883 for TLS)
#define MQTT_USERNAME "smartlock"             // MQTT username
#define MQTT_PASSWORD "secure_mqtt_password"  // MQTT password
#define MQTT_TOPIC_PREFIX "/smartlock"        // Topic prefix
#define MQTT_KEEP_ALIVE 60                    // Keep alive (seconds)
#define MQTT_QOS 1                            // Default QoS level

// ============================================================
# AUTHENTICATION SETTINGS
// ============================================================
#define MAX_USERS 100                         // Maximum users supported
#define MAX_FINGERPRINT_TEMPLATES 100         // Max fingerprint templates
#define MAX_RFID_CARDS 100                    // Max RFID cards
#define MAX_PIN_LENGTH 6                      // Maximum PIN length
#define MIN_PIN_LENGTH 4                      // Minimum PIN length
#define FAILED_ATTEMPTS_LIMIT 5               // Lockout after N attempts
#define LOCKOUT_DURATION 300                 // Lockout duration (seconds)

// ============================================================
// LOCK SETTINGS
// ============================================================
#define SERVO_PIN 12                          // Servo control pin
#define SERVO_MIN_PULSE 500                   // Servo min pulse width (µs)
#define SERVO_MAX_PULSE 2400                  // Servo max pulse width (µs)
#define LOCK_ANGLE 0                          // Servo angle for locked
#define UNLOCK_ANGLE 90                       // Servo angle for unlocked
#define AUTO_LOCK_TIMEOUT 30                  // Auto-lock after N seconds
#define LOCK_RETRY_ATTEMPTS 3                 // Lock retry attempts

// ============================================================
# SENSOR SETTINGS
// ============================================================
#define DOOR_SENSOR_PIN 34                    // Door sensor input
#define TEMP_SENSOR_PIN 36                    // Temperature sensor
#define PIR_SENSOR_PIN 35                     // Motion sensor
#define DOOR_OPEN_THRESHOLD 1                 // Door open signal
#define TEMP_UPDATE_INTERVAL 60              // Temperature update (seconds)
#define MOTION_TIMEOUT 30                     // Motion clear after (seconds)

// ============================================================
# COMMUNICATION SETTINGS
// ============================================================
#define BLE_ENABLED true                      // Enable BLE
#define BLE_DEVICE_NAME "SmartLock-001"       // BLE advertising name
#define BLE_ADV_INTERVAL 100                 // Advertising interval (ms)
#define BLE_CONNECTION_TIMEOUT 30            // BLE timeout (seconds)
#define HTTP_SERVER_ENABLED true              // Enable HTTP web server
#define HTTP_PORT 80                          // HTTP server port
#define WEBSOCKET_ENABLED true                // Enable WebSocket

// ============================================================
# SECURITY SETTINGS
// ============================================================
#define SECURE_BOOT_ENABLED true              // Enable secure boot
#define FLASH_ENCRYPTION_ENABLED true         // Enable flash encryption
#define ENCRYPTION_KEY "1234567890abcdef"    // AES encryption key
#define OTP_SECRET "otp_secret_key_12345"    // OTP secret key
#define USE_TLS true                          // Use TLS for MQTT

// ============================================================
# POWER MANAGEMENT
// ============================================================
#define DEEP_SLEEP_ENABLED true               // Enable deep sleep
#define DEEP_SLEEP_TIMEOUT 300               // Sleep timeout (seconds)
#define WAKEUP_PIN DOOR_SENSOR_PIN            // Wake on door sensor
#define POWER_SAVE_MODE true                  // Enable power saving
#define CPU_FREQ_ACTIVE 240000000L            // CPU frequency active
#define CPU_FREQ_SLEEP 80000000L              // CPU frequency sleep

// ============================================================
# OTA SETTINGS
// ============================================================
#define OTA_ENABLED true                      // Enable OTA updates
#define OTA_URL "http://192.168.1.100:8000"  // OTA server URL
#define OTA_CHECK_INTERVAL 3600              // Check interval (seconds)
#define OTA_BACKUP_ENABLED true               // Keep backup firmware

// ============================================================
# LOGGING SETTINGS
// ============================================================
#define LOG_LEVEL LOG_LEVEL_INFO              // Log level
#define LOG_TO_SERIAL true                    // Log to serial
#define LOG_TO_MQTT true                      // Log to MQTT
#define LOG_TO_SPIFFS true                    // Log to SPIFFS
#define LOG_BUFFER_SIZE 4096                 // Log buffer size

// ============================================================
# ADVANCED SETTINGS
// ============================================================
#define NTP_SERVER "pool.ntp.org"             // NTP time server
#define TIMEZONE "UTC+5:30"                   // System timezone
#define MAX_LOG_ENTRIES 10000                 // Maximum log entries
#define SPIFFS_MOUNT_POINT "/spiffs"          // SPIFFS mount point
#define TASK_STACK_SIZE 8192                 // Task stack size

#endif // CONFIG_H
```

## Web Dashboard Configuration

### System Settings

```
┌─────────────────────────────────────────────────────────────────┐
│                    SYSTEM CONFIGURATION                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  GENERAL SETTINGS                                              │
│  ├── Device Name: [SmartLock Pro]                             │
│  ├── Device Location: [Front Door]                            │
│  ├── Time Zone: [(UTC+5:30) Asia/Kolkata]                     │
│  └── Language: [English]                                      │
│                                                                 │
│  NETWORK SETTINGS                                              │
│  ├── Wi-Fi SSID: [HomeNetwork]                                │
│  ├── Wi-Fi Password: [••••••••]                              │
│  ├── IP Configuration: [DHCP / Static]                        │
│  ├── Static IP: [192.168.1.101]                               │
│  └── DNS Server: [8.8.8.8]                                    │
│                                                                 │
│  SECURITY SETTINGS                                             │
│  ├── Admin Password: [••••••••]                              │
│  ├── Failed Attempts Limit: [5]                               │
│  ├── Lockout Duration: [300] seconds                          │
│  ├── Session Timeout: [3600] seconds                          │
│  └── Two-Factor Auth: [Disabled / Enabled]                    │
│                                                                 │
│  MAINTENANCE                                                   │
│  ├── Firmware Version: [v1.2.3]                               │
│  ├── Check for Updates: [Check Now]                           │
│  ├── Backup Configuration: [Download]                         │
│  └── Factory Reset: [Reset]                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Access Control Settings

```
┌─────────────────────────────────────────────────────────────────┐
│                    ACCESS CONTROL SETTINGS                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  AUTHENTICATION METHODS                                        │
│  ├── [✓] Fingerprint                                          │
│  ├── [✓] RFID/NFC Cards                                       │
│  ├── [✓] Keypad PIN                                           │
│  ├── [✓] Mobile App (BLE)                                     │
│  ├── [✓] Remote Access (Wi-Fi)                                │
│  └── [ ] OTP Authentication                                   │
│                                                                 │
│  USER POLICIES                                                 │
│  ├── Max Users: [100]                                         │
│  ├── Default Access Method: [Fingerprint]                     │
│  ├── Auto-lock Timeout: [30] seconds                          │
│  ├── Require Admin Approval: [Yes/No]                         │
│  └── Dual Authentication: [Disabled]                          │
│                                                                 │
│  SCHEDULES                                                     │
│  ├── Working Hours: [09:00 - 18:00]                           │
│  ├── Weekend Access: [Enabled]                                │
│  ├── Holiday Schedule: [View/Edit]                            │
│  └── Time-Based Rules: [Add Rule]                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Notification Settings

```
┌─────────────────────────────────────────────────────────────────┐
│                    NOTIFICATION SETTINGS                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  EVENTS TO NOTIFY                                              │
│  ├── [✓] Successful Access                                    │
│  ├── [✓] Failed Access Attempts                               │
│  ├── [✓] Door Status Changes                                  │
│  ├── [✓] Tamper Detection                                     │
│  ├── [ ] Scheduled Events                                     │
│  └── [ ] System Alerts                                        │
│                                                                 │
│  NOTIFICATION CHANNELS                                         │
│  ├── [✓] Push Notification                                    │
│  ├── [✓] Email                                                │
│  ├── [ ] SMS                                                  │
│  ├── [ ] Telegram                                             │
│  └── [ ] Webhook                                              │
│                                                                 │
│  RECIPIENTS                                                    │
│  ├── Admin: [admin@example.com]                               │
│  ├── User: [user@example.com]                                 │
│  └── Phone: [+91-9876543210]                                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Mobile App Configuration

### App Settings

```json
{
  "app_settings": {
    "theme": "dark",
    "language": "en",
    "notifications": true,
    "auto_sync": true,
    "biometric_auth": true,
    "default_action": "unlock",
    "quick_actions": ["lock", "unlock", "status"]
  },
  "device_config": {
    "auto_connect": true,
    "ble_scan_timeout": 10,
    "wifi_retry_count": 3,
    "offline_mode": true,
    "cache_size": 100
  },
  "security": {
    "pin_enabled": true,
    "pin_code": "1234",
    "fingerprint_enabled": true,
    "face_id_enabled": false,
    "timeout": 60
  }
}
```

## MQTT Configuration

### Broker Configuration (`mosquitto.conf`)

```conf
# Mosquitto MQTT Broker Configuration

# Network
listener 1883 0.0.0.0
listener 8883 0.0.0.0

# TLS (for secure connections)
cafile /etc/ssl/certs/ca-certificates.crt
certfile /etc/ssl/certs/server.crt
keyfile /etc/ssl/private/server.key

# Authentication
allow_anonymous false
password_file /etc/mosquitto/passwd
acl_file /etc/mosquitto/acl

# Persistence
persistence true
persistence_location /var/lib/mosquitto/

# Logging
log_dest file /var/log/mosquitto/mosquitto.log
log_type error
log_type warning
log_type notice
log_type information

# Performance
max_connections -1
max_packet_size 1000000
message_size_limit 1000000
```

### ACL Rules (`acl`)

```
# Access Control List Rules

# Admin - Full access
user admin
topic readwrite /smartlock/#

# Devices - Read commands, write status
user smartlock
topic read /smartlock/+/commands/#
topic write /smartlock/+/status/#
topic write /smartlock/+/events/#
topic write /smartlock/+/sensors/#

# Mobile App - Read status, write commands
user mobile
topic read /smartlock/+/status/#
topic read /smartlock/+/events/access
topic write /smartlock/+/commands/#

# Dashboard - Read everything, write system
user dashboard
topic read /smartlock/#
topic write /smartlock/system/#
topic write /smartlock/+/commands/config

# Guests - Read only status
user guest
topic read /smartlock/+/status/lock
topic read /smartlock/+/status/door
```

## Docker Configuration

### Docker Compose (`docker-compose.yml`)

```yaml
version: '3.8'

services:
  mosquitto:
    image: eclipse-mosquitto:latest
    container_name: smartlock-mqtt
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "8883:8883"
      - "9001:9001"
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log
    environment:
      - TZ=Asia/Kolkata
    networks:
      - smartlock-network

  influxdb:
    image: influxdb:2.0
    container_name: smartlock-influxdb
    restart: unless-stopped
    ports:
      - "8086:8086"
    volumes:
      - influxdb-data:/var/lib/influxdb2
    environment:
      - INFLUXDB_DB=smartlock
      - INFLUXDB_USER=admin
      - INFLUXDB_PASSWORD=SecurePass123
    networks:
      - smartlock-network

  grafana:
    image: grafana/grafana:latest
    container_name: smartlock-grafana
    restart: unless-stopped
    ports:
      - "3000:3000"
    volumes:
      - grafana-data:/var/lib/grafana
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    depends_on:
      - influxdb
    networks:
      - smartlock-network

  web-dashboard:
    build:
      context: ./web-dashboard
      dockerfile: Dockerfile
    container_name: smartlock-dashboard
    restart: unless-stopped
    ports:
      - "8080:8080"
    environment:
      - MQTT_BROKER=mosquitto:1883
      - INFLUXDB_URL=http://influxdb:8086
      - NODE_ENV=production
    depends_on:
      - mosquitto
      - influxdb
    networks:
      - smartlock-network

volumes:
  influxdb-data:
  grafana-data:

networks:
  smartlock-network:
    driver: bridge
```

## Environment Variables

### Backend Environment (`.env`)

```env
# Server Configuration
NODE_ENV=production
PORT=3000
HOST=0.0.0.0

# Database
DB_HOST=localhost
DB_PORT=5432
DB_NAME=smartlock
DB_USER=admin
DB_PASSWORD=SecurePass123

# MQTT
MQTT_BROKER=localhost
MQTT_PORT=1883
MQTT_USERNAME=admin
MQTT_PASSWORD=SecurePass123

# Redis
REDIS_HOST=localhost
REDIS_PORT=6379
REDIS_PASSWORD=

# JWT
JWT_SECRET=your-super-secret-jwt-key
JWT_EXPIRE=7d

# Email
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_USER=your-email@gmail.com
SMTP_PASS=your-app-password

# Firebase
FIREBASE_API_KEY=your-firebase-api-key
FIREBASE_PROJECT_ID=your-project-id

# Security
ENCRYPTION_KEY=32-byte-encryption-key-here
OTP_ISSUER=SmartLockPro
OTP_PERIOD=30
```

## Advanced Configuration

### Custom Authentication Rules

```python
# custom_auth.py - Advanced authentication rules

class AuthenticationRules:
    def __init__(self):
        self.rules = {
            'time_based': {
                'enabled': True,
                'working_hours': ['09:00', '18:00'],
                'weekend_access': False
            },
            'multi_factor': {
                'enabled': True,
                'required_methods': 2,
                'allowed_combinations': [
                    ['fingerprint', 'rfid'],
                    ['fingerprint', 'pin']
                ]
            },
            'user_roles': {
                'admin': {
                    'access_all': True,
                    'max_attempts': 10
                },
                'user': {
                    'access_hours': ['07:00', '22:00'],
                    'max_attempts': 5
                },
                'guest': {
                    'schedule': {
                        'start': '2026-01-15 09:00',
                        'end': '2026-01-15 18:00'
                    },
                    'max_attempts': 3
                }
            }
        }
    
    def check_access(self, user, timestamp):
        """Check if user has access at given time"""
        # Implementation
        pass
```

### Performance Tuning

```cpp
// performance_config.h - Performance tuning

// Memory Management
#define HEAP_POISONING 1
#define HEAP_TRACE 0
#define ASSERTIONS 1

// Task Priorities
#define WIFI_TASK_PRIORITY 20
#define MQTT_TASK_PRIORITY 18
#define SENSOR_TASK_PRIORITY 15
#define UI_TASK_PRIORITY 10

// Interrupt Priorities
#define DOOR_SENSOR_INT_PRIORITY 1
#define TAMPER_INT_PRIORITY 2

// Buffer Sizes
#define NETWORK_BUFFER_SIZE 2048
#define SERIAL_BUFFER_SIZE 1024
#define MQTT_BUFFER_SIZE 2048

// Caching
#define CACHE_ENABLED true
#define CACHE_SIZE 100
#define CACHE_TTL 60  // seconds

// Rate Limiting
#define API_RATE_LIMIT 100  // requests per minute
#define MQTT_RATE_LIMIT 50  // messages per second
```

## Backup and Restore

### Backup Configuration

```bash
#!/bin/bash
# backup.sh - Configuration backup script

# Backup firmware configuration
backup_firmware() {
    echo "Backing up firmware configuration..."
    curl -X GET http://192.168.1.101/config -o backup/config_$(date +%Y%m%d).json
}

# Backup user database
backup_users() {
    echo "Backing up user database..."
    curl -X GET http://192.168.1.101/users -o backup/users_$(date +%Y%m%d).json
}

# Backup logs
backup_logs() {
    echo "Backing up logs..."
    curl -X GET http://192.168.1.101/logs/export -o backup/logs_$(date +%Y%m%d).csv
}

# Backup MQTT data
backup_mqtt() {
    echo "Backing up MQTT data..."
    docker exec smartlock-mqtt mosquitto_sub -t "/smartlock/+/status/#" -v > backup/mqtt_$(date +%Y%m%d).log
}

# Create full backup
full_backup() {
    backup_firmware
    backup_users
    backup_logs
    backup_mqtt
    echo "Backup completed: $(date)"
}

full_backup
```

---

