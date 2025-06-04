// event.cpp
#include "simulator/event.h"
#include <iostream>

Event::Event(EventType type, time_t time, std::shared_ptr<EventData> data)
    : event_type(type), event_time(time), payload(std::move(data)) {}

void Event::print() const {
    std::cout << "Event Type: " << static_cast<int>(event_type) << ", Time: " << event_time << "\n";
    if (payload) {
        payload->printInfo();
    } else {
        std::cout << "No payload\n";
    }
}
