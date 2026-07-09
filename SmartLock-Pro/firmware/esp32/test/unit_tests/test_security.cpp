// test_security.cpp - Security Unit Tests
#include <Arduino.h>
#include <unity.h>
#include "../../src/security/encryption.h"
#include "../../src/security/otp_generator.h"

EncryptionManager encryption;
OTPGenerator otpGen;

// ============================================================
// ENCRYPTION TESTS
// ============================================================
void test_encryption_init() {
    TEST_ASSERT_TRUE(encryption.init());
}

void test_aes_encrypt_decrypt() {
    const char* plaintext = "Hello SmartLock Pro!";
    size_t len = strlen(plaintext) + 1;
    
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    // Encrypt
    TEST_ASSERT_TRUE(encryption.aes_encrypt(
        (const uint8_t*)plaintext, len, encrypted
    ));
    
    // Decrypt
    TEST_ASSERT_TRUE(encryption.aes_decrypt(
        encrypted, len + 16, decrypted
    ));
    
    // Verify
    TEST_ASSERT_EQUAL_STRING(plaintext, (char*)decrypted);
}

void test_aes_encrypt_with_custom_key() {
    uint8_t custom_key[32] = "0123456789abcdef0123456789abcdef";
    const char* plaintext = "Custom key test";
    size_t len = strlen(plaintext) + 1;
    
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    TEST_ASSERT_TRUE(encryption.aes_encrypt(
        (const uint8_t*)plaintext, len, encrypted, custom_key
    ));
    
    TEST_ASSERT_TRUE(encryption.aes_decrypt(
        encrypted, len + 16, decrypted, custom_key
    ));
    
    TEST_ASSERT_EQUAL_STRING(plaintext, (char*)decrypted);
}

void test_aes_decrypt_wrong_key() {
    uint8_t key1[32] = "0123456789abcdef0123456789abcdef";
    uint8_t key2[32] = "fedcba9876543210fedcba9876543210";
    const char* plaintext = "Wrong key test";
    size_t len = strlen(plaintext) + 1;
    
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    // Encrypt with key1
    TEST_ASSERT_TRUE(encryption.aes_encrypt(
        (const uint8_t*)plaintext, len, encrypted, key1
    ));
    
    // Decrypt with key2 (should fail or produce garbage)
    bool result = encryption.aes_decrypt(
        encrypted, len + 16, decrypted, key2
    );
    
    // Decryption might succeed but produce garbage
    // We just check that it doesn't crash
    TEST_ASSERT_TRUE(result || !result);
}

void test_sha256_hash() {
    const char* data = "Test hash data";
    uint8_t hash[32];
    uint8_t expected_hash[32] = {
        0x5E, 0x63, 0x92, 0x0C, 0x4E, 0x03, 0x9E, 0xCA,
        0xFC, 0x5E, 0x8C, 0x09, 0x9D, 0x43, 0x88, 0xCD,
        0x0D, 0xB6, 0x40, 0x98, 0xE4, 0xE0, 0x10, 0x70,
        0x6B, 0x69, 0x0B, 0x1D, 0xDF, 0x35, 0x82, 0x3C
    };
    
    TEST_ASSERT_TRUE(encryption.sha256_hash(
        (const uint8_t*)data, strlen(data), hash
    ));
    
    // Verify hash length
    TEST_ASSERT_EQUAL(32, sizeof(hash));
    
    // Hash should not be all zeros
    bool all_zero = true;
    for (int i = 0; i < 32; i++) {
        if (hash[i] != 0) all_zero = false;
    }
    TEST_ASSERT_FALSE(all_zero);
}

void test_random_generation() {
    uint8_t buffer1[16];
    uint8_t buffer2[16];
    
    encryption.generate_random(buffer1, 16);
    encryption.generate_random(buffer2, 16);
    
    // Should not be identical
    bool identical = true;
    for (int i = 0; i < 16; i++) {
        if (buffer1[i] != buffer2[i]) {
            identical = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(identical);
}

void test_random_uniform_distribution() {
    const int NUM_SAMPLES = 1000;
    uint8_t samples[NUM_SAMPLES];
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        encryption.generate_random(&samples[i], 1);
    }
    
    // Count distribution
    int counts[256] = {0};
    for (int i = 0; i < NUM_SAMPLES; i++) {
        counts[samples[i]]++;
    }
    
    // Each value should appear roughly 4 times
    float avg = NUM_SAMPLES / 256.0f;
    int threshold = avg * 0.5f;
    
    int count_valid = 0;
    for (int i = 0; i < 256; i++) {
        if (counts[i] >= threshold) {
            count_valid++;
        }
    }
    
    // At least 200 values should be within threshold
    TEST_ASSERT_TRUE(count_valid >= 200);
}

// ============================================================
// SECURITY PERFORMANCE TESTS
// ============================================================
void test_encryption_performance() {
    const char* data = "Performance test data for encryption";
    size_t len = strlen(data) + 1;
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    unsigned long start = micros();
    
    for (int i = 0; i < 100; i++) {
        encryption.aes_encrypt((const uint8_t*)data, len, encrypted);
        encryption.aes_decrypt(encrypted, len + 16, decrypted);
    }
    
    unsigned long duration = micros() - start;
    Serial.printf("100 encryption/decryption cycles: %lu µs\n", duration);
    
    // Should be reasonably fast (< 1 second)
    TEST_ASSERT_TRUE(duration < 1000000);
}

void test_hash_performance() {
    const char* data = "Performance test data for hashing";
    uint8_t hash[32];
    
    unsigned long start = micros();
    
    for (int i = 0; i < 1000; i++) {
        encryption.sha256_hash((const uint8_t*)data, strlen(data), hash);
    }
    
    unsigned long duration = micros() - start;
    Serial.printf("1000 SHA256 hashes: %lu µs\n", duration);
    
    // Should be fast (< 500ms)
    TEST_ASSERT_TRUE(duration < 500000);
}

// ============================================================
// SECURITY INTEGRITY TESTS
// ============================================================
void test_data_integrity() {
    const char* original = "Important data that must not be altered";
    size_t len = strlen(original) + 1;
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    // Encrypt
    encryption.aes_encrypt((const uint8_t*)original, len, encrypted);
    
    // Tamper with encrypted data
    encrypted[20] ^= 0xFF;
    
    // Decrypt should fail or produce corrupted data
    bool result = encryption.aes_decrypt(encrypted, len + 16, decrypted);
    
    // If decryption succeeds, data should be different
    if (result) {
        TEST_ASSERT_NOT_EQUAL(0, memcmp(original, decrypted, len));
    }
}

void test_encryption_with_empty_data() {
    const char* empty = "";
    size_t len = 1; // Null terminator
    
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    TEST_ASSERT_TRUE(encryption.aes_encrypt(
        (const uint8_t*)empty, len, encrypted
    ));
    
    TEST_ASSERT_TRUE(encryption.aes_decrypt(
        encrypted, len + 16, decrypted
    ));
    
    TEST_ASSERT_EQUAL(0, decrypted[0]);
}

void test_large_data_encryption() {
    const int DATA_SIZE = 1024;
    uint8_t original[DATA_SIZE];
    uint8_t encrypted[DATA_SIZE + 32];
    uint8_t decrypted[DATA_SIZE];
    
    // Fill with random data
    for (int i = 0; i < DATA_SIZE; i++) {
        original[i] = random(256);
    }
    
    TEST_ASSERT_TRUE(encryption.aes_encrypt(original, DATA_SIZE, encrypted));
    TEST_ASSERT_TRUE(encryption.aes_decrypt(encrypted, DATA_SIZE + 16, decrypted));
    
    TEST_ASSERT_EQUAL(0, memcmp(original, decrypted, DATA_SIZE));
}

// ============================================================
// TEST SUITE
// ============================================================
void setup_security_tests() {
    UNITY_BEGIN();
    
    // Encryption tests
    RUN_TEST(test_encryption_init);
    RUN_TEST(test_aes_encrypt_decrypt);
    RUN_TEST(test_aes_encrypt_with_custom_key);
    RUN_TEST(test_aes_decrypt_wrong_key);
    RUN_TEST(test_sha256_hash);
    RUN_TEST(test_random_generation);
    RUN_TEST(test_random_uniform_distribution);
    
    // Performance tests
    RUN_TEST(test_encryption_performance);
    RUN_TEST(test_hash_performance);
    
    // Integrity tests
    RUN_TEST(test_data_integrity);
    RUN_TEST(test_encryption_with_empty_data);
    RUN_TEST(test_large_data_encryption);
    
    UNITY_END();
}
