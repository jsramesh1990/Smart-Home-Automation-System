# MQTT (Message Queuing Telemetry Transport)

## Table of Contents

1. [Introduction](#introduction)
2. [What is MQTT?](#what-is-mqtt)
3. [Why Do We Need MQTT?](#why-do-we-need-mqtt)
4. [History of MQTT](#history-of-mqtt)
5. [MQTT Architecture](#mqtt-architecture)
6. [How MQTT Works](#how-mqtt-works)
7. [MQTT Components](#mqtt-components)
8. [Publish-Subscribe Model](#publish-subscribe-model)
9. [MQTT Topics](#mqtt-topics)
10. [MQTT Message Flow](#mqtt-message-flow)
11. [MQTT Quality of Service (QoS)](#mqtt-quality-of-service-qos)
12. [MQTT Packet Structure](#mqtt-packet-structure)
13. [MQTT Control Packets](#mqtt-control-packets)
14. [MQTT Broker](#mqtt-broker)
15. [MQTT Security](#mqtt-security)
16. [MQTT in Embedded Linux & Yocto](#mqtt-in-embedded-linux--yocto)
17. [MQTT in IoT Applications](#mqtt-in-iot-applications)
18. [MQTT in Automotive Systems](#mqtt-in-automotive-systems)
19. [Popular MQTT Brokers](#popular-mqtt-brokers)
20. [Advantages](#advantages)
21. [Disadvantages](#disadvantages)
22. [Best Practices](#best-practices)
23. [Interview Questions](#interview-questions)
24. [Conclusion](#conclusion)

---

# Introduction

Modern IoT and embedded systems require efficient communication between devices, cloud platforms, and applications.

Traditional protocols such as HTTP are often too heavy for:

* Low-power devices
* Battery-operated sensors
* Embedded systems
* Low-bandwidth networks

To solve this problem, MQTT was developed.

MQTT is one of the most widely used communication protocols in:

* IoT
* Smart Homes
* Industrial Automation
* Automotive Telematics
* Embedded Linux Systems
* Cloud Platforms

---

# What is MQTT?

MQTT stands for:

```text
Message Queuing Telemetry Transport
```

### Definition

> MQTT is a lightweight publish-subscribe messaging protocol designed for low-bandwidth, high-latency, and resource-constrained environments.

---

# Why Do We Need MQTT?

Without MQTT:

```text
Sensor
   ↓
HTTP Request
   ↓
Cloud Server

High Overhead
More Bandwidth
More Power Usage
```

With MQTT:

```text
Sensor
   ↓
MQTT Broker
   ↓
Subscribers

Low Bandwidth
Low Power
Fast Communication
```

---

## Interview Answer

### Why do we use MQTT?

MQTT is used to enable lightweight, reliable, and efficient communication between devices and applications. It minimizes bandwidth usage, reduces power consumption, and supports scalable publish-subscribe communication, making it ideal for IoT and embedded systems.

---

# History of MQTT

Developed in:

```text
1999
```

Created by:

* Andy Stanford-Clark
* Arlen Nipper

Original purpose:

```text
Oil Pipeline Monitoring
```

Today MQTT is a standard protocol for IoT systems.

---

# MQTT Architecture

```text
Publisher
     │
     ▼

 ┌──────────┐
 │  Broker  │
 └──────────┘

     ▲
     │

 Subscriber
```

The broker acts as a central communication hub.

---

# How MQTT Works

MQTT uses a **Publish-Subscribe Model**.

### Example

Temperature Sensor:

```text
Temperature Sensor
       ↓
Publish Message
       ↓
MQTT Broker
       ↓
Subscriber Receives Data
```

The publisher and subscriber never communicate directly.

---

# MQTT Components

## Publisher

Sends data.

Examples:

* Temperature Sensor
* Vehicle ECU
* Smart Meter

---

## Subscriber

Receives data.

Examples:

* Mobile App
* Cloud Service
* Dashboard

---

## Broker

Manages message routing.

Responsibilities:

* Receive messages
* Filter topics
* Forward messages
* Manage clients

---

## Topic

Logical channel used for communication.

Example:

```text
home/livingroom/temperature
```

---

# Publish-Subscribe Model

Traditional Client-Server:

```text
Client
   ↓
Server
```

MQTT:

```text
Publisher
      ↓
   Broker
      ↓
Subscriber
```

Advantages:

* Loose coupling
* Scalability
* Efficient communication

---

# MQTT Topics

Topics organize messages.

Example:

```text
factory/machine1/temperature
```

---

## Topic Hierarchy

```text
factory
   │
   ├── machine1
   │      └── temperature
   │
   └── machine2
          └── pressure
```

---

## Wildcards

### Single-Level Wildcard

```text
+
```

Example:

```text
factory/+/temperature
```

---

### Multi-Level Wildcard

```text
#
```

Example:

```text
factory/#
```

---

# MQTT Message Flow

```text
Publisher
    ↓
PUBLISH
    ↓
Broker
    ↓
Topic Matching
    ↓
Subscriber
```

---

# MQTT Quality of Service (QoS)

QoS defines message delivery reliability.

---

## QoS 0

### At Most Once

```text
Send Once
No Acknowledgment
```

Fastest.

Applications:

* Sensor Data
* Monitoring

---

## QoS 1

### At Least Once

```text
Send
 ↓
ACK
```

Message may be delivered multiple times.

---

## QoS 2

### Exactly Once

```text
Send
 ↓
Confirm
 ↓
Receive
```

Most reliable.

---

## QoS Comparison

| QoS | Reliability | Overhead |
| --- | ----------- | -------- |
| 0   | Low         | Low      |
| 1   | Medium      | Medium   |
| 2   | High        | High     |

---

# MQTT Packet Structure

Every MQTT packet contains:

```text
┌──────────────┐
│ Fixed Header │
├──────────────┤
│ Variable Hdr │
├──────────────┤
│ Payload      │
└──────────────┘
```

---

## Fixed Header

Contains:

* Packet Type
* Flags
* Remaining Length

---

## Variable Header

Depends on packet type.

Contains:

* Topic Name
* Packet Identifier

---

## Payload

Contains application data.

Example:

```json
{
  "temperature": 25
}
```

---

# MQTT Control Packets

MQTT defines several packet types.

| Packet      | Purpose                   |
| ----------- | ------------------------- |
| CONNECT     | Client Connection         |
| CONNACK     | Connection Acknowledgment |
| PUBLISH     | Send Message              |
| SUBSCRIBE   | Subscribe Topic           |
| SUBACK      | Subscribe Acknowledgment  |
| UNSUBSCRIBE | Remove Subscription       |
| PINGREQ     | Keep Alive Request        |
| PINGRESP    | Keep Alive Response       |
| DISCONNECT  | Client Disconnect         |

---

# MQTT Broker

The broker is the heart of MQTT communication.

Responsibilities:

* Authentication
* Authorization
* Topic Routing
* Message Delivery
* Session Management

---

# MQTT Security

## Username/Password Authentication

```text
Client
   ↓
Username + Password
```

---

## TLS/SSL Encryption

Secure communication.

Port:

```text
8883
```

---

## Access Control Lists (ACL)

Control topic access.

Example:

```text
Device1 → Publish Only
Device2 → Subscribe Only
```

---

# MQTT in Embedded Linux & Yocto

MQTT is widely used in Embedded Linux devices.

Common packages:

```bitbake
IMAGE_INSTALL += "mosquitto"
IMAGE_INSTALL += "mosquitto-clients"
```

---

## Publish Message

```bash
mosquitto_pub -h localhost \
-t sensor/temp \
-m "25"
```

---

## Subscribe Topic

```bash
mosquitto_sub -h localhost \
-t sensor/temp
```

---

# MQTT in IoT Applications

## Smart Home

Devices:

* Smart Lights
* Thermostats
* Security Systems

---

## Agriculture

Monitor:

* Soil Moisture
* Temperature
* Humidity

---

## Smart City

Monitor:

* Traffic
* Pollution
* Energy Usage

---

# MQTT in Automotive Systems

MQTT is increasingly used for:

---

## Vehicle Telematics

```text
Vehicle
   ↓
MQTT
   ↓
Cloud
```

Data:

* GPS
* Speed
* Fuel Level

---

## Fleet Management

Monitor:

* Vehicle Health
* Driver Behavior

---

## OTA Updates

Remote software updates.

---

## EV Monitoring

Battery data sent to cloud.

---

# Popular MQTT Brokers

## Mosquitto

Most popular open-source broker.

Eclipse Mosquitto

---

## EMQX

Enterprise MQTT broker.

EMQX

---

## HiveMQ

Commercial MQTT platform.

HiveMQ

---

## VerneMQ

Distributed MQTT broker.

VerneMQ

---

# Advantages

## Lightweight

Minimal bandwidth usage.

---

## Low Power Consumption

Ideal for battery devices.

---

## Scalable

Supports thousands of clients.

---

## Reliable Communication

QoS support.

---

## Decoupled Architecture

Publishers and subscribers are independent.

---

## Cloud Friendly

Works well with AWS, Azure, and Google Cloud.

---

# Disadvantages

## Broker Dependency

Broker failure impacts communication.

---

## Security Configuration Required

Needs TLS and authentication.

---

## Limited Built-In Data Validation

Application must validate payloads.

---

## Not Ideal for Large File Transfers

Designed for messaging, not bulk data.

---

# Best Practices

* Use TLS encryption.
* Enable authentication.
* Use meaningful topic structures.
* Choose appropriate QoS levels.
* Monitor broker performance.
* Implement Last Will and Testament (LWT).
* Avoid large payloads.

---

# Interview Questions

### What is MQTT?

MQTT is a lightweight publish-subscribe messaging protocol designed for IoT and embedded systems.

---

### Why Do We Use MQTT?

To enable efficient, low-bandwidth communication between devices and applications.

---

### What are MQTT Components?

* Publisher
* Subscriber
* Broker
* Topic

---

### What is a Topic?

A logical communication channel used to organize messages.

---

### What are QoS Levels?

* QoS 0: At Most Once
* QoS 1: At Least Once
* QoS 2: Exactly Once

---

### What is the Role of a Broker?

The broker receives messages from publishers and forwards them to subscribers.

---

### Which Port Does MQTT Use?

| Protocol      | Port |
| ------------- | ---- |
| MQTT          | 1883 |
| MQTT over TLS | 8883 |

---

### Why is MQTT Popular in IoT?

Because it is lightweight, reliable, scalable, and optimized for low-bandwidth environments.

---

# Most Asked Interview Question

### Explain MQTT Communication Flow.

MQTT uses a publish-subscribe architecture. Publishers send messages to specific topics on a broker. Subscribers register interest in those topics and receive matching messages from the broker. This decouples senders and receivers, making communication scalable and efficient for IoT and embedded systems.

---

# Conclusion

MQTT is one of the most important protocols in IoT, Embedded Linux, Automotive Telematics, and Industrial Automation. Its lightweight publish-subscribe architecture, low bandwidth usage, QoS support, and scalability make it ideal for resource-constrained devices and cloud-connected applications. Understanding MQTT is essential for engineers working with Yocto, Linux, networking, and modern connected systems.
