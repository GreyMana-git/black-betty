#pragma once

#include <PID_v1.h>

/*
    Class for managing the heater pid
*/
class HeaterPID
{
public:
    HeaterPID();
    HeaterPID(const HeaterPID&) = delete;
    HeaterPID& operator=(const HeaterPID&) = delete;

    double get_kp() const;
    double get_ki() const;
    double get_kd() const;

    double get_input() const;
    double get_output() const;
    double get_setpoint() const;
    void set_setpoint(double setpoint);

    double get_window() const;
    void set_window(int window);

    void compute(double input);

    void configure(double kp, double ki, double kd);

    // Should the heating switch turned on?
    bool is_active() const;

    // Is the complete pid enabled oder disabled?
    bool is_enabled() const;
    void enable();
    void disable();

private:
    PID pid;
    double setpoint;
    double input;
    double output;
    int window;
    bool active;
};

HeaterPID& get_heater();
