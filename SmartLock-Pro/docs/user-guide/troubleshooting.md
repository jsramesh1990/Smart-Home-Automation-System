# SmartLock Pro - Troubleshooting Guide

## Quick Reference

| Symptom | Most Likely Cause | Quick Fix |
|---------|-------------------|-----------|
| Device not powering on | Power supply issue | Check USB cable, power adapter |
| WiFi not connecting | Wrong credentials | Verify SSID and password |
| Fingerprint not working | Dirty sensor | Clean sensor surface |
| RFID not reading | Card not supported | Try different tag type |
| Servo not moving | Wiring issue | Check connections |
| Door sensor wrong state | Sensor misalignment | Adjust magnet position |
| BLE not connecting | Not advertising | Enable BLE in settings |
| MQTT disconnected | Broker down | Check broker status |
| OTA update fails | Insufficient space | Free up flash memory |
| Web dashboard not loading | Service stopped | Restart dashboard service |

## Hardware Troubleshooting

### Power Issues

**Problem**: ESP32 won't power on, no LED indicators

**Diagnosis**: 
```bash
# Test voltage with multimeter
VIN to GND: Should be 5V (±0.5V)
3.3V to GND: Should be 3.3V (±0.1V)
```

**Solutions**:
1. Check USB cable - try different cable
2. Check power adapter - 5V 2A minimum
3. Remove all peripherals and test bare board
4. Check for shorts between VIN and GND

### WiFi Issues

**Problem**: ESP32 cannot connect to WiFi network

**Diagnosis**:
```cpp
// Test WiFi module
void testWiFi() {
    Serial.println("Testing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect!");
        Serial.print("Status: ");
        Serial.println(WiFi.status());
    }
}
```

**Solutions**:
1. Verify SSID and password in config.h
2. Check signal strength - move closer to router
3. Check if MAC address is blocked
4. Try alternative WiFi channel (2.4GHz only)
5. Use WiFiManager for easier setup

### Fingerprint Sensor Issues

**Problem**: Fingerprint sensor not responding or inaccurate

**Diagnosis**:
```cpp
// Test fingerprint sensor
void testFingerprint() {
    Serial.println("Testing fingerprint...");
    
    if (!fingerprint.verifyPassword()) {
        Serial.println("No fingerprint sensor found!");
        Serial.println("Check wiring and power.");
        return;
    }
    
    // Check sensor parameters
    Serial.print("Fingerprint sensor: ");
    Serial.println(fingerprint.verifyPassword() ? "OK" : "FAIL");
    
    // Get template count
    uint8_t count = fingerprint.getTemplateCount();
    Serial.print("Templates stored: ");
    Serial.println(count);
    
    // Test reading a finger
    if (fingerprint.getImage() == FINGERPRINT_OK) {
        Serial.println("Finger detected!");
    } else {
        Serial.println("No finger detected!");
    }
}
```

**Solutions**:
1. Clean sensor surface with alcohol wipe
2. Check wiring: VCC (Red) to 5V, GND (Black), TX to RX, RX to TX
3. Verify baud rate (57600)
4. Re-enroll fingerprint with better positioning
5. Try different finger

### RFID Issues

**Problem**: RFID module not reading cards

**Diagnosis**:
```cpp
// Test RFID module
void testRFID() {
    Serial.println("Testing RFID...");
    mfrc522.PCD_Init();
    
    if (mfrc522.PCD_PerformSelfTest()) {
        Serial.println("RFID self-test passed.");
    } else {
        Serial.println("RFID self-test failed!");
        return;
    }
    
    if (mfrc522.PCD_IsAccessByte(0x00)) {
        Serial.println("RFID module ready.");
    } else {
        Serial.println("RFID module not ready!");
    }
    
    // Test reading
    if (mfrc522.PICC_IsNewCardPresent() && 
        mfrc522.PICC_ReadCardSerial()) {
        Serial.print("Card UID: ");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}
```

**Solutions**:
1. Check SPI connections: MOSI, MISO, SCK, CS
2. Verify CS pin is configured correctly
3. Try different card types (MIFARE Classic, Ultralight)
4. Check power supply (3.3V only)
5. Reduce SPI clock speed if unstable

### Servo Issues

**Problem**: Servo doesn't move or moves erratically

**Diagnosis**:
```cpp
// Test servo
void testServo() {
    Serial.println("Testing servo...");
    servo.attach(SERVO_PIN);
    
    for (int pos = 0; pos <= 180; pos += 10) {
        servo.write(pos);
        delay(100);
        Serial.print("Position: ");
        Serial.println(pos);
    }
    
    servo.detach();
}
```

**Solutions**:
1. Check power supply - servo needs sufficient current
2. Use separate power supply if ESP32 can't provide enough current
3. Check PWM pin and signal
4. Adjust servo min/max pulse widths in config
5. Avoid drawing too much current on 3.3V rail

## Software Troubleshooting

### Compilation Issues

**Problem**: Firmware fails to compile

**Solutions**:
1. **Missing Libraries**
   ```bash
   # Install all required libraries
   pio lib install "Adafruit Fingerprint Sensor Library"
   pio lib install "MFRC522"
   pio lib install "Keypad"
   pio lib install "PubSubClient"
   pio lib install "ArduinoJson"
   pio lib install "WiFiManager"
   ```

2. **Board Not Selected**
   - Arduino IDE: Tools → Board → ESP32 Dev Module
   - PlatformIO: Set `board = esp32dev` in platformio.ini

3. **Outdated Libraries**
   ```bash
   # Update all libraries
   pio lib update
   ```

4. **Insufficient Memory**
   - Reduce debug level
   - Disable unused features
   - Use smaller data types

### Serial Monitor Issues

**Problem**: No output on serial monitor

**Solutions**:
1. Check baud rate: 115200
2. Verify port selection
3. Check USB cable (data not just power)
4. Reinstall USB drivers:
   ```bash
   # Windows: Install CP210x drivers
   # Linux:
   sudo apt install python3-serial
   # Mac: Install SiLabs drivers
   ```

### MQTT Connection Issues

**Problem**: ESP32 cannot connect to MQTT broker

**Diagnosis**:
```cpp
// MQTT test
void testMQTT() {
    Serial.println("Testing MQTT connection...");
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    
    if (mqttClient.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("MQTT connected!");
    } else {
        Serial.print("MQTT failed! State: ");
        Serial.println(mqttClient.state());
    }
}
```

**Solutions**:
1. Verify broker IP and port
2. Check if broker is running
3. Verify username and password
4. Check firewall settings
5. Ensure network connectivity
6. If using TLS, verify certificates

### OTA Update Issues

**Problem**: OTA update fails

**Solutions**:
```cpp
// OTA error codes
#define OTA_SUCCESS 0
#define OTA_ERROR_SPACE 1
#define OTA_ERROR_CONNECT 2
#define OTA_ERROR_AUTH 3
#define OTA_ERROR_DATA 4
#define OTA_ERROR_WRITE 5

void handleOTAError(int code) {
    switch(code) {
        case OTA_ERROR_SPACE:
            Serial.println("Not enough flash space!");
            // Free up space
            break;
        case OTA_ERROR_CONNECT:
            Serial.println("Failed to connect to OTA server!");
            // Check URL and network
            break;
        case OTA_ERROR_AUTH:
            Serial.println("Authentication failed!");
            // Check credentials
            break;
        case OTA_ERROR_DATA:
            Serial.println("Data transmission error!");
            // Retry with smaller chunks
            break;
        case OTA_ERROR_WRITE:
            Serial.println("Flash write error!");
            // Check partition
            break;
    }
}
```

### Memory Issues

**Problem**: ESP32 crashes or resets randomly

**Diagnosis**:
```cpp
// Memory monitoring
void checkMemory() {
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    
    Serial.print("Free PSRAM: ");
    Serial.println(ESP.getFreePsram());
    
    Serial.print("Min free heap: ");
    Serial.println(ESP.getMinFreeHeap());
    
    Serial.print("Max alloc heap: ");
    Serial.println(ESP.getMaxAllocHeap());
}
```

**Solutions**:
1. Reduce buffer sizes
2. Free up static memory
3. Use PSRAM if available
4. Optimize string usage
5. Use smaller data structures
6. Implement memory pooling

## Network Troubleshooting

### IP Address Issues

**Problem**: Cannot find ESP32 on network

**Solutions**:
1. **Check DHCP Table**
   ```bash
   # Linux/Mac
   arp -a | grep -i espressif
   
   # Windows
   arp -a | findstr espressif
   ```

2. **Use mDNS**
   ```bash
   # Check mDNS name
   ping smartlock-001.local
   ```

3. **Use Serial Monitor**
   - ESP32 will print IP on connection
   - Look for "Connected to WiFi" message

4. **Fixed IP**
   - Set static IP in config.h

### Port Forwarding Issues

**Problem**: Cannot access ESP32 from outside local network

**Solutions**:
1. Verify port forwarding in router
2. Check firewall rules
3. Use VPN instead of port forwarding
4. Consider using ngrok or similar

## Mobile App Troubleshooting

### Connection Issues

**Problem**: Mobile app cannot connect to lock

**Solutions**:
1. **BLE Connection**
   - Ensure BLE is enabled on phone
   - Check lock is advertising
   - Try different BLE scanner app

2. **WiFi Connection**
   - Ensure phone on same network
   - Check IP address
   - Try ping test

3. **Cloud Connection**
   - Check internet connectivity
   - Verify cloud credentials
   - Check API status

### Authentication Issues

**Problem**: App shows "Authentication Failed"

**Solutions**:
1. Verify username/password
2. Check user permissions
3. Reset password via email
4. Contact admin for access

## Database Troubleshooting

### Connection Issues

**Problem**: Cannot connect to database

**Solutions**:
```bash
# Check database status
sudo systemctl status postgresql
sudo systemctl status mongod

# Check logs
tail -f /var/log/postgresql/postgresql-*.log
tail -f /var/log/mongodb/mongod.log

# Restart database
sudo systemctl restart postgresql
sudo systemctl restart mongod
```

### Performance Issues

**Problem**: Database queries are slow

**Solutions**:
1. Add indexes to frequently queried fields
2. Archive old logs
3. Use connection pooling
4. Optimize queries
5. Increase hardware resources

## Web Dashboard Troubleshooting

### Loading Issues

**Problem**: Dashboard not loading

**Solutions**:
```bash
# Check if service is running
sudo systemctl status smartlock-dashboard

# Check logs
journalctl -u smartlock-dashboard -f

# Restart service
sudo systemctl restart smartlock-dashboard
```

### Display Issues

**Problem**: Dashboard not showing data

**Solutions**:
1. Check MQTT connection
2. Verify database connectivity
3. Clear browser cache
4. Check JavaScript console for errors
5. Verify API endpoints

## Security Troubleshooting

### Unauthorized Access

**Problem**: Suspicious access attempts detected

**Actions**:
1. **Immediate Steps**
   - Lock all doors (remote)
   - Change admin password
   - Enable 2FA
   - Review access logs

2. **Investigation**
   ```bash
   # Check recent access logs
   curl -X GET http://localhost:3000/v1/logs \
        -H "Authorization: Bearer YOUR_TOKEN" \
        | jq '.data[] | select(.granted==false)'
   ```

3. **Prevention**
   - Enable rate limiting
   - Implement IP blacklisting
   - Set up alerting
   - Regular security audits

### Tamper Detection

**Problem**: Tamper alert triggered

**Actions**:
1. Check physical enclosure
2. Verify sensor readings
3. Review tamper logs
4. Reset tamper sensor
5. Update security measures

## System Recovery

### Firmware Recovery

**Problem**: ESP32 stuck in boot loop

**Solutions**:
1. **Safe Boot**
   ```bash
   # Hold BOOT button while resetting
   # Upload minimal firmware
   ```

2. **Factory Reset**
   ```bash
   # Erase flash completely
   esptool.py --port /dev/ttyUSB0 erase_flash
   
   # Upload fresh firmware
   esptool.py --port /dev/ttyUSB0 write_flash 0x1000 firmware.bin
   ```

3. **Use Backup Partition**
   - Boot from OTA backup partition
   - Recover from backup image

### Database Recovery

**Problem**: Database corrupted

**Solutions**:
```bash
# Restore from backup
pg_restore -d smartlock backup.dump

# Repair database
postgres --single -D /var/lib/postgresql/data

# Rebuild indexes
REINDEX DATABASE smartlock;
```

## Performance Optimization

### Slow Response Times

**Diagnosis**:
```cpp
// Performance monitoring
void measurePerformance() {
    unsigned long start = micros();
    
    // Operation to measure
    authenticateUser();
    
    unsigned long end = micros();
    Serial.print("Operation time: ");
    Serial.println(end - start);
}
```

**Solutions**:
1. Use hardware acceleration
2. Optimize algorithms
3. Reduce polling frequency
4. Use interrupts instead of polling
5. Upgrade to faster hardware

### Memory Leaks

**Detection**:
```cpp
// Memory leak detection
void checkMemoryLeak() {
    static unsigned long lastHeap = ESP.getFreeHeap();
    unsigned long currentHeap = ESP.getFreeHeap();
    
    if (currentHeap < lastHeap - 1000) {
        Serial.println("Potential memory leak detected!");
        Serial.print("Heap decreased by: ");
        Serial.println(lastHeap - currentHeap);
    }
    lastHeap = currentHeap;
}
```

**Solutions**:
1. Always free allocated memory
2. Use smart pointers
3. Avoid dynamic allocation in loops
4. Check for string concatenation leaks
5. Use fixed-size buffers

## Debugging Tools

### Serial Monitor Commands

```cpp
// Debug commands
void processDebugCommand(String cmd) {
    if (cmd == "status") {
        printStatus();
    } else if (cmd == "reset") {
        ESP.restart();
    } else if (cmd == "wifi") {
        printWiFiInfo();
    } else if (cmd == "memory") {
        printMemoryInfo();
    } else if (cmd == "logs") {
        printRecentLogs();
    } else if (cmd == "test") {
        runAllTests();
    }
}
```

### Web Debug Interface

Available at `http://[device_ip]/debug`

```
┌─────────────────────────────────────────────────────────────────┐
│                    DEBUG INTERFACE                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  System Status                                                 │
│  ├── CPU: 45%                                                 │
│  ├── Memory: 60%                                              │
│  ├── WiFi: Connected (RSSI: -45dBm)                           │
│  └── MQTT: Connected                                          │
│                                                                 │
│  Recent Errors                                                 │
│  1. 2026-01-15 10:30:15 - WiFi reconnect attempt #3           │
│  2. 2026-01-15 10:28:42 - Fingerprint timeout                 │
│  3. 2026-01-15 10:25:33 - MQTT reconnection                  │
│                                                                 │
│  Commands                                                      │
│  [Reboot] [Factory Reset] [Test Mode] [Update Firmware]       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Support Resources

### Official Documentation
- [SmartLock Pro Website](https://smartlock.com)
- [GitHub Repository](https://github.com/yourusername/SmartLock-Pro)
- [API Documentation](https://api.smartlock.com/docs)

### Community
- [Discord Server](https://discord.gg/smartlock)
- [Reddit r/SmartLock](https://reddit.com/r/SmartLock)
- [Forum](https://forum.smartlock.com)

### Getting Help
1. Check this troubleshooting guide
2. Search GitHub issues
3. Join community Discord
4. Email support: support@smartlock.com

---

