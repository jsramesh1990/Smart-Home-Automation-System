#include "include/base64.h"
#include <string.h>

static const char base64_table[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char base64_pad = '=';

size_t base64_encode(const uint8_t* input, size_t input_len, char* output, size_t output_len) {
    size_t required_len = ((input_len + 2) / 3) * 4 + 1;
    
    if (output_len < required_len) {
        return 0;
    }
    
    size_t i, j;
    for (i = 0, j = 0; i < input_len; i += 3, j += 4) {
        uint32_t octet_a = i < input_len ? input[i] : 0;
        uint32_t octet_b = i + 1 < input_len ? input[i + 1] : 0;
        uint32_t octet_c = i + 2 < input_len ? input[i + 2] : 0;
        
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        output[j] = base64_table[(triple >> 18) & 0x3F];
        output[j + 1] = base64_table[(triple >> 12) & 0x3F];
        output[j + 2] = base64_table[(triple >> 6) & 0x3F];
        output[j + 3] = base64_table[triple & 0x3F];
    }
    
    size_t mod = input_len % 3;
    if (mod == 1) {
        output[j - 2] = base64_pad;
        output[j - 1] = base64_pad;
    } else if (mod == 2) {
        output[j - 1] = base64_pad;
    }
    
    output[required_len - 1] = '\0';
    return required_len - 1;
}

static int base64_char_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t base64_decode(const char* input, uint8_t* output, size_t output_len) {
    size_t input_len = strlen(input);
    if (input_len % 4 != 0) return 0;
    
    size_t output_pos = 0;
    
    for (size_t i = 0; i < input_len; i += 4) {
        int v0 = base64_char_value(input[i]);
        int v1 = base64_char_value(input[i + 1]);
        int v2 = input[i + 2] == base64_pad ? -1 : base64_char_value(input[i + 2]);
        int v3 = input[i + 3] == base64_pad ? -1 : base64_char_value(input[i + 3]);
        
        if (v0 < 0 || v1 < 0) return 0;
        
        uint32_t triple = (v0 << 18) | (v1 << 12);
        
        if (v2 >= 0) {
            triple |= (v2 << 6);
            if (v3 >= 0) {
                triple |= v3;
            }
        }
        
        if (output_pos < output_len) {
            output[output_pos++] = (triple >> 16) & 0xFF;
        }
        
        if (v2 >= 0 && output_pos < output_len) {
            output[output_pos++] = (triple >> 8) & 0xFF;
        }
        
        if (v3 >= 0 && output_pos < output_len) {
            output[output_pos++] = triple & 0xFF;
        }
    }
    
    return output_pos;
}
