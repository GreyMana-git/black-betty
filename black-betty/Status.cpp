#include "Status.h"

#include <Arduino.h>

#include "HeaterPID.h"

///////////////////////////////////////////////////////////////////////////////
// Simple Timer
SimpleTimer::SimpleTimer(unsigned long window) : window(window), nextTick(0) {
}

bool SimpleTimer::next() {
    unsigned long now = millis();
    if (now < this->nextTick) {
        return false;
    }

    if (this->nextTick == 0) {
        this->nextTick = now + this->window;
        return true;
    }

    do {
        this->nextTick += this->window;
    } while (now >= this->nextTick);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Stat Counter
StatCounter::StatCounter() : current(0.0), min(1.0), max(-1.0), sum(0.0) {
}

bool StatCounter::is_empty() const {
    return this->min == 1.0 && this->max == -1.0;
}

void StatCounter::next(double value) {
    this->current = value;
    if (this->is_empty()) {
        this->min = value;
        this->max = value;
        this->sum = value;
    } else {
        if (value < min) { this->min = value; }
        if (value > max) { this->max = value; }
        this->sum += value;
    }
}

void StatCounter::reset() {
    this->min = 1.0;
    this->max = -1.0;
}

///////////////////////////////////////////////////////////////////////////////
// StatusHistory
StatusHistoryItem::StatusHistoryItem() : samples(0) {
}

///////////////////////////////////////////////////////////////////////////////
// Status
Status::Status() :  
        temperature(0.0),
        heater_mode(HeaterMode::off),
        display_temperature(0),
        display_heater_active(false),
        history_index(0),
        history_next_slot(millis() + HISTORY_SLOT_TIME),
 
        console_timer(15),
        heater_timer(25),
        webserver_timer(50),
        display_timer(30),
        alive_timer(10000) {
}

int Status::next_sequence = 0;

bool Status::display_needs_update(int temperature, bool heater_active) {
    if (temperature == this->display_temperature && heater_active == this->display_heater_active) {
        return false;
    }

    this->display_temperature = temperature;
    this->display_heater_active = heater_active;
    return true;
}

void Status::update_history(double temperature, double output, bool heater, unsigned long healthtime) {
    unsigned long now = millis();

    // Initialize next slot on first call
    if (this->history_next_slot == 0) {
        this->history_next_slot = now - 1;
    }

    // Calculate the current index of the ringbuffer and reset the data if it changes
    while (this->history_next_slot < now) {
      this->history_next_slot += HISTORY_SLOT_TIME;
        this->history_index = (this->history_index + 1) % HISTORY_SIZE;
        StatusHistoryItem& item = this->history_ringbuffer[this->history_index];
        item.temperature.reset();
        item.output.reset();
        item.heater.reset();
        item.health.reset();
        item.samples = 0;
        item.sequence = (Status::next_sequence++);
    }

    StatusHistoryItem& item = this->history_ringbuffer[this->history_index];
    item.temperature.next(temperature);
    item.output.next(output);
    item.heater.next(heater ? 1.0 : 0.0);
    item.health.next(static_cast<double>(healthtime));
    item.samples++;
}

const StatusHistoryItem& Status::get_history(int index) const {
    return this->history_ringbuffer[(HISTORY_SIZE + this->history_index - index) % HISTORY_SIZE];
}

const char* Status::get_heater_mode() const {
    switch (this->heater_mode) {
        case HeaterMode::off: return "off";
        case HeaterMode::low: return "low";
        case HeaterMode::high: return "high";
    }

    return "invalid";
}

void Status::sendStatus() const {
    const HeaterPID& heater = get_heater();
    
    Serial.print("kp: "); Serial.print(heater.get_kp());
    Serial.print(" ki: "); Serial.print(heater.get_ki());
    Serial.print(" kd: "); Serial.print(heater.get_kd());
    Serial.print(" input: "); Serial.print(heater.get_input());
    Serial.print(" output: "); Serial.print(heater.get_output());
    Serial.print(" setpoint: "); Serial.print(heater.get_setpoint());
    Serial.print(" heater: "); Serial.print(heater.is_active() ? "on" : "off");
    Serial.print(" mode: "); Serial.print(this->get_heater_mode());
    Serial.println("");
}

Status& get_status() {
    static Status instance;
    return instance;
}
