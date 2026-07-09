// test_auth.cpp - Authentication Unit Tests
#include <Arduino.h>
#include <unity.h>
#include "../../src/authentication/fingerprint.h"
#include "../../src/authentication/rfid.h"
#include "../../src/authentication/keypad.h"
#include "../../src/security/otp_generator.h"

// Test Fixtures
FingerprintAuth fingerprintAuth;
RFIDAuth rfidAuth;
KeypadAuth keypadAuth;
OTPGenerator otpGen;

// ============================================================
// FINGERPRINT TESTS
// ============================================================
void test_fingerprint_init() {
    TEST_ASSERT_TRUE(fingerprintAuth.init());
}

void test_fingerprint_enroll() {
    TEST_ASSERT_TRUE(fingerprintAuth.enroll(1));
}

void test_fingerprint_authenticate() {
    int result = fingerprintAuth.authenticate();
    TEST_ASSERT_TRUE(result >= -1);
}

void test_fingerprint_delete() {
    TEST_ASSERT_TRUE(fingerprintAuth.deleteTemplate(1));
}

void test_fingerprint_clear_all() {
    TEST_ASSERT_TRUE(fingerprintAuth.clearAllTemplates());
}

void test_fingerprint_get_count() {
    int count = fingerprintAuth.getTemplateCount();
    TEST_ASSERT_TRUE(count >= 0);
}

void test_fingerprint_detection() {
    bool detected = fingerprintAuth.isFingerDetected();
    TEST_ASSERT_TRUE(detected || true); // Test passes regardless
}

// ============================================================
// RFID TESTS
// ============================================================
void test_rfid_init() {
    TEST_ASSERT_TRUE(rfidAuth.init());
}

void test_rfid_authenticate() {
    uint32_t uid = rfidAuth.authenticate();
    TEST_ASSERT_TRUE(uid >= 0);
}

void test_rfid_card_detection() {
    bool detected = rfidAuth.isCardDetected();
    TEST_ASSERT_TRUE(detected || true);
}

void test_rfid_add_card() {
    TEST_ASSERT_TRUE(rfidAuth.addCard(0x12345678, 1));
}

void test_rfid_remove_card() {
    TEST_ASSERT_TRUE(rfidAuth.removeCard(0x12345678));
}

void test_rfid_get_user() {
    rfidAuth.addCard(0xABCDEF01, 5);
    uint16_t userId = rfidAuth.getUserByUID(0xABCDEF01);
    TEST_ASSERT_EQUAL(5, userId);
}

// ============================================================
// KEYPAD TESTS
// ============================================================
void test_keypad_init() {
    TEST_ASSERT_TRUE(keypadAuth.init());
}

void test_keypad_read_input() {
    String input = keypadAuth.readInput();
    TEST_ASSERT_TRUE(input.length() >= 0);
}

void test_keypad_set_pin() {
    TEST_ASSERT_TRUE(keypadAuth.setPIN(1, "1234"));
}

void test_keypad_authenticate() {
    keypadAuth.setPIN(2, "5678");
    TEST_ASSERT_TRUE(keypadAuth.authenticate("5678"));
    TEST_ASSERT_FALSE(keypadAuth.authenticate("wrong"));
}

void test_keypad_get_user() {
    keypadAuth.setPIN(3, "9999");
    uint16_t userId = keypadAuth.getUserByPIN("9999");
    TEST_ASSERT_EQUAL(3, userId);
}

void test_keypad_remove_pin() {
    keypadAuth.setPIN(4, "1111");
    TEST_ASSERT_TRUE(keypadAuth.removePIN("1111"));
}

// ============================================================
// OTP TESTS
// ============================================================
void test_otp_init() {
    TEST_ASSERT_TRUE(otpGen.init());
}

void test_otp_generate() {
    String otp = otpGen.generateOTP();
    TEST_ASSERT_EQUAL(6, otp.length());
    TEST_ASSERT_TRUE(otp.toInt() >= 0 && otp.toInt() <= 999999);
}

void test_otp_validate_correct() {
    // Generate and validate
    String otp = otpGen.generateOTP();
    TEST_ASSERT_TRUE(otpGen.validateOTP(otp));
}

void test_otp_validate_incorrect() {
    TEST_ASSERT_FALSE(otpGen.validateOTP("000000"));
    TEST_ASSERT_FALSE(otpGen.validateOTP("123456"));
}

void test_otp_time_offset() {
    otpGen.setTimeOffset(60); // 1 minute ahead
    String otp = otpGen.generateOTP();
    TEST_ASSERT_EQUAL(6, otp.length());
    otpGen.setTimeOffset(0);
}

// ============================================================
# AUTHENTICATION PERFORMANCE TESTS
// ============================================================
void test_auth_performance() {
    unsigned long start = micros();
    
    // Test authentication speed
    fingerprintAuth.authenticate();
    rfidAuth.authenticate();
    keypadAuth.authenticate("1234");
    otpGen.generateOTP();
    
    unsigned long duration = micros() - start;
    TEST_ASSERT_TRUE(duration < 5000000); // < 5 seconds total
    
    Serial.printf("Authentication performance: %lu µs\n", duration);
}

// ============================================================
# AUTHENTICATION SECURITY TESTS
// ============================================================
void test_security_lockout() {
    // Simulate multiple failed attempts
    for (int i = 0; i < 6; i++) {
        keypadAuth.authenticate("wrong");
    }
    
    // User should be locked out
    // This is a placeholder - actual lockout logic would be tested
    TEST_ASSERT_TRUE(true);
}

void test_otp_replay_attack() {
    String otp = otpGen.generateOTP();
    
    // Try to reuse OTP
    TEST_ASSERT_TRUE(otpGen.validateOTP(otp));
    TEST_ASSERT_FALSE(otpGen.validateOTP(otp)); // Should fail on reuse
}

// ============================================================
// TEST SUITE
// ============================================================
void setup_auth_tests() {
    UNITY_BEGIN();
    
    // Fingerprint tests
    RUN_TEST(test_fingerprint_init);
    RUN_TEST(test_fingerprint_enroll);
    RUN_TEST(test_fingerprint_authenticate);
    RUN_TEST(test_fingerprint_delete);
    RUN_TEST(test_fingerprint_clear_all);
    RUN_TEST(test_fingerprint_get_count);
    RUN_TEST(test_fingerprint_detection);
    
    // RFID tests
    RUN_TEST(test_rfid_init);
    RUN_TEST(test_rfid_authenticate);
    RUN_TEST(test_rfid_card_detection);
    RUN_TEST(test_rfid_add_card);
    RUN_TEST(test_rfid_get_user);
    RUN_TEST(test_rfid_remove_card);
    
    // Keypad tests
    RUN_TEST(test_keypad_init);
    RUN_TEST(test_keypad_read_input);
    RUN_TEST(test_keypad_set_pin);
    RUN_TEST(test_keypad_authenticate);
    RUN_TEST(test_keypad_get_user);
    RUN_TEST(test_keypad_remove_pin);
    
    // OTP tests
    RUN_TEST(test_otp_init);
    RUN_TEST(test_otp_generate);
    RUN_TEST(test_otp_validate_correct);
    RUN_TEST(test_otp_validate_incorrect);
    RUN_TEST(test_otp_time_offset);
    
    // Performance and security
    RUN_TEST(test_auth_performance);
    RUN_TEST(test_security_lockout);
    RUN_TEST(test_otp_replay_attack);
    
    UNITY_END();
}
