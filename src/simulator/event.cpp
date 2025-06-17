// event.cpp
#include "enums.h"
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

// TODO: This might be entirely unnecessary, but it is here for now.
FireStationEvent::FireStationEvent(const int& stationIndex, const int& enginesCount)
    : stationIndex(stationIndex), enginesCount(enginesCount) {}

void FireStationEvent::printInfo() const {
    std::cout << ", Station Index: " << stationIndex << ", Engines Count: " << enginesCount << "\n";
}


IncidentResolutionEvent::IncidentResolutionEvent(const int& incidentID, const int& stationIndex)
    : incidentID(incidentID), stationIndex(stationIndex) {}

void IncidentResolutionEvent::printInfo() const {
    std::cout << "Incident ID: " << incidentID << ", Station Index: " << stationIndex << "\n";
}