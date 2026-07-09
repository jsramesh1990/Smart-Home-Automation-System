# Hardware Design

## Component Selection

### ESP32 DevKit - Main Controller

| Specification | Details |
|---------------|---------|
| **Model** | ESP32-WROOM-32D / ESP32-WROVER |
| **Processor** | Dual-core Xtensa LX6 @ 240MHz |
| **Memory** | 520KB SRAM + 4MB Flash (expandable) |
| **Wireless** | Wi-Fi 802.11 b/g/n + Bluetooth 4.2/BLE |
| **Security** | Crypto acceleration, Secure Boot, Flash Encryption |
| **Pins** | 25 GPIO (varies by board) |

**Pros:**
- Built-in Wi-Fi and Bluetooth
- Excellent community support
- Low cost ($10-$15)
- Low power consumption
- Hardware security features

**Cons:**
- Limited RAM (520KB)
- Requires external storage for large data

### R307 Fingerprint Sensor

| Specification | Details |
|---------------|---------|
| **Sensor Type** | Optical CMOS |
| **Resolution** | 500 DPI |
| **Capacity** | 1000 fingerprints |
| **Interface** | UART (TTL) |
| **Voltage** | 3.3V - 5V |
| **Current** | < 100mA |

**Features:**
- High accuracy (< 0.001% FAR)
- Fast recognition (< 0.5s)
- Built-in DSP for processing
- Stores templates securely
- Anti-spoofing detection

### RC522 RFID/NFC Module

| Specification | Details |
|---------------|---------|
| **Frequency** | 13.56 MHz |
| **Standard** | ISO 14443A/MIFARE |
| **Range** | 50-100mm |
| **Interface** | SPI |
| **Voltage** | 3.3V |
| **Current** | 13-26mA |

**Supported Tags:**
- MIFARE Classic
- MIFARE Ultralight
- NTAG series
- NFC tags

### 4x4 Matrix Keypad

| Specification | Details |
|---------------|---------|
| **Layout** | 4 rows x 4 columns |
| **Keys** | 16 (0-9, A-D, *, #) |
| **Interface** | GPIO (matrix scanning) |
| **Voltage** | 3.3V - 5V |
| **Current** | < 1mA |

**Key Features:**
- Membrane or mechanical
- Tactile feedback
- Backlight optional
- IP65 rated version

### MG995 Servo Motor (Lock Actuator)

| Specification | Details |
|---------------|---------|
| **Type** | Digital Servo |
| **Torque** | 8.5 kg/cm (at 4.8V) |
| **Speed** | 0.2s/60° |
| **Interface** | PWM |
| **Voltage** | 4.8V - 7.2V |
| **Current** | 300mA (idle), 500mA (peak) |

### Door Sensor (Magnetic Reed Switch)

| Specification | Details |
|---------------|---------|
| **Type** | Magnetic Reed Switch |
| **Output** | NC/NO |
| **Range** | 10-15mm |
| **Interface** | GPIO (digital input) |
| **Voltage** | 3.3V - 12V |
| **Current** | < 1mA |

## Schematic Design

### Main Schematic

```
                    ┌─────────────────────────────────────┐
                    │         ESP32 DEVKIT                │
                    │                                     │
    ┌───────────────┤  3.3V     VIN   5V  ├───────────────┤── 5V Power
    │               │                                     │     │
    │               │  GPIO0   GPIO1  ├───UART0 TX (Debug)
    │               │  GPIO2   GPIO3  ├───UART0 RX
    │               │  GPIO4   GPIO5  ├───RFID CS
    │               │  GPIO9   GPIO10 ├───UART1 TX (RPi)
    │               │  GPIO12  GPIO13 ├───Keypad Col 1
    │               │  GPIO14  GPIO15 ├───Keypad Col 2
    │               │  GPIO16  GPIO17 ├───Fingerprint UART
    │               │  GPIO18  GPIO19 ├───SPI MISO
    │               │  GPIO21  GPIO22 ├───I2C (OLED)
    │               │  GPIO23  GPIO25 ├───Keypad Row 3
    │               │  GPIO26  GPIO27 ├───RFID RST
    │               │  GPIO32  GPIO33 ├───Keypad Row 1
    │               │  GPIO34  GPIO35 ├───PIR Sensor
    │               │  GPIO36  GPIO39 ├───USB Detect
    │               │  EN      GND   ├───GND
    │               │  BTN     ADC   ├───Temp Sensor
    │               └─────────────────────────────────────┘
    │
    ├──────────────▶ R307 Fingerprint Sensor
    │
    ├──────────────▶ RC522 RFID Module
    │
    ├──────────────▶ 4x4 Matrix Keypad
    │
    ├──────────────▶ MG995 Servo
    │
    ├──────────────▶ OLED Display (Optional)
    │
    ├──────────────▶ Reed Switch (Door Sensor)
    │
    ├──────────────▶ PIR Motion Sensor
    │
    ├──────────────▶ DHT22 Temp/Humidity Sensor
    │
    └──────────────▶ Buzzer + LEDs
```

### Power Distribution

```
                    ┌─────────────────────────┐
                    │   5V 2A Power Supply    │
                    └────────────┬────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              ▼                  ▼                  ▼
      ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
      │  ESP32      │   │  Servo      │   │  Sensors    │
      │  (3.3V Reg) │   │  (5V)       │   │  (3.3V/5V)  │
      └─────────────┘   └─────────────┘   └─────────────┘
              │
              ▼
      ┌─────────────┐
      │  Peripherals│
      │  (3.3V)    │
      └─────────────┘

    ┌─────────────┬───────────────────────────────┐
    │  Component  │  Power Consumption            │
    ├─────────────┼───────────────────────────────┤
    │  ESP32      │  80-150mA (active)            │
    │  Fingerprint│  100mA (peak)                 │
    │  RFID       │  13-26mA                      │
    │  Servo      │  300-500mA (peak)             │
    │  Keypad     │  < 1mA                        │
    │  Sensors    │  < 5mA each                   │
    │  OLED       │  10-20mA                      │
    │  Buzzer     │  30-50mA (active)             │
    │  LEDs       │  5-20mA each                  │
    └─────────────┴───────────────────────────────┘

    Total: ~600-800mA peak, ~200mA average
```

### PCB Design Recommendations

```
PCB Specifications:
─────────────────
• Layers: 2-4 layer board
• Size: 80mm x 60mm (compact design)
• Material: FR4
• Copper: 1 oz
• Finish: HASL or ENIG
• Thickness: 1.6mm

Routing Guidelines:
─────────────────
• Power traces: 0.5mm minimum
• Signal traces: 0.25mm minimum
• Keep antennas clear of interference
• Ground plane for noise reduction
• Decoupling capacitors near ICs

Component Placement:
─────────────────
• ESP32 at center
• Antenna at edge, away from metals
• Power supply section at one corner
• I/O connectors at edges
• Keep high-current paths short
• Isolate sensitive analog components

Thermal Management:
─────────────────
• ESP32: 2.5W maximum
• Voltage regulator: Heat sink recommended
• Ventilation holes in enclosure
• Keep components spaced
```

### 3D Enclosure Design

```
External Dimensions:
─────────────────
• Width: 120mm
• Height: 150mm
• Depth: 35mm

Material:
─────────────────
• PLA or ABS filament
• PETG for outdoor use
• Consider IP65 rating

Components:
─────────────────
• Top cover
• Bottom base
• Mounting brackets
• Screw holes
• Cable pass-throughs
• LED light pipes
• Keypad cutout
• Fingerprint window
• RFID reader area

Design Features:
─────────────────
• Tamper-proof screws
• Anti-removal mechanism
• Weather-resistant seals
• Ventilation for heat dissipation
• Easy wall mounting
```

### Hardware Bill of Materials (BOM)

| Component | Quantity | Part Number | Cost (USD) | Source |
|-----------|----------|-------------|------------|--------|
| ESP32 DevKit | 1 | ESP32-WROOM-32D | $15 | Amazon/AliExpress |
| Fingerprint Sensor | 1 | R307 | $18 | AliExpress |
| RFID Module | 1 | RC522 | $5 | AliExpress |
| Matrix Keypad | 1 | 4x4 Membrane | $3 | AliExpress |
| Servo Motor | 1 | MG995 | $8 | Amazon |
| Door Sensor | 1 | Magnetic Reed | $2 | AliExpress |
| OLED Display | 1 | SSD1306 128x64 | $5 | AliExpress |
| PIR Sensor | 1 | HC-SR501 | $3 | AliExpress |
| DHT22 | 1 | DHT22 | $5 | Amazon |
| Buzzer | 1 | 5V Passive | $1 | AliExpress |
| LEDs (Red, Green, Blue) | 3 | 5mm | $2 | AliExpress |
| Resistors (220Ω, 10kΩ) | 10 | Various | $2 | AliExpress |
| Capacitors (100µF, 10µF) | 5 | Various | $3 | AliExpress |
| Jumper Wires | 40 | Various | $5 | Amazon |
| Breadboard | 1 | 830 points | $5 | Amazon |
| PCB (Custom) | 5 | 4 layer | $40 | JLCPCB |
| 3D Print (Case) | 1 set | PLA/ABS | $20 | Local |
| Power Supply | 1 | 5V 2A | $10 | Amazon |
| Connectors | 1 set | Various | $10 | AliExpress |
| **Total** | | | **$162** | |

## Hardware Testing Procedures

### 1. Power Supply Test
```python
def test_power_supply():
    """Test all voltage rails"""
    voltages = {
        '5V': measure_voltage('5V'),
        '3.3V': measure_voltage('3.3V'),
        'GND': measure_resistance('GND', 'GND')
    }
    
    for rail, value in voltages.items():
        assert value in acceptable_ranges[rail], f"{rail} voltage out of spec"
```

### 2. GPIO Connectivity Test
```python
def test_gpio_pins():
    """Test all GPIO pins"""
    for pin in gpio_pins:
        set_pin_output(pin, HIGH)
        assert read_pin(pin) == HIGH, f"Pin {pin} failed HIGH test"
        
        set_pin_output(pin, LOW)
        assert read_pin(pin) == LOW, f"Pin {pin} failed LOW test"
```

### 3. Communication Protocol Test
```python
def test_communication():
    """Test all communication interfaces"""
    # Test UART
    test_uart('UART0', baud=115200)
    test_uart('UART1', baud=57600)
    test_uart('UART2', baud=9600)
    
    # Test SPI
    test_spi(cs_pin=5, device='RFID')
    
    # Test I2C
    test_i2c(device='OLED')
```

### 4. End-to-End Functionality Test
```python
def test_full_system():
    """Test complete system functionality"""
    # Test each authentication method
    test_fingerprint_scan()
    test_rfid_read()
    test_keypad_entry()
    test_mobile_control()
    
    # Test lock actuation
    test_lock_unlock()
    
    # Test sensors
    test_door_sensor()
    test_motion_sensor()
    test_temperature_humidity()
    
    # Test communication
    test_wifi_connection()
    test_mqtt_publish()
```

---

