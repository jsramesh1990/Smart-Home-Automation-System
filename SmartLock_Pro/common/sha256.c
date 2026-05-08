#include "include/sha256.h"
#include <string.h>
#include <stdint.h>

#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb3, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void sha256_transform(SHA256_CTX* ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
    
    for (i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    }
    
    for (; i < 64; ++i) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256_init(SHA256_CTX* ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
    memset(ctx->buffer, 0, 64);
}

void sha256_update(SHA256_CTX* ctx, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->buffer[ctx->count & 63] = data[i];
        ctx->count++;
        
        if ((ctx->count & 63) == 0) {
            sha256_transform(ctx, ctx->buffer);
        }
    }
}

void sha256_final(SHA256_CTX* ctx, uint8_t* hash) {
    uint64_t bit_count = ctx->count * 8;
    size_t pad_len = 56 - (ctx->count & 63);
    
    if (pad_len <= 0) pad_len += 64;
    
    uint8_t padding[64] = {0x80};
    sha256_update(ctx, padding, pad_len);
    
    uint8_t length_bytes[8];
    for (int i = 7; i >= 0; --i) {
        length_bytes[i] = (uint8_t)(bit_count & 0xFF);
        bit_count >>= 8;
    }
    sha256_update(ctx, length_bytes, 8);
    
    for (int i = 0; i < 8; ++i) {
        hash[i * 4] = (ctx->state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = ctx->state[i] & 0xFF;
    }
}

void sha256(const uint8_t* data, size_t len, uint8_t* hash) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

void hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, uint8_t* output) {
    uint8_t k_ipad[64] = {0};
    uint8_t k_opad[64] = {0};
    uint8_t inner_hash[SHA256_DIGEST_SIZE];
    SHA256_CTX ctx;
    
    if (key_len > 64) {
        sha256(key, key_len, k_ipad);
        memcpy(k_ipad, k_ipad, SHA256_DIGEST_SIZE);
    } else {
        memcpy(k_ipad, key, key_len);
    }
    
    for (int i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    
    sha256_init(&ctx);
    sha256_update(&ctx, k_ipad, 64);
    sha256_update(&ctx, data, data_len);
    sha256_final(&ctx, inner_hash);
    
    sha256_init(&ctx);
    sha256_update(&ctx, k_opad, 64);
    sha256_update(&ctx, inner_hash, SHA256_DIGEST_SIZE);
    sha256_final(&ctx, output);
}

void pbkdf2_hmac_sha256(const char* password, const uint8_t* salt, int iterations, uint8_t* output, size_t output_len) {
    uint8_t digest[SHA256_DIGEST_SIZE];
    uint8_t block[4];
    uint8_t temp[SHA256_DIGEST_SIZE];
    size_t password_len = strlen(password);
    
    for (size_t i = 1; i <= output_len / SHA256_DIGEST_SIZE + 1; i++) {
        block[0] = (i >> 24) & 0xFF;
        block[1] = (i >> 16) & 0xFF;
        block[2] = (i >> 8) & 0xFF;
        block[3] = i & 0xFF;
        
        uint8_t* combined = (uint8_t*)malloc(32 + 4);
        memcpy(combined, salt, 32);
        memcpy(combined + 32, block, 4);
        
        hmac_sha256((const uint8_t*)password, password_len, combined, 36, digest);
        
        memcpy(temp, digest, SHA256_DIGEST_SIZE);
        
        for (int j = 1; j < iterations; j++) {
            hmac_sha256((const uint8_t*)password, password_len, digest, SHA256_DIGEST_SIZE, digest);
            for (int k = 0; k < SHA256_DIGEST_SIZE; k++) {
                temp[k] ^= digest[k];
            }
        }
        
        size_t offset = (i - 1) * SHA256_DIGEST_SIZE;
        size_t copy_len = SHA256_DIGEST_SIZE;
        if (offset + copy_len > output_len) {
            copy_len = output_len - offset;
        }
        memcpy(output + offset, temp, copy_len);
        
        free(combined);
    }
}

void hash_password(const char* password, const uint8_t* salt, uint8_t* output) {
    pbkdf2_hmac_sha256(password, salt, 10000, output, SHA256_DIGEST_SIZE);
}

bool verify_password(const char* password, const uint8_t* salt, const uint8_t* hash) {
    uint8_t computed_hash[SHA256_DIGEST_SIZE];
    pbkdf2_hmac_sha256(password, salt, 10000, computed_hash, SHA256_DIGEST_SIZE);
    return memcmp(hash, computed_hash, SHA256_DIGEST_SIZE) == 0;
}
