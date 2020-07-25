#pragma once
#include <stddef.h>

// Get the size of an array without hardcoding numbers. Way back this was a macro, now it seems to be done via constexpr
template <size_t N, class T> static constexpr size_t array_size(T (&)[N]) { return N; }

template<typename T> inline T clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

// Tokenizes a string seperated by space including quote support
int tokenize(const char* source, char* dest, const int dest_count, char** token, const int token_count);

class StatCounter;
class __FlashStringHelper;

// Json support
char* json_add(char* pos, const __FlashStringHelper* source_flash);
char* json_add(char* pos, const char* source);
char* json_add(char* pos, int value, int digits = -1);
char* json_add(char* pos, double value, double min = 0.0, double max = 0.0);
char* json_add_property(char* pos, const __FlashStringHelper* name, const char* value, bool add_comma);
char* json_add_property(char* pos, const __FlashStringHelper* name, int value, bool add_comma);
char* json_add_property(char* pos, const __FlashStringHelper* name, double value, bool add_comma);
char* json_add_property(char* pos, const __FlashStringHelper* name, bool value, bool add_comma);
char* json_add_counter(char* pos, const StatCounter& counter, bool first, int samples);
char* json_add_array_item(char* pos, int value, bool first);

char* double_to_string(char* output, double value);

// Flash string helper
char* copy_flash_string(char* output, const __FlashStringHelper* source_flash, size_t count);
