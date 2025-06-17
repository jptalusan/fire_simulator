#pragma once
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <string>
#include "simulator/event.h"

// Sorts a vector of Event by event_time (ascending)
inline void sortEventsByTime(std::vector<Event>& events) {
    std::sort(events.begin(), events.end(),
        [](const Event& a, const Event& b) {
            return a.event_time < b.event_time;
        });
}

inline std::string formatTime(std::time_t t) {
    std::ostringstream oss;
    std::tm tm = *std::localtime(&t);
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}