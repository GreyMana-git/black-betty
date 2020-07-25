#include "Settings.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "util.h"

constexpr int ADDRESS_OFFSET = 32;

Settings::Settings() : magic(0xB1ACBE11), // The magic number identifies the settings on the eeprom
                       version(1),
                       relay_pin(15),
                       heater_toggle_pin(12),
                       display_clock_pin(0),
                       display_dio_pin(2),
                       unused_in_v1(0),
                       heater_window(500),
                       heater_kp(50),
                       heater_ki(2),
                       heater_kd(1),
                       heater_temperature_low(104.0),
                       heater_temperature_high(135.0)
{
    // Zero all string to ensure they are always the same in every settings instance
    memset(this->device_id, 0, sizeof(this->device_id));
    memset(this->wifi_ssid, 0, sizeof(this->wifi_ssid));
    memset(this->wifi_password, 0, sizeof(this->wifi_password));
}

bool Settings::validate_set_device_id(const char *id)
{
    if (*id == 0x00 || strlen(id) >= array_size(this->device_id)) {
        return false;
    }
    
    memset(this->device_id, 0, sizeof(this->device_id));
    strncpy(this->device_id, id, array_size(this->device_id) - 1);
    return true;
}

bool Settings::validate_set_wifi(const char* id, const char *ssid, const char *password)
{
    if (!this->validate_set_device_id(id)) {
        return false;
    }
    
    if (strlen(ssid) >= array_size(this->wifi_ssid) || strlen(password) >= array_size(this->wifi_password)) {
        return false;
    }
    
    memset(this->wifi_ssid, 0, sizeof(this->wifi_ssid));
    strncpy(this->wifi_ssid, ssid, array_size(this->wifi_ssid) - 1);
    
    memset(this->wifi_password, 0, sizeof(this->wifi_password));
    strncpy(this->wifi_password, password, array_size(this->wifi_password) - 1);
    return true;
}

bool Settings::validate_set_heater_temperature_low(double value) {
    if (value < 20.0 || value > 150.0) {
        return false;
    }

    this->heater_temperature_low = value;
    return true;
}

bool Settings::validate_set_heater_temperature_high(double value) {
    if (value < 20.0 || value > 150.0) {
        return false;
    }

    this->heater_temperature_high = value;
    return true;
}

bool Settings::validate_set_heater_pid(double kp, double ki, double kd) {
    if (kp <= 0.0 || kp >= 1000.0 || ki <= 0.0 || ki >= 1000.0 || kd <= 0 || kd >= 1000.0) {
        return false;
    }

    this->heater_kp = kp;
    this->heater_ki = ki;
    this->heater_kd = kd;
    return true;
}

bool Settings::validate_set_heater_window(int value) {
    if (value <= 0 || value >= UINT16_MAX) {
        return false;
    }

    this->heater_window = static_cast<uint16_t>(value);
    return true;
}

void Settings::load()
{
    // Load the data from eprom into memory (including offset bytes)
    Settings loaded;
    EEPROM.begin(ADDRESS_OFFSET + sizeof(*this));
    EEPROM.get<Settings>(ADDRESS_OFFSET, loaded);
    EEPROM.end();

    if (loaded.magic != this->magic)
    {
        Serial.println(F("Settings::load Magic word does not match, using defaults"));
        return;
    }

    if (loaded.version > this->version)
    {
        Serial.println(F("Settings::load Stored version is higher that current"));
        return;
    }

    if (loaded.version == this->version)
    {
        Serial.println(F("Settings::load Settings loaded"));
        memcpy(this, &loaded, sizeof(*this));
        return;
    }

    Serial.println(F("Settings::load Unable to load settings, using default"));
}

void Settings::save() const
{
    // Not so good because it will write all bytes all the time
    EEPROM.begin(ADDRESS_OFFSET + sizeof(*this));
    EEPROM.put<Settings>(ADDRESS_OFFSET, *this);
    EEPROM.end();
}

void Settings::clear()
{
    Settings cleared;
    memcpy(this, &cleared, sizeof(*this));
    this->save();
}

Settings& get_settings()
{
    static Settings instance;
    return instance;
}
