#pragma once
#include <Arduino.h>

struct Task {
    void     (*fn)();
    uint32_t  interval_ms;
    uint32_t  _last_ms;
};

class Scheduler {
public:
    Scheduler(Task* tasks, size_t count) : _tasks(tasks), _n(count) {}

    // Call at the end of setup() so all intervals start from that moment.
    void begin() {
        uint32_t now = millis();
        for (size_t i = 0; i < _n; i++)
            _tasks[i]._last_ms = now;
    }

    void run() {
        uint32_t now = millis();
        for (size_t i = 0; i < _n; i++) {
            if (now - _tasks[i]._last_ms >= _tasks[i].interval_ms) {
                _tasks[i].fn();
                _tasks[i]._last_ms = now;
            }
        }
    }

private:
    Task*  _tasks;
    size_t _n;
};
