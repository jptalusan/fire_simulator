#ifndef EVENT_H
#define EVENT_H

#include <ctime>
#include "enums.h"
#include <queue>
#include <iostream>
#include "utils/helpers.h"

class Event {
public:
    EventType event_type;
    time_t event_time;
    
    // All possible fields (unused fields will be -1 or 0)
    int stationIndex = -1;
    int incidentIndex = -1;
    int apparatusCount = -1;
    ApparatusType apparatusType = ApparatusType::Invalid; // Type of apparatus involved in the event
    std::vector<int> apparatusIds = {};
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
    
    static Event createApparatusArrivalEvent(time_t time, int stationIndex, int incidentIndex, int apparatusCount, ApparatusType type) {
        Event e(EventType::ApparatusArrivalAtIncident, time);
        e.stationIndex = stationIndex;
        e.incidentIndex = incidentIndex;
        e.apparatusCount = apparatusCount;
        e.apparatusType = type;
        return e;
    }

    static Event createApparatusReturnEvent(time_t time, int stationIndex, int incidentIndex, int apparatusCount, ApparatusType type,
                                            const std::vector<int>& apparatusIds = {}) {
        Event e(EventType::ApparatusReturnToStation, time);
        e.stationIndex = stationIndex;
        e.incidentIndex = incidentIndex;
        e.apparatusCount = apparatusCount;
        e.apparatusType = type;
        e.apparatusIds = apparatusIds;
        return e;
    }

    static Event createIncidentResolutionEvent(time_t time, int incidentIndex) {
        Event e(EventType::IncidentResolution, time);
        e.incidentIndex = incidentIndex;
        return e;
    }
    
    bool operator>(const Event& other) const {
        if (event_time != other.event_time)
            return event_time > other.event_time;
        
        return event_type > other.event_type;
    }

    void print() const {
        std::cout << "[" << utils::formatTime(event_time) << "] ";
        
        switch (event_type) {
            case EventType::Incident:
                std::cout << "Incident Event - IncidentIndex: " << incidentIndex;
                break;
                
            case EventType::ApparatusArrivalAtIncident:
                std::cout << "Apparatus Arrival - StationIndex: " << stationIndex 
                        << ", IncidentIndex: " << incidentIndex 
                        << ", ApparatusCount: " << apparatusCount
                        << ", ApparatusType: " << to_string(apparatusType);
                break;
                
            case EventType::ApparatusReturnToStation:
                std::cout << "Apparatus Return - StationIndex: " << stationIndex 
                        << ", IncidentIndex: " << incidentIndex 
                        << ", ApparatusCount: " << apparatusCount
                        << ", ApparatusType: " << to_string(apparatusType);
                break;
                
            case EventType::IncidentResolution:
                std::cout << "Incident Resolution - IncidentIndex: " << incidentIndex;
                break;
                
            default:
                std::cout << "Unknown Event Type";
                break;
        }
        
        std::cout << std::endl;
    }
};

using EventQueue = std::priority_queue<Event, std::vector<Event>, std::greater<Event>>;

#endif // EVENT_H