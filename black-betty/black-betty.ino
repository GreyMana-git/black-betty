#include <WString.h>
#include <EasyADT7410.h>
#include <TM1637Display.h>

#include "util.h"
#include "Settings.h"
#include "HeaterPID.h"
#include "WebServer.h"
#include "CommandParser.h"
#include "Status.h"

static TM1637Display& get_display() {
  const Settings &settings = get_settings();
  static TM1637Display instance(settings.display_clock_pin, settings.display_dio_pin); //CLK, DIO
  return instance;
}

static ADT7410& get_sensor()
{
  static ADT7410 instance;
  return instance;
}

void nextStep(const __FlashStringHelper* message) {
  if (message != nullptr) {
    Serial.println(message);
  }
  
  static int setup_step = 0;
  get_display().showNumberDecEx(++setup_step, 0, false, 4, 0);
}

void setup()
{
  // Serial connection
  Serial.begin(115200);
  delay(500);

  Serial.println(F("Loading settings..."));
  Settings &settings = get_settings();
  settings.load();

  Serial.printf("Welcome on \"%s\"\n", settings.device_id);

  // Display
  Serial.println(F("Setting up display..."));
  TM1637Display &display = get_display();
  display.setBrightness(2); //set the diplay brightness 0-7

  // Temperature sensor
  nextStep(F("Setting up temperature sensor..."));
  ADT7410 &sensor = get_sensor();
  sensor.begin();

  // Heater pid
  nextStep(F("Setting up header pid..."));
  HeaterPID &heater = get_heater();
  heater.set_setpoint(settings.heater_temperature_low);

  // Pins
  nextStep(F("Setting up pins..."));
  pinMode(settings.relay_pin, OUTPUT);
  pinMode(settings.heater_toggle_pin, INPUT);

  // Setup server
  Serial.printf("Setting up web server. Connecting to %s\n", settings.wifi_ssid);
  nextStep(nullptr);
  get_webserver().connect(settings.device_id, settings.wifi_ssid, settings.wifi_password, 30000);

  // Activate heater pid
  nextStep(F("Setting up PID..."));
  heater.enable();

  delay(100);
  Serial.println("Setup successful");
}

void loop() {
  unsigned long start = millis();
  const Settings& settings = get_settings();
  Status& status = get_status();
  HeaterPID &heater = get_heater();

  // Execute console CommandParser first to be responsible if something bad happens after this
  if (status.console_timer.next()) {
    get_command_parser().update();
  }

  // Measure temperature
  double temperature = get_sensor().readTemperature(); // 50.0 + static_cast<double>(rand() % 10); 
  status.temperature = temperature;
  status.is_heater_toggle_active = digitalRead(settings.heater_toggle_pin) == HIGH;

  // Update heater
  if (status.heater_timer.next()) {
    // Update heater rely
    heater.compute(temperature);
    digitalWrite(settings.relay_pin, heater.is_active() ? HIGH : LOW);
    
    if (!heater.is_enabled()) {
      status.heater_mode = HeaterMode::off;
    } else if (settings.is_countdown_mode() || !status.is_heater_toggle_active) {
      status.heater_mode = HeaterMode::low;
    } else {
      status.heater_mode = HeaterMode::high;
    }

    heater.set_setpoint(status.heater_mode == HeaterMode::low ? settings.heater_temperature_low : settings.heater_temperature_high);
  }

  // Update display. This is a 4 digit display, the last number is 0.1, so multiply by 10 for displaying
  if (status.display_timer.next()) {
    // Use the first dot as on/off indicator for the heater
    const uint8_t dots = heater.is_active() ? 0b10100000 : 0b00100000;
    if (settings.is_countdown_mode() && status.is_heater_toggle_active) {
      int elapsed = status.update_countdown() / 100;
      if (status.display_needs_update(elapsed, heater.is_active())) {
        get_display().showNumberDecEx(elapsed, dots, false, 4, 0); //(number, dots, leading_zeros, length, position)
      }
    } else {
      // Normal temperature mode
      status.countdown_start = 0;
      const int display_temperature = static_cast<int>(status.temperature * 10.0);
      if (status.display_needs_update(display_temperature, heater.is_active())) {
        get_display().showNumberDecEx(display_temperature, dots, false, 4, 0); //(number, dots, leading_zeros, length, position)
      }
    }
  }

  // Handle web requests
  if (status.webserver_timer.next()) {
    get_webserver().serve();
  }

  // Serial status alive
  if (status.alive_timer.next()) {
    status.sendStatus();
  }

  status.update_history(temperature, heater.get_output(), heater.is_active(), static_cast<double>(millis() - start));

  // Wait a nick of time
  delay(5);
}
