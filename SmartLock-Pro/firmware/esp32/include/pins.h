// pins.h - ESP32 GPIO Pin Definitions
#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// ============================================================
// POWER & GROUND
// ============================================================
#define PIN_VIN         5        // 5V Input
#define PIN_3V3         3.3f     // 3.3V Regulated
#define PIN_GND         0        // Common Ground

// ============================================================
// UART PINS
// ============================================================
#define PIN_UART0_TX    GPIO_NUM_1   // Serial Debug TX
#define PIN_UART0_RX    GPIO_NUM_3   // Serial Debug RX
#define PIN_UART1_TX    GPIO_NUM_10  // Raspberry Pi TX
#define PIN_UART1_RX    GPIO_NUM_9   // Raspberry Pi RX
#define PIN_UART2_TX    GPIO_NUM_17  // Fingerprint TX
#define PIN_UART2_RX    GPIO_NUM_16  // Fingerprint RX

// ============================================================
// SPI PINS
// ============================================================
#define PIN_SPI_MOSI    GPIO_NUM_23  // SPI MOSI
#define PIN_SPI_MISO    GPIO_NUM_19  // SPI MISO
#define PIN_SPI_SCK     GPIO_NUM_18  // SPI Clock

// ============================================================
// I2C PINS
// ============================================================
#define PIN_I2C_SDA     GPIO_NUM_21  // I2C Data
#define PIN_I2C_SCL     GPIO_NUM_22  // I2C Clock

// ============================================================
// AUTHENTICATION PERIPHERALS
// ============================================================
#define PIN_FINGERPRINT_RX  PIN_UART2_RX
#define PIN_FINGERPRINT_TX  PIN_UART2_TX
#define PIN_FINGERPRINT_RESET GPIO_NUM_5   // Optional reset

#define PIN_RFID_CS     GPIO_NUM_5   // SPI CS
#define PIN_RFID_RST    GPIO_NUM_27  // RFID Reset

// Keypad Matrix (4x4)
#define PIN_KEYPAD_R1   GPIO_NUM_32  // Row 1
#define PIN_KEYPAD_R2   GPIO_NUM_33  // Row 2
#define PIN_KEYPAD_R3   GPIO_NUM_25  // Row 3
#define PIN_KEYPAD_R4   GPIO_NUM_26  // Row 4
#define PIN_KEYPAD_C1   GPIO_NUM_13  // Col 1
#define PIN_KEYPAD_C2   GPIO_NUM_14  // Col 2
#define PIN_KEYPAD_C3   GPIO_NUM_15  // Col 3
#define PIN_KEYPAD_C4   GPIO_NUM_4   // Col 4

// ============================================================
// LOCK & ACTUATORS
// ============================================================
#define PIN_SERVO       GPIO_NUM_12  // Servo PWM Signal
#define PIN_RELAY       GPIO_NUM_2   // Relay Control (for solenoid)

// ============================================================
// SENSORS
// ============================================================
#define PIN_DOOR_SENSOR GPIO_NUM_34  // Reed Switch (Input)
#define PIN_PIR_SENSOR  GPIO_NUM_35  // Motion Detection (Input)
#define PIN_TEMP_SENSOR GPIO_NUM_36  // ADC (Temperature/Humidity)

// ============================================================
// INDICATORS
// ============================================================
#define PIN_LED_RED     GPIO_NUM_0   // Red LED (Error/Status)
#define PIN_LED_GREEN   GPIO_NUM_2   // Green LED (Access Granted)
#define PIN_LED_BLUE    GPIO_NUM_4   // Blue LED (Wi-Fi/Bluetooth)
#define PIN_BUZZER      GPIO_NUM_10  // Piezo Buzzer

// ============================================================
// TOUCH CAPACITIVE (for tamper detection)
// ============================================================
#define PIN_TOUCH_T0    TOUCH_PAD_NUM0   // GPIO4
#define PIN_TOUCH_T1    TOUCH_PAD_NUM1   // GPIO0
#define PIN_TOUCH_T2    TOUCH_PAD_NUM2   // GPIO2

// ============================================================
// CONFIGURATION
// ============================================================
#define PIN_BOOT_BTN    GPIO_NUM_0   // Boot/Config Button
#define PIN_USB_DETECT  GPIO_NUM_39  // USB Power Detection

// ============================================================
// HELPER MACROS
// ============================================================
#define IS_GPIO_VALID(pin) ((pin) >= 0 && (pin) <= 39)

#endif // PINS_H
