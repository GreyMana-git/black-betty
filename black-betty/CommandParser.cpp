#include "CommandParser.h"

#include <Arduino.h>

#include "util.h"
#include "Settings.h"
#include "HeaterPID.h"

static bool is_token(const char* token, const __FlashStringHelper* value_flash) {
  // Casting to char here enables pointer arimetrics later
  const char* value = reinterpret_cast<const char*>(value_flash);

  char c;
  while ((c = static_cast<char>(pgm_read_byte_near(value++))) != 0x00){
    if (tolower(c) != tolower(*(token++))) {
      return false;
    }
  }

  return true;
}

static bool is_token(const char* token, const __FlashStringHelper* value_flash, bool prerequisite) {
    if (!prerequisite) {
        return false;
    }
    
    return is_token(token, value_flash);
}



// Calculates the next security token
static int next_security_token() {
  return static_cast<int>(rand() & 0x7FFFFFFF);
}

CommandParser::CommandParser() : security_token_timeout(0) {
  memset(this->serialbuffer, 0, sizeof(this->serialbuffer));
  this->security_token[0] = next_security_token();
  this->security_token[1] = next_security_token();
}

void CommandParser::update() {
    unsigned long now = millis();
    if (now > this->security_token_timeout) {
        this->security_token[1] = this->security_token[0];
        this->security_token[0] = next_security_token();
        this->security_token_timeout = now + 300000; // 5 Minute token timeout
    }
    
    if (Serial.available() <= 0) {
        return;
    }

    int incomming = -1;
    int position = strlen(this->serialbuffer);
    const int buffersize = array_size(this->serialbuffer);
    
    while ((incomming = Serial.read()) != -1) {
        // Handle newline as execute signal
        if (incomming == '\n') {
            if (position < buffersize && this->serialbuffer[buffersize - 1] == 0x00) {
                this->serialbuffer[position] = 0x00;

                char output[128];
                output[0] = output[array_size(output) - 1] = 0x00;
                this->execute(this->serialbuffer, false, output, array_size(output));
                if (*output != 0x00) {
                  Serial.println(output);
                }
            } else {
                Serial.printf("CommandParser::pollSerial Buffer overflow, the command will be ignored.\n");
            }
            
            this->serialbuffer[0] = this->serialbuffer[position] = this->serialbuffer[buffersize] = 0x00;
            position = 0;
            continue;
        }
        else if (position < buffersize) {
            this->serialbuffer[position++] = static_cast<char>(incomming);
        }
    }

}

int CommandParser::get_security_token() const {
    return this->security_token[0];
}

bool CommandParser::validate_security_token(int token) const {
  return token != 0 && (token == this->security_token[0] || token == this->security_token[1]);
}

bool CommandParser::execute(const char* command, bool requires_security_token, char* output, size_t output_size) {
    // Tokenize the command
    char token_buffer[256];
    char* token[8];

    output[0] = output[output_size - 1] = 0x00;

    int token_count = tokenize(command, token_buffer, array_size(token_buffer), token, array_size(token));
    if (token_count == 0) {
        copy_flash_string(output, F("CommandParser::execute Invalid/Empty command"), output_size);
        return false;
    }

    bool success = false;
   
    if (requires_security_token || is_token(token[0], F("token"))) {
      if (is_token(token[0], F("token")) && token_count > 2) {
        if (this->validate_security_token(atoi(token[1]))) {
          success = this->main(token + 2, token_count - 2, output, output_size);
        }
      }
    } else {
      success = this->main(token, token_count, output, output_size);
    }

    if (success) {
        if (*output == 0x00) {
            copy_flash_string(output, F("ok"), output_size);
        }

        return true;
    }

    if (*output == 0x00) {
        copy_flash_string(output, F("Error executing command: "), output_size);
        strncpy(output + strlen(output), command, output_size - strlen(output) - 1);
    }

    return false;
}

bool CommandParser::main(char** token, const int token_count, char* output, size_t output_size) {
    if (is_token(token[0], F("get"), token_count > 1)) {
        return this->get(token + 1, token_count - 1, output, output_size);
    } else if (is_token(token[0], F("set"), token_count > 1)) {
        return this->set(token + 1, token_count - 1, output, output_size);
    } else if (is_token(token[0], F("save"))) {
        Serial.println(F("Saving configuration to eeprom"));
        get_settings().save();
        return true;
    } else if (is_token(token[0], F("purge"))) {
        Serial.println(F("Purging configuration, resetting to defaults"));
        get_settings().clear();
        return true;
    } else if (is_token(token[0], F("restart"))) {
        this->restart();
        return true;
    }

    return false;
}

bool CommandParser::get(char** token, const int token_count, char* output, size_t output_size) {
    Settings& settings = get_settings();
    HeaterPID& heater = get_heater();

    if (token_count < 1) {
        return false;
    }

    if (is_token(token[0], F("wifi.ssid"))) {
        strncpy(output, settings.wifi_ssid, output_size);
        return true;
    } else if (is_token(token[0], F("id"))) {
        strncpy(output, settings.device_id, output_size);
        return true;
    } else if (is_token(token[0], F("heater.low"), token_count > 1)) {
      double_to_string(output, settings.heater_temperature_low);
        return true;
    } else if (is_token(token[0], F("heater.high"), token_count > 1)) {
        double_to_string(output, settings.heater_temperature_high);
        return true;
    } else if (is_token(token[0], F("pid"))) {
        char* pos = json_add(output, heater.get_kp());
        *(pos++) = ' ';
        pos = json_add(pos, heater.get_ki());
        *(pos++) = ' ';
        double_to_string(pos, heater.get_kd());
    } else if (is_token(token[0], F("pid.kp"))) {
      double_to_string(output, heater.get_kp());
        return true;
    } else if (is_token(token[0], F("pid.ki"))) {
        double_to_string(output, heater.get_ki());
        return true;
    } else if (is_token(token[0], F("pid.kd"))) {
      double_to_string(output, heater.get_kd());
        return true;
    } else if (is_token(token[0], F("pid.input"))) {
        double_to_string(output, heater.get_input());
        return true;
    } else if (is_token(token[0], F("pid.ouput"))) {
        double_to_string(output, heater.get_output());
        return true;
    } else if (is_token(token[0], F("pid.setpoint"))) {
        double_to_string(output, heater.get_setpoint());
        return true;
    } 

    return false;
}

bool CommandParser::set(char** token, const int token_count, char* output, size_t output_size) {
    Settings& settings = get_settings();

    if (is_token(token[0], F("wifi"), token_count > 3)) {
        return settings.validate_set_wifi(token[1], token[2], token[3]);
    } else if (is_token(token[0], F("id"), token_count > 1)) {
        return settings.validate_set_device_id(token[1]);
    } else if (is_token(token[0], F("heater.low"), token_count > 1)) {
        return settings.validate_set_heater_temperature_low(atof(token[1]));
    } else if (is_token(token[0], F("heater.high"), token_count > 1)) {
        return settings.validate_set_heater_temperature_high(atof(token[1]));
    } else if (is_token(token[0], F("heater.enabled"), token_count > 1)) {
        if (is_token(token[1], F("true"))) {
            get_heater().enable();
            return true;
        } else if (is_token(token[1], F("false"))) {
            get_heater().disable();
            return true;
        }
    } else if (is_token(token[0], F("pid"), token_count > 3)) {
        // Reconfigure pid
        if (settings.validate_set_heater_pid(atof(token[1]), atof(token[2]), atof(token[3]))) {
            get_heater().configure(settings.heater_kp, settings.heater_ki, settings.heater_kd);
            return true;
        }
    }

    return false;
}

void CommandParser::restart() {
    Serial.println(F("Restarting device using watchdog in 4 seconds"));

    wdt_disable();
    wdt_enable(WDTO_4S);
    while (1) {}
}

CommandParser& get_command_parser() {
    static CommandParser instance;
    return instance;
}
