#include "include/json_parser.h"
#include <cstdlib>
#include <cstring>
#include <cctype>

JsonValue* json_parse(const char* str) {
    JsonValue* value = (JsonValue*)malloc(sizeof(JsonValue));
    if (!value) return nullptr;
    
    // Skip whitespace
    while (isspace(*str)) str++;
    
    switch (*str) {
        case '{': {
            value->type = JSON_OBJECT;
            value->object_value = json_object_new();
            str++;
            
            while (*str && *str != '}') {
                // Skip whitespace
                while (isspace(*str)) str++;
                
                // Parse key
                if (*str != '"') {
                    json_free(value);
                    return nullptr;
                }
                str++;
                
                char key[256];
                int key_len = 0;
                while (*str && *str != '"' && key_len < 255) {
                    key[key_len++] = *str++;
                }
                key[key_len] = '\0';
                str++;
                
                // Skip whitespace
                while (isspace(*str)) str++;
                if (*str != ':') {
                    json_free(value);
                    return nullptr;
                }
                str++;
                
                // Skip whitespace
                while (isspace(*str)) str++;
                
                // Parse value
                JsonValue* val = json_parse(str);
                if (!val) {
                    json_free(value);
                    return nullptr;
                }
                
                json_object_set(value->object_value, key, val);
                
                // Move str forward based on parsed value
                char temp[1024];
                json_stringify(val, temp, sizeof(temp));
                str += strlen(temp);
                
                while (isspace(*str)) str++;
                if (*str == ',') {
                    str++;
                    while (isspace(*str)) str++;
                }
            }
            if (*str == '}') str++;
            break;
        }
        
        case '[': {
            value->type = JSON_ARRAY;
            value->array_value = json_array_new();
            str++;
            
            while (*str && *str != ']') {
                while (isspace(*str)) str++;
                
                JsonValue* val = json_parse(str);
                if (!val) {
                    json_free(value);
                    return nullptr;
                }
                
                json_array_append(value->array_value, val);
                
                char temp[1024];
                json_stringify(val, temp, sizeof(temp));
                str += strlen(temp);
                
                while (isspace(*str)) str++;
                if (*str == ',') {
                    str++;
                    while (isspace(*str)) str++;
                }
            }
            if (*str == ']') str++;
            break;
        }
        
        case '"': {
            value->type = JSON_STRING;
            str++;
            
            char buf[1024];
            int buf_len = 0;
            while (*str && *str != '"' && buf_len < 1023) {
                if (*str == '\\') {
                    str++;
                    switch (*str) {
                        case 'n': buf[buf_len++] = '\n'; break;
                        case 't': buf[buf_len++] = '\t'; break;
                        case 'r': buf[buf_len++] = '\r'; break;
                        default: buf[buf_len++] = *str; break;
                    }
                } else {
                    buf[buf_len++] = *str;
                }
                str++;
            }
            buf[buf_len] = '\0';
            value->string_value = strdup(buf);
            if (*str == '"') str++;
            break;
        }
        
        case 't':
            if (strncmp(str, "true", 4) == 0) {
                value->type = JSON_BOOL;
                value->bool_value = true;
                str += 4;
            }
            break;
            
        case 'f':
            if (strncmp(str, "false", 5) == 0) {
                value->type = JSON_BOOL;
                value->bool_value = false;
                str += 5;
            }
            break;
            
        case 'n':
            if (strncmp(str, "null", 4) == 0) {
                value->type = JSON_NULL;
                str += 4;
            }
            break;
            
        default:
            if (isdigit(*str) || *str == '-') {
                value->type = JSON_NUMBER;
                char* endptr;
                value->number_value = strtod(str, &endptr);
                str = endptr;
            }
            break;
    }
    
    return value;
}

JsonObject* json_object_new() {
    JsonObject* obj = (JsonObject*)malloc(sizeof(JsonObject));
    if (obj) {
        obj->keys = nullptr;
        obj->values = nullptr;
        obj->count = 0;
        obj->capacity = 0;
    }
    return obj;
}

void json_object_set(JsonObject* obj, const char* key, JsonValue* value) {
    if (obj->count >= obj->capacity) {
        obj->capacity = obj->capacity == 0 ? 4 : obj->capacity * 2;
        obj->keys = (char**)realloc(obj->keys, obj->capacity * sizeof(char*));
        obj->values = (JsonValue**)realloc(obj->values, obj->capacity * sizeof(JsonValue*));
    }
    
    obj->keys[obj->count] = strdup(key);
    obj->values[obj->count] = value;
    obj->count++;
}

JsonValue* json_object_get(JsonObject* obj, const char* key) {
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->keys[i], key) == 0) {
            return obj->values[i];
        }
    }
    return nullptr;
}

JsonArray* json_array_new() {
    JsonArray* arr = (JsonArray*)malloc(sizeof(JsonArray));
    if (arr) {
        arr->values = nullptr;
        arr->count = 0;
        arr->capacity = 0;
    }
    return arr;
}

void json_array_append(JsonArray* arr, JsonValue* value) {
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity == 0 ? 4 : arr->capacity * 2;
        arr->values = (JsonValue**)realloc(arr->values, arr->capacity * sizeof(JsonValue*));
    }
    arr->values[arr->count++] = value;
}

void json_stringify(const JsonValue* value, char* output, size_t output_size) {
    if (!value || !output || output_size == 0) return;
    
    char buffer[4096];
    buffer[0] = '\0';
    
    switch (value->type) {
        case JSON_OBJECT:
            strcat(buffer, "{");
            for (int i = 0; i < value->object_value->count; i++) {
                if (i > 0) strcat(buffer, ",");
                char temp[256];
                snprintf(temp, sizeof(temp), "\"%s\":", value->object_value->keys[i]);
                strcat(buffer, temp);
                char val_str[1024];
                json_stringify(value->object_value->values[i], val_str, sizeof(val_str));
                strcat(buffer, val_str);
            }
            strcat(buffer, "}");
            break;
            
        case JSON_ARRAY:
            strcat(buffer, "[");
            for (int i = 0; i < value->array_value->count; i++) {
                if (i > 0) strcat(buffer, ",");
                char val_str[1024];
                json_stringify(value->array_value->values[i], val_str, sizeof(val_str));
                strcat(buffer, val_str);
            }
            strcat(buffer, "]");
            break;
            
        case JSON_STRING:
            snprintf(buffer, sizeof(buffer), "\"%s\"", value->string_value);
            break;
            
        case JSON_NUMBER:
            snprintf(buffer, sizeof(buffer), "%g", value->number_value);
            break;
            
        case JSON_BOOL:
            strcat(buffer, value->bool_value ? "true" : "false");
            break;
            
        case JSON_NULL:
            strcat(buffer, "null");
            break;
    }
    
    strncpy(output, buffer, output_size - 1);
    output[output_size - 1] = '\0';
}

void json_free(JsonValue* value) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_OBJECT:
            for (int i = 0; i < value->object_value->count; i++) {
                free(value->object_value->keys[i]);
                json_free(value->object_value->values[i]);
            }
            free(value->object_value->keys);
            free(value->object_value->values);
            free(value->object_value);
            break;
            
        case JSON_ARRAY:
            for (int i = 0; i < value->array_value->count; i++) {
                json_free(value->array_value->values[i]);
            }
            free(value->array_value->values);
            free(value->array_value);
            break;
            
        case JSON_STRING:
            free(value->string_value);
            break;
            
        default:
            break;
    }
    
    free(value);
}
