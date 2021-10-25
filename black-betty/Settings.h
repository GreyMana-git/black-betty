#pragma once

#include <stdint.h>

enum SettingsFlags {
  FLAG_DEBUG = 0x01, // Device debug mode. This allows CORS requests to the device
  FLAG_COUNTDOWN_MODE = 0x02 // Puts the device in countdown mode, this showns a counter when pressing the high temperature button instead of changing the setpoint
};

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

    bool is_debug() const;
    void set_debug(bool enable);

    bool is_countdown_mode() const;
    void set_countdown_mode(bool enable);

    uint32_t magic;
    uint16_t heater_window;
    uint8_t version;
    uint8_t relay_pin;
    uint8_t heater_toggle_pin;
    uint8_t display_clock_pin;
    uint8_t display_dio_pin;
    uint8_t flags;
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
