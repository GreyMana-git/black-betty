#pragma once

#include <stdint.h>

/** Class for managing the variables in the project. This also supports serializing/storing/loading */
class Settings
{
public:
    Settings();
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    // Loads the settings from the EEPROM
    void load();

    // Saves the settings to the EEPROM
    void save() const;

    // Use to wipe out passwords etc.
    void clear();

    bool validate_set_device_id(const char *id);
    bool validate_set_wifi(const char* id, const char *ssid, const char *password);
    bool validate_set_heater_temperature_low(double value);
    bool validate_set_heater_temperature_high(double value);
    bool validate_set_heater_pid(double kp, double ki, double kd);
    bool validate_set_heater_window(int value);

    uint32_t magic;
    uint8_t version;
    uint8_t relay_pin;
    uint8_t heater_toggle_pin;
    uint8_t display_clock_pin;
    uint8_t display_dio_pin;
    uint16_t heater_window;
    uint8_t unused_in_v1;
    char device_id[16];
    char wifi_ssid[32];
    char wifi_password[32];
    double heater_kp;
    double heater_ki;
    double heater_kd;
    double heater_temperature_low;
    double heater_temperature_high;
};

// Use this function to get the settings, there should be (outside of this class) only one settings instance
Settings& get_settings();
