#include "util.h"

#include <Arduino.h>
#include "Status.h"

int tokenize(const char* source, char* dest, const int dest_count, char** token, const int token_count) {
    int count = 0;
    char delimiter = 0x00;
    char* dest_end = dest + dest_count - 1;

    // Clear the tokens and output buffer
    memset(token, 0, token_count * sizeof(char*));
    *dest = *dest_end = 0x00;

    // Empty string will be returned as one token
    if (*source == 0x00 && dest_count > 0 && token_count > 0) {
        *(token[0] = dest) = 0x00;
        return 1;
    }

    for (; *source != 0x00 && *source != '\n'; source++) {
        if (delimiter == 0x00) {
            if (*source == ' ') {
                continue;
            }

            if (count < token_count) {
                token[count++] = dest;
                delimiter = *source == '\"' ? *source : ' ';
                if (delimiter != ' ') {
                    source++;
                }
            }
            else {
                break;
            }
        }

        // Copy or end token
        if (*source != delimiter) {
            *(dest++) = *source;
        }
        else {
            if (delimiter == ' ') {
                source--;
            }

            *(dest++) = 0x00;
            delimiter = 0x00;
        }

        // Avoid overflow
        if (dest >= dest_end) {
            *dest = 0x00;
            break;
        }
    }

    *dest = 0x00;
    return count;
}

char* json_add(char* pos, const __FlashStringHelper* source_flash) {
  // Casting to char here enables pointer arimetrics
  const char* source = reinterpret_cast<const char*>(source_flash);
  
  char c;
  while ((c = static_cast<char>(pgm_read_byte_near(source++))) != 0x00){
    *(pos++) = c;
  }

  return pos;
}

char* json_add(char* pos, const char* source) {
  while (*source != 0x00) {
    *(pos++) = *(source)++;
  }
  
  return pos;
}

// An itoa emulation that returns the END position
char* json_add(char* pos, int value, int digits) {
    char* start = pos;
    if (digits > 12) {
        return pos;
    }

    // Handle zero value explicit because they mess up with the digit output (e.g. they will be rendered as 0.00 instead of 0.0)
    if (value == 0) {
      *(pos++) = '0';
      if (digits > 0) {
        *(pos++) = '.';
        *(pos++) = '0';
      }
      
      return pos;
    }
    
    char reverse_buffer[16];
    char* reverse_pos = reverse_buffer;
    int quotient = abs(value);
    bool still_zero = digits > 0;

    do {
        const int tmp = quotient / 10;
        const int digit_value = quotient - (tmp * 10);
        still_zero = still_zero && digits > 1 && digit_value == 0;
        if (!still_zero) {
            *(reverse_pos++) = "0123456789"[digit_value];
        }
        
        quotient = tmp;

        if (--digits == 0) {
            *(reverse_pos++) = '.';
        }
    } while (quotient != 0);

    // Add digit symbol on smaller numbers
    if (digits > 0) {
        while (digits-- > 0) {
            *(reverse_pos++) = '0';
        }

        *(reverse_pos++) = '.';
    }

    // Always have a leading zero
    if (*(reverse_pos - 1) == '.') {
        *(reverse_pos++) = '0';
    }

    // Apply negative sign
    if (value < 0) {
        *(reverse_pos++) = '-';
    }

    // Copy reversed to buffer
    --reverse_pos;
    while (reverse_pos >= reverse_buffer) {
        *(pos++) = *(reverse_pos--);
    };

    return pos;
}

char* json_add(char* pos, double value, double min, double max) {
  if (min != 0.0 && max != 0.0) {
    value = clamp(value, min, max);
  }
  return json_add(pos, static_cast<int>(value * 1000.0), 3);
}

static char* json_add_prefix(char* pos, const __FlashStringHelper* name) {
  *(pos++) = '\"';
  pos = json_add(pos, name);
  *(pos++) = '\"';
  *(pos++) = ':';
  return pos;
}

static char* json_add_postfix(char* pos, bool add_comma) {
    if (add_comma) {
    *(pos++) = ',';
  }
  
  return pos;
}

char* json_add_property(char* pos, const __FlashStringHelper* name, const char* value, bool add_comma) {
  pos = json_add_prefix(pos, name);
  *(pos++) = '\"';
  pos = json_add(pos, value);
  *(pos++) = '\"';
  return json_add_postfix(pos, add_comma);
}

char* json_add_property(char* pos, const __FlashStringHelper* name, int value, bool add_comma) {
  pos = json_add_prefix(pos, name);
  pos = json_add(pos, value);
  return json_add_postfix(pos, add_comma);
}

char* json_add_property(char* pos, const __FlashStringHelper* name, double value, bool add_comma) {
  pos = json_add_prefix(pos, name);
  pos = json_add(pos, value, 0.0, 0.0);
  return json_add_postfix(pos, add_comma);
}

char* json_add_property(char* pos, const __FlashStringHelper* name, bool value, bool add_comma) {
  pos = json_add_prefix(pos, name);
  pos = json_add(pos, value ? F("true") : F("false"));
  return json_add_postfix(pos, add_comma);
}

char* json_add_counter(char* pos, const StatCounter& counter, bool first, int samples) {
  if (!first) {
  *(pos++) = ',';
  }
    pos = json_add(pos, counter.current, -99.999, 999.999);
    *(pos++) = ',';
    pos = json_add(pos, counter.min, -99.999, 999.999);
    *(pos++) = ',';
    pos = json_add(pos, counter.max, -99.999, 999.999);
    *(pos++) = ',';
    if (samples > 0) {
      pos = json_add(pos, counter.sum / static_cast<double>(samples), -99.999, 999.999);
    } else {
      *(pos++) = '0';
      *(pos++) = '.';
      *(pos++) = '0';
    }

    return pos;
}

char* json_add_array_item(char* pos, int value, bool first) {
  if (!first) {
    *(pos++) = ',';
  }

  return json_add(pos, value);
}

char* double_to_string(char* output, double value) {
  char* end = json_add(output, value);
  *end = 0x00;
  return output;
}

char* copy_flash_string(char* output, const __FlashStringHelper* source_flash, size_t count) {
  // Casting to char here enables pointer arimetrics
  const char* source = reinterpret_cast<const char*>(source_flash);
  char c;
  
  while ((c = static_cast<char>(pgm_read_byte_near(source++))) != 0x00){
    if (--count <= 1) {
      break;
    }

    *(output++) = c;
  }

  *output = 0x00;
  return output;
}
