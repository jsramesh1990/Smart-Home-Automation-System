# MQTT Topics Documentation

## Overview

SmartLock Pro uses MQTT for real-time communication between devices. All topics follow a hierarchical structure for easy organization and filtering.

## Topic Structure

```
/smartlock/
    [device_id]/
        status/
            lock
            door
            battery
            wifi
            uptime
            errors
        events/
            access
            auth
            tamper
            error
        commands/
            lock
            unlock
            status
            config
            reset
        sensors/
            temperature
            humidity
            motion
            light
            air_quality
    system/
        devices/
        ota/
        logs/
        alerts/
```

## Topic Definitions

### Status Topics

#### `/smartlock/[device_id]/status/lock`

**Purpose**: Current lock state

**QoS**: 1 (At least once)

**Retained**: Yes

**Payload Format**:
```json
{
    "state": "locked",
    "timestamp": 1704499200,
    "source": "keypad",
    "user": "john_doe"
}
```

**States**:
- `locked` - Door is locked
- `unlocked` - Door is unlocked
- `jammed` - Lock mechanism stuck
- `unknown` - State cannot be determined
- `transitioning` - Currently changing state

#### `/smartlock/[device_id]/status/door`

**Purpose**: Door open/closed status

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "state": "closed",
    "timestamp": 1704499200,
    "duration": 120.5
}
```

#### `/smartlock/[device_id]/status/battery`

**Purpose**: Battery level

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "level": 85,
    "voltage": 3.8,
    "charging": true,
    "timestamp": 1704499200
}
```

#### `/smartlock/[device_id]/status/wifi`

**Purpose**: Wi-Fi connection status

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "ssid": "HomeNetwork",
    "rssi": -45,
    "ip": "192.168.1.101",
    "connected": true,
    "timestamp": 1704499200
}
```

#### `/smartlock/[device_id]/status/uptime`

**Purpose**: Device uptime

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "uptime": 86400,
    "boot_count": 42,
    "last_boot": "2026-01-15T10:00:00Z",
    "timestamp": 1704499200
}
```

### Event Topics

#### `/smartlock/[device_id]/events/access`

**Purpose**: Access events

**QoS**: 2 (Exactly once)

**Retained**: No

**Payload Format**:
```json
{
    "user_id": "john_doe",
    "user_name": "John Doe",
    "method": "fingerprint",
    "granted": true,
    "timestamp": 1704499200,
    "duration": 2.5,
    "ip_address": "192.168.1.100",
    "device_info": {
        "type": "smartlock",
        "version": "v1.2.3"
    }
}
```

#### `/smartlock/[device_id]/events/auth`

**Purpose**: Authentication attempts

**QoS**: 2

**Retained**: No

**Payload Format**:
```json
{
    "user_id": "unknown",
    "method": "rfid",
    "success": false,
    "reason": "invalid_card",
    "attempts": 3,
    "timestamp": 1704499200,
    "ip_address": "192.168.1.100"
}
```

#### `/smartlock/[device_id]/events/tamper`

**Purpose**: Tamper detection events

**QoS**: 2

**Retained**: Yes (until cleared)

**Payload Format**:
```json
{
    "event": "detected",
    "type": "enclosure_open",
    "severity": "high",
    "timestamp": 1704499200,
    "action": "alarm_triggered"
}
```

### Command Topics

#### `/smartlock/[device_id]/commands/lock`

**Purpose**: Lock the door

**QoS**: 1

**Retained**: No

**Payload Format**:
```json
{
    "action": "lock",
    "requester": "admin@smartlock.com",
    "timestamp": 1704499200,
    "device_id": "DEV_001"
}
```

#### `/smartlock/[device_id]/commands/unlock`

**Purpose**: Unlock the door

**QoS**: 1

**Retained**: No

**Payload Format**:
```json
{
    "action": "unlock",
    "requester": "admin@smartlock.com",
    "auth_code": "123456",
    "timestamp": 1704499200,
    "device_id": "DEV_001"
}
```

#### `/smartlock/[device_id]/commands/status`

**Purpose**: Request device status

**QoS**: 1

**Retained**: No

**Payload Format**:
```json
{
    "action": "status",
    "requester": "dashboard",
    "timestamp": 1704499200,
    "fields": ["lock", "door", "battery", "wifi"]
}
```

**Response**: Device responds with all status topics

#### `/smartlock/[device_id]/commands/config`

**Purpose**: Update device configuration

**QoS**: 1

**Retained**: No

**Payload Format**:
```json
{
    "action": "config",
    "config": {
        "auto_lock_timeout": 30,
        "notifications_enabled": true,
        "sound_enabled": false,
        "led_brightness": 50
    },
    "timestamp": 1704499200
}
```

### Sensor Topics

#### `/smartlock/[device_id]/sensors/temperature`

**Purpose**: Temperature reading

**QoS**: 1

**Retained**: Yes (last value only)

**Payload Format**:
```json
{
    "value": 22.5,
    "unit": "celsius",
    "timestamp": 1704499200
}
```

#### `/smartlock/[device_id]/sensors/humidity`

**Purpose**: Humidity reading

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "value": 45.0,
    "unit": "percent",
    "timestamp": 1704499200
}
```

#### `/smartlock/[device_id]/sensors/motion`

**Purpose**: Motion detection

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "state": "detected",
    "timestamp": 1704499200,
    "duration": 5.0
}
```

## System Topics

#### `/smartlock/system/devices`

**Purpose**: Device discovery and registration

**QoS**: 1

**Retained**: Yes

**Payload Format**:
```json
{
    "devices": [
        {
            "id": "DEV_001",
            "type": "smartlock",
            "status": "online",
            "firmware": "v1.2.3",
            "location": "Front Door"
        }
    ],
    "timestamp": 1704499200
}
```

#### `/smartlock/system/ota`

**Purpose**: OTA update notifications

**QoS**: 2

**Retained**: Yes

**Payload Format**:
```json
{
    "device_id": "DEV_001",
    "current_version": "v1.2.3",
    "available_version": "v1.3.0",
    "status": "available",
    "timestamp": 1704499200,
    "release_notes": "Added new features..."
}
```

#### `/smartlock/system/logs`

**Purpose**: System log aggregation

**QoS**: 1

**Retained**: No

**Payload Format**:
```json
{
    "device_id": "DEV_001",
    "level": "error",
    "code": "ERR_001",
    "message": "Failed to connect to MQTT broker",
    "timestamp": 1704499200,
    "stack": "..."
}
```

#### `/smartlock/system/alerts`

**Purpose**: Critical alerts

**QoS**: 2

**Retained**: Yes

**Payload Format**:
```json
{
    "alert_id": "ALERT_001",
    "severity": "critical",
    "type": "security_breach",
    "device": "DEV_001",
    "message": "Multiple failed authentication attempts detected",
    "timestamp": 1704499200,
    "acknowledged": false
}
```

## Topic Patterns

### Subscriptions

```python
# Subscribe to all status updates for a specific device
subscribe("/smartlock/DEV_001/status/#")

# Subscribe to access events
subscribe("/smartlock/+/events/access")

# Subscribe to all alerts
subscribe("/smartlock/system/alerts")

# Subscribe to all topics (debugging only)
subscribe("/smartlock/#")
```

### Publishing Guidelines

1. **Retain Important Messages**: Status updates should be retained
2. **Use Proper QoS**: 
   - QoS 0: Non-critical sensor data
   - QoS 1: Status updates and commands
   - QoS 2: Critical events and logs
3. **Include Timestamps**: Always include ISO 8601 timestamps
4. **JSON Format**: Use consistent JSON structure
5. **Keep Payloads Small**: Minimize bandwidth usage

## Message Examples

### Lock Event Flow

```
Client (ESP32) publishes:
/smartlock/DEV_001/events/access
{
    "user_id": "john_doe",
    "method": "fingerprint",
    "granted": true,
    "timestamp": 1704499200
}

MQTT Broker stores and forwards to subscribers

Subscribers receive:
- Web Dashboard: Update UI
- Mobile App: Push notification
- Cloud Service: Log to database
- Automation System: Trigger rules
```

### Command Flow

```
Admin publishes:
/smartlock/DEV_001/commands/unlock
{
    "action": "unlock",
    "requester": "admin@smartlock.com",
    "timestamp": 1704499200
}

ESP32 receives and processes:
- Verify authorization
- Unlock door
- Publish confirmation:
  /smartlock/DEV_001/status/lock
  {
      "state": "unlocked",
      "timestamp": 1704499200,
      "source": "remote"
  }
```

## Wildcard Usage

- **Single-level wildcard (+)**: Matches one topic level
  - `/smartlock/+/status/lock` - Matches any device's lock status
  - `/smartlock/+/events/access` - Matches any device's access events

- **Multi-level wildcard (#)**: Matches multiple levels
  - `/smartlock/#` - Matches all SmartLock topics
  - `/smartlock/+/status/#` - Matches all status topics for any device

## Security Considerations

1. **Authentication**: All connections require username/password
2. **TLS Encryption**: Use TLS 1.2+ for secure communication
3. **ACL Rules**: Implement proper access control lists
4. **Topic Restrictions**: Restrict publishing to authorized clients
5. **Payload Validation**: Validate all incoming messages
6. **Rate Limiting**: Prevent DDoS attacks
7. **Audit Logging**: Log all MQTT operations
8. **Client ID Validation**: Prevent spoofing

## Topic Statistics

| Metric | Value |
|--------|-------|
| Total Topics | 24 |
| Status Topics | 6 |
| Event Topics | 5 |
| Command Topics | 5 |
| Sensor Topics | 6 |
| System Topics | 4 |
| Average Payload Size | 150 bytes |
| Peak Message Rate | 100 messages/sec |
| Retained Topics | 15 |

---
