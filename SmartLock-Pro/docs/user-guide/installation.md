# SmartLock Pro - Installation Guide

## Prerequisites

### Required Components

**Hardware:**
- [ ] ESP32 DevKit V1 (or compatible)
- [ ] R307 Fingerprint Sensor
- [ ] RC522 RFID Module
- [ ] 4x4 Matrix Keypad
- [ ] MG995 Servo Motor
- [ ] Magnetic Reed Switch (Door Sensor)
- [ ] USB Cable (Data + Power)
- [ ] 5V 2A Power Supply
- [ ] Jumper Wires (Male-to-Female, Female-to-Female)
- [ ] Breadboard (for prototyping)
- [ ] Optional: OLED Display, PIR Sensor, DHT22

**Software:**
- [ ] Arduino IDE 2.0+ or PlatformIO
- [ ] Git
- [ ] Python 3.8+
- [ ] Node.js 16+ (for web dashboard)
- [ ] MQTT Broker (Mosquitto, EMQX, or AWS IoT Core)
- [ ] Mobile App (React Native/Flutter development environment)

### System Requirements
- Windows 10+, macOS 10.15+, or Ubuntu 20.04+
- 4GB+ RAM
- 2GB+ free disk space
- Stable internet connection

## Step 1: Hardware Assembly

### Breadboard Setup

```
┌─────────────────────────────────────────────────────┐
│                    BREADBOARD SETUP                 │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ESP32 DevKit          │   Sensors & Modules       │
│  ─────────────         │   ────────────────        │
│  Pin 5V ──► Power Rail │   Fingerprint ─► UART     │
│  Pin GND──► Ground Rail│   RFID ─────────► SPI     │
│  GPIO Pins─► Devices   │   Keypad ───────► GPIO    │
│                     │   Servo ────────► PWM    │
│                     │   Buzzer ────────► GPIO    │
│                     │   Door Sensor ──► GPIO    │
│                     │   LEDs ──────────► GPIO    │
│                     │   OLED ──────────► I2C    │
│                     └─────────────────────────────┘
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Wiring Instructions

1. **Connect ESP32 to Breadboard**
   ```
   ESP32 → Breadboard
   ├── VIN (5V) → Power Rail (Red)
   ├── GND → Ground Rail (Black)
   ├── 3.3V → 3.3V Rail (Orange)
   └── All GPIO pins → Component connections
   ```

2. **Connect Fingerprint Sensor (R307)**
   ```
   Fingerprint Sensor → ESP32
   ├── VCC (Red) → 5V (VIN)
   ├── GND (Black) → GND
   ├── TX (Green) → GPIO16 (RX2)
   └── RX (White) → GPIO17 (TX2)
   ```

3. **Connect RFID Module (RC522)**
   ```
   RFID Module → ESP32
   ├── VCC (3.3V) → 3.3V
   ├── GND → GND
   ├── RST → GPIO27
   ├── SDA (SS) → GPIO5
   ├── MOSI → GPIO23
   ├── MISO → GPIO19
   └── SCK → GPIO18
   ```

4. **Connect Keypad (4x4 Matrix)**
   ```
   Keypad → ESP32
   ├── R1 → GPIO32
   ├── R2 → GPIO33
   ├── R3 → GPIO25
   ├── R4 → GPIO26
   ├── C1 → GPIO13
   ├── C2 → GPIO14
   ├── C3 → GPIO15
   └── C4 → GPIO4
   ```

5. **Connect Servo Motor (MG995)**
   ```
   Servo → ESP32
   ├── Red → 5V (VIN)
   ├── Brown → GND
   └── Orange → GPIO12 (PWM)
   ```

6. **Connect Door Sensor**
   ```
   Door Sensor → ESP32
   ├── One end → 3.3V
   └── Other end → GPIO34 (with 10kΩ pull-up)
   ```

7. **Connect LEDs and Buzzer**
   ```
   LEDs → ESP32
   ├── Red LED → GPIO0 (with 220Ω resistor)
   ├── Green LED → GPIO2 (with 220Ω resistor)
   ├── Blue LED → GPIO4 (with 220Ω resistor)
   └── Buzzer → GPIO10
   ```

## Step 2: Software Installation

### Install Arduino IDE

1. **Download Arduino IDE**
   ```bash
   # Windows/Mac: Download from https://www.arduino.cc/en/software
   # Linux:
   sudo apt update
   sudo apt install arduino
   ```

2. **Install ESP32 Board Package**
   - Open Arduino IDE
   - Go to File → Preferences
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Board Manager
   - Search for "ESP32" and install "ESP32 by Espressif Systems"

3. **Install Required Libraries**
   - Go to Sketch → Include Library → Manage Libraries
   - Install these libraries:
     ```
     ✅ Adafruit Fingerprint Sensor Library
     ✅ MFRC522
     ✅ Keypad
     ✅ PubSubClient
     ✅ ArduinoJson
     ✅ WiFiManager
     ✅ Servo (built-in)
     ✅ Wire (built-in)
     ✅ SPI (built-in)
     ```

### Clone Repository

```bash
# Clone the project repository
git clone https://github.com/yourusername/SmartLock-Pro.git
cd SmartLock-Pro

# Navigate to ESP32 firmware
cd firmware/esp32
```

### PlatformIO Setup (Alternative)

```bash
# Install PlatformIO
# VSCode extension or CLI

# Navigate to firmware directory
cd SmartLock-Pro/firmware/esp32

# Install dependencies
pio lib install

# Build the firmware
pio run

# Upload to ESP32
pio run --target upload
```

## Step 3: Firmware Configuration

### Configure WiFi Settings

1. **Using WiFiManager (Recommended)**
   - First boot will create an Access Point
   - Connect to "SmartLock-AP" on your phone/computer
   - Open browser to 192.168.4.1
   - Enter your WiFi credentials
   - Save and reboot

2. **Manual Configuration**
   ```cpp
   // config.h
   #define WIFI_SSID "YourNetworkName"
   #define WIFI_PASSWORD "YourNetworkPassword"
   #define WIFI_HOSTNAME "smartlock-001"
   ```

### Configure MQTT Settings

```cpp
// config.h
#define MQTT_BROKER "192.168.1.100"  // Raspberry Pi or Cloud
#define MQTT_PORT 1883
#define MQTT_USERNAME "smartlock"
#define MQTT_PASSWORD "secure_password"
#define MQTT_TOPIC_PREFIX "/smartlock/DEV_001"
```

### Configure Device ID

```cpp
// config.h
#define DEVICE_ID "DEV_001"  // Unique identifier
#define DEVICE_NAME "Front Door"
#define DEVICE_LOCATION "Main Entrance"
```

## Step 4: Upload Firmware

### Using Arduino IDE

1. Open `SmartLock-Pro/firmware/esp32/SmartLock.ino`
2. Select board: Tools → Board → ESP32 Dev Module
3. Select port: Tools → Port → /dev/ttyUSB0 (or COMx)
4. Click Upload (→) button

### Using PlatformIO

```bash
# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor
```

### Using esptool (Command Line)

```bash
# Erase flash
esptool.py --port /dev/ttyUSB0 erase_flash

# Upload firmware
esptool.py --port /dev/ttyUSB0 --baud 115200 write_flash -fm dout \
    0x1000 firmware.bin
```

## Step 5: Post-Installation Setup

### Initial Configuration

1. **Connect to WiFi**
   - Power on ESP32
   - LED indicators show status:
     - Blue LED flashing → WiFi connecting
     - Blue LED solid → WiFi connected
     - Red LED solid → Error

2. **Access Web Interface**
   - Find device IP in serial monitor or router DHCP list
   - Open browser to `http://[device_ip]`
   - Default credentials:
     - Username: `admin`
     - Password: `smartlock123`

3. **Set System Time**
   - Configure NTP server
   - Set timezone
   - Verify current time

### Enroll First User

1. **Register Fingerprint**
   - Access web dashboard
   - Go to Users → Add User
   - Select "Fingerprint" as method
   - Place finger on sensor 3 times
   - Save user profile

2. **Register RFID Card**
   - Go to Users → Add User
   - Select "RFID" as method
   - Place card near reader
   - Card ID detected
   - Save user profile

3. **Set PIN Code**
   - Go to Users → Add User
   - Select "Keypad" as method
   - Enter 4-6 digit PIN
   - Confirm PIN
   - Save user profile

## Step 6: Mobile App Setup

### Install from Source

```bash
# Clone and navigate to mobile app
cd SmartLock-Pro/mobile-app/android

# Build APK
./gradlew assembleRelease

# Install on device
adb install app/build/outputs/apk/release/app-release.apk
```

### Connect to Lock

1. Open SmartLock Mobile App
2. Tap "Add New Device"
3. Select "SmartLock Pro"
4. Choose connection method:
   - **BLE**: Scan and connect directly
   - **WiFi**: Enter device IP
   - **Cloud**: Login to account
5. Set device name and location
6. Test connection

## Step 7: Web Dashboard Setup

### Backend Installation

```bash
cd SmartLock-Pro/web-dashboard/backend

# Install dependencies
npm install

# Configure environment
cp .env.example .env
nano .env  # Edit configuration

# Start the server
npm start
```

### Frontend Installation

```bash
cd SmartLock-Pro/web-dashboard/frontend

# Install dependencies
npm install

# Build for production
npm run build

# Start development server
npm run dev
```

### Docker Deployment (Recommended)

```bash
cd SmartLock-Pro/web-dashboard

# Build Docker image
docker build -t smartlock-dashboard .

# Run container
docker run -d -p 3000:3000 --name smartlock-dashboard smartlock-dashboard

# Using Docker Compose
docker-compose up -d
```

## Step 8: Testing

### Hardware Test

```cpp
// Test code for hardware verification
void testHardware() {
    Serial.println("=== Hardware Test ===");
    
    // Test Servo
    servo.attach(12);
    servo.write(0);
    delay(1000);
    servo.write(90);
    delay(1000);
    servo.write(180);
    delay(1000);
    Serial.println("Servo Test: Pass");
    
    // Test Fingerprint
    if (fingerprint.verifyPassword()) {
        Serial.println("Fingerprint: Found sensor");
    }
    
    // Test RFID
    if (mfrc522.PCD_PerformSelfTest()) {
        Serial.println("RFID: Module working");
    }
    
    // Test Door Sensor
    Serial.print("Door status: ");
    Serial.println(digitalRead(PIN_DOOR_SENSOR) ? "Open" : "Closed");
}
```

### Integration Test

```bash
# MQTT connectivity test
mosquitto_sub -t "/smartlock/+/status/#" -v

# REST API test
curl -X GET http://localhost:3000/v1/system/health

# WebSocket test
wscat -c ws://localhost:3000/ws
```

## Step 9: Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| ESP32 not detected | Check USB cable, drivers, try different port |
| Fingerprint not working | Check wiring, baud rate (57600) |
| RFID not reading | Check SPI pins, CS pin |
| WiFi not connecting | Verify credentials, signal strength |
| MQTT not connecting | Check broker IP, port, credentials |
| OTA fails | Check memory, partition size |

### Logs

```bash
# View serial logs
screen /dev/ttyUSB0 115200

# View system logs (Raspberry Pi)
journalctl -u smartlock.service -f

# View docker logs
docker logs smartlock-dashboard
```

## Step 10: Security Hardening

1. **Change Default Passwords**
2. **Enable Secure Boot**
3. **Enable Flash Encryption**
4. **Use TLS for MQTT**
5. **Implement Rate Limiting**
6. **Regular Firmware Updates**
7. **Enable HTTPS for Web Dashboard**
8. **Use Strong Encryption Keys**

---

