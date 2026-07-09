// encryption.cpp - Encryption Implementation
#include "encryption.h"
#include "config.h"
#include "constants.h"
#include "logger.h"
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

extern Logger logger;

EncryptionManager::EncryptionManager() {
    initialized = false;
}

bool EncryptionManager::init() {
    logger.log(LOG_LEVEL_INFO, "Encryption", "Initializing...");
    
    // Initialize mbedTLS context
    mbedtls_aes_init(&aes_ctx);
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_entropy_init(&entropy_ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
    
    // Seed random generator
    const char* pers = "smartlock_rng";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func, 
                                     &entropy_ctx, (const unsigned char*)pers, 
                                     strlen(pers));
    if (ret != 0) {
        logger.log(LOG_LEVEL_ERROR, "Encryption", "RNG seed failed: %d", ret);
        return false;
    }
    
    // Set encryption key
    if (strlen(ENCRYPTION_KEY) != 32) {
        logger.log(LOG_LEVEL_ERROR, "Encryption", "Invalid key length");
        return false;
    }
    
    memcpy(aes_key, ENCRYPTION_KEY, 32);
    
    initialized = true;
    logger.log(LOG_LEVEL_INFO, "Encryption", "Initialized successfully");
    return true;
}

bool EncryptionManager::aes_encrypt(const uint8_t* input, size_t len, 
                                     uint8_t* output, const uint8_t* key) {
    if (!initialized) return false;
    
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    
    // Set key
    if (mbedtls_aes_setkey_enc(&ctx, key ? key : aes_key, 256) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }
    
    // Generate random IV
    uint8_t iv[16];
    generate_random(iv, 16);
    
    // Copy IV to output
    memcpy(output, iv, 16);
    
    // Encrypt with CBC
    if (mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, len, 
                               iv, input, output + 16) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }
    
    mbedtls_aes_free(&ctx);
    return true;
}

bool EncryptionManager::aes_decrypt(const uint8_t* input, size_t len, 
                                     uint8_t* output, const uint8_t* key) {
    if (!initialized) return false;
    
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    
    // Set key
    if (mbedtls_aes_setkey_dec(&ctx, key ? key : aes_key, 256) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }
    
    // Extract IV
    uint8_t iv[16];
    memcpy(iv, input, 16);
    
    // Decrypt with CBC
    if (mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, len - 16, 
                               iv, input + 16, output) != 0) {
        mbedtls_aes_free(&ctx);
        return false;
    }
    
    mbedtls_aes_free(&ctx);
    return true;
}

bool EncryptionManager::sha256_hash(const uint8_t* data, size_t len, uint8_t* hash) {
    if (!initialized) return false;
    
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    
    if (mbedtls_sha256_starts_ret(&ctx, 0) != 0) {
        mbedtls_sha256_free(&ctx);
        return false;
    }
    
    if (mbedtls_sha256_update_ret(&ctx, data, len) != 0) {
        mbedtls_sha256_free(&ctx);
        return false;
    }
    
    if (mbedtls_sha256_finish_ret(&ctx, hash) != 0) {
        mbedtls_sha256_free(&ctx);
        return false;
    }
    
    mbedtls_sha256_free(&ctx);
    return true;
}

void EncryptionManager::generate_random(uint8_t* buffer, size_t len) {
    if (!initialized) {
        // Fallback to esp_random if not initialized
        for (size_t i = 0; i < len; i += 4) {
            uint32_t r = esp_random();
            memcpy(buffer + i, &r, min((size_t)4, len - i));
        }
        return;
    }
    
    mbedtls_ctr_drbg_random(&ctr_drbg_ctx, buffer, len);
}
