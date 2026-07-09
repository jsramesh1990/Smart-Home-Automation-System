// otp_generator.h - OTP Generator Header
#ifndef OTP_GENERATOR_H
#define OTP_GENERATOR_H

#include <Arduino.h>
#include <time.h>
#include <mbedtls/sha1.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

class OTPGenerator {
private:
    uint8_t secretKey[32];
    bool initialized;
    int32_t timeOffset;
    mbedtls_entropy_context entropy_ctx;
    mbedtls_ctr_drbg_context ctr_drbg_ctx;
    
    void generateHMAC(const uint8_t* msg, size_t len, uint8_t* hmac);
    bool validateOTPForSlot(const String& otp, uint32_t timeSlot);
    
public:
    OTPGenerator();
    bool init();
    String generateOTP();
    bool validateOTP(const String& otp);
    void setTimeOffset(int32_t offset) { timeOffset = offset; }
};

#endif // OTP_GENERATOR_H
