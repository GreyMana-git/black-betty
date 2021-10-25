#pragma once

constexpr int HISTORY_SIZE = 5;
constexpr int HISTORY_SLOT_TIME = 1000;

class SimpleTimer {
public:
    SimpleTimer(unsigned long window);
    SimpleTimer(const SimpleTimer &) = delete;
    SimpleTimer &operator=(const SimpleTimer &) = delete;

    bool next();

private:
    unsigned long window;
    unsigned long nextTick;
};

class StatCounter {
public:
    StatCounter();
    StatCounter(const StatCounter &) = delete;
    StatCounter &operator=(const StatCounter &) = delete;

    bool is_empty() const;
    void next(double value);
    void reset();

    double current;
    double min;
    double max;
    double sum;
};

enum HeaterMode { off, low, high };

struct StatusHistoryItem {
    StatusHistoryItem();
    StatusHistoryItem(const StatusHistoryItem &) = delete;
    StatusHistoryItem &operator=(const StatusHistoryItem &) = delete;

    int sequence;
    int samples;
    StatCounter temperature;
    StatCounter output;
    StatCounter heater;
    StatCounter health;
};

class Status {
public:
    Status();
    Status(const Status &) = delete;
    Status &operator=(const Status &) = delete;

    double temperature;
    HeaterMode heater_mode;
    unsigned long countdown_start;
    bool is_heater_toggle_active;

    SimpleTimer console_timer;
    SimpleTimer heater_timer;
    SimpleTimer webserver_timer;
    SimpleTimer display_timer;
    SimpleTimer alive_timer;

    bool display_needs_update(int temperature, bool heater_active);
    void update_history(double temperature, double output, bool heater, unsigned long healthtime);
    const StatusHistoryItem& get_history(int index) const;
    const char* get_heater_mode() const;
    void sendStatus() const;

    int update_countdown();

private:
    static int next_sequence;
    // Members for processing the history
    int history_index;
    unsigned long history_next_slot;
    StatusHistoryItem history_ringbuffer[HISTORY_SIZE];

    // Members for checking if the display has changed
    int display_temperature;
    bool display_heater_active;

};

Status& get_status();
