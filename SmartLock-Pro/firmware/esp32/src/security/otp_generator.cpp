// otp_generator.cpp - OTP Generator Implementation
#include "otp_generator.h"
#include "config.h"
#include "constants.h"
#include "logger.h"
#include <mbedtls/sha1.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

extern Logger logger;

OTPGenerator::OTPGenerator() {
    initialized = false;
    timeOffset = 0;
}

bool OTPGenerator::init() {
    logger.log(LOG_LEVEL_INFO, "OTP", "Initializing...");
    
    // Initialize random generator
    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
    
    const char* pers = "smartlock_otp";
    if (mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func, 
                               &entropy_ctx, (const unsigned char*)pers, 
                               strlen(pers)) != 0) {
        logger.log(LOG_LEVEL_ERROR, "OTP", "Failed to seed RNG");
        return false;
    }
    
    // Set secret key
    memcpy(secretKey, OTP_SECRET, 32);
    
    initialized = true;
    logger.log(LOG_LEVEL_INFO, "OTP", "Initialized successfully");
    return true;
}

String OTPGenerator::generateOTP() {
    if (!initialized) return "";
    
    // Get current time slot
    uint32_t timeSlot = (uint32_t)(time(nullptr) + timeOffset) / OTP_PERIOD;
    
    // Convert to bytes
    uint8_t msg[8];
    for (int i = 7; i >= 0; i--) {
        msg[i] = timeSlot & 0xFF;
        timeSlot >>= 8;
    }
    
    // Generate HMAC-SHA1
    uint8_t hmac[20];
    generateHMAC(msg, 8, hmac);
    
    // Dynamic truncation
    int offset = hmac[19] & 0x0F;
    uint32_t binary = ((hmac[offset] & 0x7F) << 24) |
                       ((hmac[offset + 1] & 0xFF) << 16) |
                       ((hmac[offset + 2] & 0xFF) << 8) |
                       (hmac[offset + 3] & 0xFF);
    
    // Generate 6-digit code
    uint32_t otp = binary % 1000000;
    
    // Format with leading zeros
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%06lu", (unsigned long)otp);
    
    return String(buffer);
}

bool OTPGenerator::validateOTP(const String& otp) {
    if (!initialized || otp.length() != 6) return false;
    
    // Check current and previous time slots (allow 1 slot drift)
    for (int i = -1; i <= 1; i++) {
        uint32_t timeSlot = (uint32_t)(time(nullptr) + timeOffset) / OTP_PERIOD + i;
        if (validateOTPForSlot(otp, timeSlot)) {
            return true;
        }
    }
    
    return false;
}

void OTPGenerator::generateHMAC(const uint8_t* msg, size_t len, uint8_t* hmac) {
    // Simple HMAC-SHA1 implementation
    // In production, use mbedtls HMAC functions
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);
    
    // Key padding
    uint8_t key[64];
    for (int i = 0; i < 64; i++) {
        key[i] = i < 32 ? secretKey[i] : 0;
    }
    
    // XOR with 0x36
    uint8_t k_ipad[64];
    for (int i = 0; i < 64; i++) {
        k_ipad[i] = key[i] ^ 0x36;
    }
    
    // SHA1(k_ipad + msg)
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, k_ipad, 64);
    mbedtls_sha1_update(&ctx, msg, len);
    mbedtls_sha1_finish(&ctx, hmac);
    
    // XOR with 0x5C
    uint8_t k_opad[64];
    for (int i = 0; i < 64; i++) {
        k_opad[i] = key[i] ^ 0x5C;
    }
    
    // SHA1(k_opad + previous)
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, k_opad, 64);
    mbedtls_sha1_update(&ctx, hmac, 20);
    mbedtls_sha1_finish(&ctx, hmac);
    
    mbedtls_sha1_free(&ctx);
}

bool OTPGenerator::validateOTPForSlot(const String& otp, uint32_t timeSlot) {
    // Convert time slot to bytes
    uint8_t msg[8];
    for (int i = 7; i >= 0; i--) {
        msg[i] = timeSlot & 0xFF;
        timeSlot >>= 8;
    }
    
    // Generate HMAC
    uint8_t hmac[20];
    generateHMAC(msg, 8, hmac);
    
    // Dynamic truncation
    int offset = hmac[19] & 0x0F;
    uint32_t binary = ((hmac[offset] & 0x7F) << 24) |
                       ((hmac[offset + 1] & 0xFF) << 16) |
                       ((hmac[offset + 2] & 0xFF) << 8) |
                       (hmac[offset + 3] & 0xFF);
    
    uint32_t expected = binary % 1000000;
    
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%06lu", (unsigned long)expected);
    return otp.equals(String(buffer));
}
