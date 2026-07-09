# Network Diagram

## System Network Architecture

```
                               INTERNET
                                  │
                                  ▼
                        ┌─────────────────┐
                        │   Cloud/Hosted  │
                        │   Services      │
                        │   - AWS IoT    │
                        │   - Firebase   │
                        │   - MongoDB    │
                        └────────┬────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              ▼                  ▼                  ▼
    ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
    │  Mobile App     │  │  Web Dashboard  │  │  3rd Party APIs │
    │  (iOS/Android)  │  │  (Admin/User)   │  │  - IFTTT       │
    └─────────────────┘  └─────────────────┘  │  - Telegram    │
              │                  │             │  - Slack       │
              └────────┬─────────┘             └─────────────────┘
                       │
                       ▼
    ┌─────────────────────────────────────────────────────────────┐
    │                    Local Network                          │
    │  (192.168.1.0/24)                                         │
    │                                                            │
    │  ┌────────────────────────────────────────────────────────┐│
    │  │               Home Router / Gateway                   ││
    │  │  (192.168.1.1)                                       ││
    │  │  - DHCP Server                                       ││
    │  │  - DNS Resolver                                      ││
    │  │  - Port Forwarding (Optional)                        ││
    │  └────────────────────────────────────────────────────────┘│
    │              │                         │                   │
    │              ▼                         ▼                   │
    │  ┌────────────────────┐  ┌─────────────────────────────┐ │
    │  │  Raspberry Pi 4    │  │  ESP32 Smart Lock           │ │
    │  │  (192.168.1.100)  │  │  (192.168.1.101)            │ │
    │  │                    │  │                             │ │
    │  │  - MQTT Broker    │  │  - Wi-Fi STA Mode           │ │
    │  │  - Database       │  │  - BLE Peripheral           │ │
    │  │  - Web Server     │  │  - OTA Server               │ │
    │  │  - Automation     │  │  - MQTT Client              │ │
    │  │  - API Gateway    │  │  - HTTP Server              │ │
    │  └────────────────────┘  └─────────────────────────────┘ │
    │              │                         │                   │
    │              └─────────────┬───────────┘                   │
    │                            │                               │
    └────────────────────────────┼──────────────────────────────┘
                                 │
                          ┌─────┴─────┐
                          │  BLE      │
                          │  Direct   │
                          └───────────┘
```

## Wireless Communication Protocol

```
┌─────────────────────────────────────────────────────────────────┐
│                    COMMUNICATION PROTOCOL STACK                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  APPLICATION LAYER                                             │
│  ├── HTTP/HTTPS (Web Dashboard, REST API)                     │
│  ├── MQTT (IoT Communication)                                 │
│  ├── WebSocket (Real-time Updates)                            │
│  └── Custom BLE Protocol (Mobile Control)                     │
│                                                                 │
│  SECURITY LAYER                                                │
│  ├── TLS 1.3 (Transport Layer Security)                       │
│  ├── AES-256 (Data Encryption)                                │
│  ├── JWT (Authentication Tokens)                              │
│  └── OAuth 2.0 (Authorization)                                │
│                                                                 │
│  TRANSPORT LAYER                                               │
│  ├── TCP/IP (Wi-Fi)                                           │
│  ├── UDP (Wi-Fi)                                              │
│  ├── GATT (BLE)                                               │
│  └── Serial (UART - Local Communication)                      │
│                                                                 │
│  PHYSICAL LAYER                                                │
│  ├── 802.11 b/g/n (2.4 GHz Wi-Fi)                            │
│  ├── Bluetooth 4.2 (2.4 GHz)                                 │
│  ├── SPI (RFID Module)                                        │
│  └── UART (Fingerprint Sensor)                                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## MQTT Topic Structure

```
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT TOPIC STRUCTURE                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Root Topic: /smartlock/                                       │
│                                                                 │
│  ├── /[device_id]/                                             │
│  │   ├── /status/                                              │
│  │   │   ├── /lock        (locked/unlocked)                   │
│  │   │   ├── /door        (open/closed)                       │
│  │   │   ├── /battery     (percentage)                        │
│  │   │   ├── /wifi        (signal strength)                   │
│  │   │   └── /uptime      (seconds)                           │
│  │   │                                                        │
│  │   ├── /events/                                             │
│  │   │   ├── /access      (granted/denied)                    │
│  │   │   ├── /auth        (method: fingerprint/rfid/keypad)  │
│  │   │   ├── /tamper      (alert/clear)                      │
│  │   │   └── /error       (error codes)                       │
│  │   │                                                        │
│  │   ├── /commands/                                           │
│  │   │   ├── /lock        (lock/unlock)                      │
│  │   │   ├── /status      (request status)                   │
│  │   │   └── /config      (update configuration)             │
│  │   │                                                        │
│  │   └── /sensors/                                            │
│  │       ├── /temperature  (value)                            │
│  │       ├── /humidity     (value)                            │
│  │       ├── /motion       (detected/clear)                   │
│  │       └── /light        (lux value)                        │
│  │                                                             │
│  └── /system/                                                 │
│      ├── /devices         (device list)                       │
│      ├── /ota             (update status)                     │
│      └── /logs            (system logs)                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

Example MQTT Messages:
─────────────────────
/smartlock/device_001/status/lock     : { "state": "locked", "timestamp": 1704499200 }
/smartlock/device_001/events/access   : { "user": "John", "method": "fingerprint", "granted": true }
/smartlock/device_001/commands/lock   : { "action": "unlock", "requester": "admin" }
```

## REST API Endpoints

```
┌─────────────────────────────────────────────────────────────────┐
│                    REST API ENDPOINTS                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Base URL: https://api.smartlock.com/v1/                       │
│                                                                 │
│  AUTHENTICATION                                                │
│  ├── POST    /auth/login                                       │
│  ├── POST    /auth/refresh                                     │
│  ├── POST    /auth/logout                                      │
│  └── POST    /auth/register                                    │
│                                                                 │
│  DEVICE MANAGEMENT                                             │
│  ├── GET     /devices                                          │
│  ├── GET     /devices/{device_id}                              │
│  ├── POST    /devices                                          │
│  ├── PUT     /devices/{device_id}                              │
│  └── DELETE  /devices/{device_id}                              │
│                                                                 │
│  LOCK CONTROL                                                  │
│  ├── POST    /devices/{device_id}/lock                         │
│  ├── POST    /devices/{device_id}/unlock                       │
│  ├── GET     /devices/{device_id}/status                       │
│  └── POST    /devices/{device_id}/ota                          │
│                                                                 │
│  USER MANAGEMENT                                               │
│  ├── GET     /users                                            │
│  ├── POST    /users                                            │
│  ├── GET     /users/{user_id}                                  │
│  ├── PUT     /users/{user_id}                                  │
│  └── DELETE  /users/{user_id}                                  │
│                                                                 │
│  ACCESS LOGS                                                   │
│  ├── GET     /logs                                             │
│  ├── GET     /logs/{device_id}                                 │
│  ├── GET     /logs/{device_id}/{date}                          │
│  └── GET     /logs/export                                      │
│                                                                 │
│  SYSTEM                                                        │
│  ├── GET     /system/health                                    │
│  ├── GET     /system/metrics                                   │
│  └── POST    /system/backup                                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Network Security Zones

```
┌─────────────────────────────────────────────────────────────────┐
│                    NETWORK SECURITY ZONES                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Zone 0: Untrusted Zone (Internet)                             │
│  ├── Public IP Address                                         │
│  ├── Firewall Rule: Block all inbound                         │
│  └── VPN Required for Remote Access                           │
│                                                                 │
│  Zone 1: DMZ Zone                                             │
│  ├── Cloud Services                                            │
│  ├── Web Server                                                │
│  ├── API Gateway                                               │
│  └── Firewall Rule: Limited Access                            │
│                                                                 │
│  Zone 2: Internal Network (Trusted)                           │
│  ├── Raspberry Pi                                              │
│  ├── MQTT Broker                                               │
│  ├── Database Server                                           │
│  └── Firewall Rule: Full Trust                                │
│                                                                 │
│  Zone 3: IoT Network (Segregated)                             │
│  ├── ESP32 Devices                                             │
│  ├── Sensors                                                   │
│  └── Actuators                                                 │
│    └── Firewall Rule: VLAN Isolation                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Bandwidth Requirements

| Traffic Type | Average | Peak | Frequency |
|--------------|---------|------|-----------|
| MQTT Status Updates | 100 bytes | 1KB | Every 10s |
| MQTT Events | 200 bytes | 2KB | On trigger |
| REST API Requests | 1KB | 10KB | On demand |
| OTA Updates | 5MB | 4MB | Monthly |
| Video Feed (Optional) | 500KB/s | 2MB/s | On demand |
| **Total Average** | **~1KB/s** | **~2MB/s** | - |

## Firewall Rules

```
┌─────────────────────────────────────────────────────────────────┐
│                    FIREWALL RULES                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  INPUT Chain (Inbound)                                        │
│  ├── Allow: Established connections                           │
│  ├── Allow: SSH from Internal Network                         │
│  ├── Allow: HTTP/HTTPS from Internal Network                  │
│  ├── Allow: MQTT from Internal Network                        │
│  └── Drop: All other inbound                                 │
│                                                                 │
│  OUTPUT Chain (Outbound)                                      │
│  ├── Allow: All from Internal Network                         │
│  └── Allow: HTTPS to Cloud Services                           │
│                                                                 │
│  FORWARD Chain (Between Networks)                             │
│  ├── Allow: VPN to Internal Network                           │
│  └── Drop: All other forwarding                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Failover and Redundancy

```
┌─────────────────────────────────────────────────────────────────┐
│                    FAILOVER ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Primary Path (Preferred)                                      │
│  ESP32 ──Wi-Fi──► Local Network ──► Internet ──► Cloud        │
│                                                                 │
│  Secondary Path (Wi-Fi Failed)                                 │
│  ESP32 ──BLE──► Mobile App ──► Cellular Data ──► Cloud        │
│                                                                 │
│  Fallback Path (Internet Failed)                              │
│  ESP32 ──Local BLE──► Mobile App                              │
│                                                                 │
│  Offline Mode (No Connectivity)                                │
│  ESP32 ──Local Authentication──► Store Logs for Later Sync   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

