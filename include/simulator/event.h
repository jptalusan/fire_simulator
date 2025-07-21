#ifndef EVENT_H
#define EVENT_H

#include <ctime>
#include "enums.h"
#include <queue>

class Event {
public:
    EventType event_type;
    time_t event_time;
    
    // All possible fields (unused fields will be -1 or 0)
    int stationIndex = -1;
    int incidentIndex = -1;
    int enginesCount = -1;

    // Default constructor
    Event() = default;
    
    // Generic constructor
    Event(EventType type, time_t time)
        : event_type(type), event_time(time) {}
    
    static Event createIncidentEvent(time_t time, int incidentIndex) {
        Event e(EventType::Incident, time);
        e.incidentIndex = incidentIndex;
        return e;
    }
    
    static Event createApparatusArrivalEvent(time_t time, int stationIndex, int incidentIndex, int enginesCount) {
        Event e(EventType::ApparatusArrivalAtIncident, time);
        e.stationIndex = stationIndex;
        e.incidentIndex = incidentIndex;
        e.enginesCount = enginesCount;
        return e;
    }

    static Event createApparatusReturnEvent(time_t time, int stationIndex, int incidentIndex, int enginesCount) {
        Event e(EventType::ApparatusReturnToStation, time);
        e.stationIndex = stationIndex;
        e.incidentIndex = incidentIndex;
        e.enginesCount = enginesCount;
        return e;
    }

    static Event createIncidentResolutionEvent(time_t time, int incidentIndex) {
        Event e(EventType::IncidentResolution, time);
        e.incidentIndex = incidentIndex;
        return e;
    }

    void print() const;
    
    bool operator>(const Event& other) const {
        if (event_time != other.event_time)
            return event_time > other.event_time;
        
        return event_type > other.event_type;
    }
};

using EventQueue = std::priority_queue<Event, std::vector<Event>, std::greater<Event>>;

#endif // EVENT_H