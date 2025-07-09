// event.h
#ifndef EVENT_H
#define EVENT_H

#include <memory>
#include <ctime>
#include "enums.h"

class EventData {
public:
    virtual ~EventData() = default;
    virtual void printInfo() const = 0;
};

// Payload is any class that inherits from EventData
class Event {
public:
    EventType event_type;
    time_t event_time;
    std::shared_ptr<EventData> payload;
    int eventId;

    Event(EventType type, time_t time, std::shared_ptr<EventData> data, int id = -1);
    void print() const;
    void updateEventTime(int id, time_t time);
};

class FireStationEvent : public EventData {
public:
    int stationIndex;
    int incidentIndex;
    int enginesCount; // Number of engines involved in the event

    FireStationEvent(const int& stationIndex, const int& incidentIndex, const int& enginesCount);
    void printInfo() const override;
};

class IncidentResolutionEvent : public EventData {
public:
    int incidentIndex;

    IncidentResolutionEvent(const int& incidentIndex);
    void printInfo() const override;
};

#endif // EVENT_H
