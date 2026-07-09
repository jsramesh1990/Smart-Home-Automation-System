// encryption.h - Encryption Header
#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

class EncryptionManager {
private:
    mbedtls_aes_context aes_ctx;
    mbedtls_sha256_context sha_ctx;
    mbedtls_entropy_context entropy_ctx;
    mbedtls_ctr_drbg_context ctr_drbg_ctx;
    uint8_t aes_key[32];
    bool initialized;
    
public:
    EncryptionManager();
    bool init();
    bool aes_encrypt(const uint8_t* input, size_t len, uint8_t* output, 
                     const uint8_t* key = nullptr);
    bool aes_decrypt(const uint8_t* input, size_t len, uint8_t* output, 
                     const uint8_t* key = nullptr);
    bool sha256_hash(const uint8_t* data, size_t len, uint8_t* hash);
    void generate_random(uint8_t* buffer, size_t len);
};

#endif // ENCRYPTION_H
