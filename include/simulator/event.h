// event.h
#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <memory>
#include <ctime>
#include "event_data.h"
#include "enums/event_type.h"

class Event {
public:
    EventType event_type;
    time_t event_time;
    std::shared_ptr<EventData> payload;

    Event(EventType type, time_t time, std::shared_ptr<EventData> data);
    void print() const;
};

#endif // EVENT_H