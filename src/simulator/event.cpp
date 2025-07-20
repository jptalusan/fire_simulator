// event.cpp
#include <iostream>
#include <spdlog/spdlog.h>
#include "enums.h"
#include "simulator/event.h"
#include "utils/helpers.h"


void Event::print() const {
    std::cout << "[" << formatTime(event_time) << "] ";
    
    switch (event_type) {
        case EventType::Incident:
            std::cout << "Incident Event - IncidentIndex: " << incidentIndex;
            break;
            
        case EventType::ApparatusArrivalAtIncident:
            std::cout << "Apparatus Arrival - StationIndex: " << stationIndex 
                      << ", IncidentIndex: " << incidentIndex 
                      << ", EnginesCount: " << enginesCount;
            break;
            
        case EventType::ApparatusReturnToStation:
            std::cout << "Apparatus Return - StationIndex: " << stationIndex 
                      << ", IncidentIndex: " << incidentIndex 
                      << ", EnginesCount: " << enginesCount;
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
