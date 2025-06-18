#pragma once
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <string>
#include "simulator/event.h"
#include "data/incident.h"

// Really stupid, i should just fix simulator so i dont modify the vector while iterating over it.
inline void sortEventsByTimeAndType(std::vector<Event>& events) {
    std::sort(events.begin(), events.end(),
        [](const Event& a, const Event& b) {
            if (a.event_time != b.event_time)
                return a.event_time < b.event_time; // Primary sort

            if (a.event_type != b.event_type)
                return a.event_type < b.event_type; // Secondary sort

            // Tertiary sort for incidents (should not be too expensive)
            if (a.event_type == EventType::Incident) {
                auto a_ptr = static_cast<Incident*>(a.payload.get());
                auto b_ptr = static_cast<Incident*>(b.payload.get());
                return a_ptr->incident_id < b_ptr->incident_id;
            }

            return false; // Equal
        });
}

inline std::string formatTime(std::time_t t) {
    std::ostringstream oss;
    std::tm tm = *std::localtime(&t);
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}