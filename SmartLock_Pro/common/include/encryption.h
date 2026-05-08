#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define AES_KEY_SIZE 32
#define AES_BLOCK_SIZE 16
#define SHA256_DIGEST_SIZE 32
#define SALT_SIZE 16
#define HMAC_SIZE 32

// AES-256 context
typedef struct {
    uint8_t round_keys[240];
    int rounds;
} AES_CTX;

// Encryption functions
void aes_init(AES_CTX* ctx, const uint8_t* key);
void aes_encrypt_block(AES_CTX* ctx, const uint8_t* input, uint8_t* output);
void aes_decrypt_block(AES_CTX* ctx, const uint8_t* input, uint8_t* output);
void aes_encrypt_cbc(AES_CTX* ctx, const uint8_t* input, uint8_t* output, size_t length, const uint8_t* iv);
void aes_decrypt_cbc(AES_CTX* ctx, const uint8_t* input, uint8_t* output, size_t length, const uint8_t* iv);

// SHA256
void sha256(const uint8_t* data, size_t len, uint8_t* hash);
void hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, uint8_t* output);

// PBKDF2
void pbkdf2_hmac_sha256(const char* password, const uint8_t* salt, int iterations, uint8_t* output, size_t output_len);

// Base64
size_t base64_encode(const uint8_t* input, size_t input_len, char* output, size_t output_len);
size_t base64_decode(const char* input, uint8_t* output, size_t output_len);

// Random
void random_bytes(uint8_t* buffer, size_t length);
uint32_t random_uint32(void);

// JWT
typedef struct {
    char* token;
    char* header;
    char* payload;
    char* signature;
} jwt_t;

jwt_t* jwt_create(const char* user_id, const char* role, int exp_seconds, const char* secret);
bool jwt_verify(const char* token, const char* secret, char* user_id, size_t user_id_size);
void jwt_free(jwt_t* jwt);

// Password hashing
void hash_password(const char* password, const uint8_t* salt, uint8_t* output);
bool verify_password(const char* password, const uint8_t* salt, const uint8_t* hash);

#endif
