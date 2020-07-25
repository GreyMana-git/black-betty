#include "HeaterPID.h"

#include <stdint.h>
#include <Arduino.h>

#include "Settings.h"

HeaterPID::HeaterPID() : input(0),
                         output(0),
                         setpoint(0),
                         window(100),
                         active(false),
                         pid(&input, &output, &setpoint, 0.0, 0.0, 0.0, DIRECT) {
    const Settings &settings = get_settings();
    this->configure(settings.heater_kp, settings.heater_ki, settings.heater_kd);
    this->setpoint = settings.heater_temperature_low;
    this->window = settings.heater_window;
}

double HeaterPID::get_kp() const { return const_cast<HeaterPID*>(this)->pid.GetKp(); }
double HeaterPID::get_ki() const { return const_cast<HeaterPID*>(this)->pid.GetKi(); }
double HeaterPID::get_kd() const { return const_cast<HeaterPID*>(this)->pid.GetKd(); }
double HeaterPID::get_input() const { return this->input; }
double HeaterPID::get_output() const { return this->output; }
double HeaterPID::get_setpoint() const { return this->setpoint; }

void HeaterPID::compute(double input)
{
    // If the PID is disabled, the digital state is off
    if (this->pid.GetMode() == MANUAL)
    {
        this->active = false;
        return;
    }

    // Compute the current PID
    this->input = input;
    this->pid.Compute();

    int elapsed = static_cast<int>(millis() % static_cast<unsigned long>(this->window));
    this->active = elapsed < static_cast<int>(this->output);

    // Disable the heater if the temperature is lower than 5° and assume that the temperature sensor is borked
    if (input < 5.0) {
        this->active = false;
    }
}

void HeaterPID::set_setpoint(double setpoint)
{
    // Limit setpoint to hard coded range
    setpoint = setpoint < 20.0 ? 20.0 : (setpoint > 150.0 ? 150.0 : setpoint);
    this->setpoint = setpoint;
}

void HeaterPID::set_window(int window)
{
    // The window will be stored as u16, so limit it to that size
    if (get_settings().validate_set_heater_window(window)) {
        this->window = window;
        this->pid.SetOutputLimits(0.0, static_cast<double>(window));
    } else {
        Serial.printf("HeaterPID::set_window Invalid value for window: %d\n", window);
    }
}

void HeaterPID::configure(double kp, double ki, double kd)
{
    this->pid.SetTunings(kp, ki, kd);
    Settings& settings = get_settings();
    settings.heater_kp = kp;
    settings.heater_ki = ki;
    settings.heater_kd = kd;
}

bool HeaterPID::is_active() const {
  return this->active;
}

bool HeaterPID::is_enabled() const {
    return const_cast<HeaterPID*>(this)->pid.GetMode() == AUTOMATIC;
}

void HeaterPID::enable()
{
    this->pid.SetMode(AUTOMATIC);
}

void HeaterPID::disable()
{
    this->pid.SetMode(MANUAL);
}

HeaterPID &get_heater()
{
    static HeaterPID instance;
    return instance;
}
