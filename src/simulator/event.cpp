// event.cpp
#include <iostream>
#include <spdlog/spdlog.h>
#include "enums.h"
#include "simulator/event.h"

Event::Event(EventType type, time_t time, std::shared_ptr<EventData> data)
    : event_type(type), event_time(time), payload(std::move(data)) {}

void Event::print() const {
    spdlog::debug("Event Type: {}, Time: {}", static_cast<int>(event_type), event_time);
    if (payload) {
        payload->printInfo();
    } else {
        spdlog::debug("No payload");
    }
}

// TODO: This might be entirely unnecessary, but it is here for now.
FireStationEvent::FireStationEvent(const int& stationIndex, const int& enginesCount)
    : stationIndex(stationIndex), enginesCount(enginesCount) {}

void FireStationEvent::printInfo() const {
    spdlog::debug(", Station Index: {}, Engines Count: {}", stationIndex, enginesCount);
}


IncidentResolutionEvent::IncidentResolutionEvent(const int& incidentIndex, const int& stationIndex)
    : incidentIndex(incidentIndex), stationIndex(stationIndex) {}

void IncidentResolutionEvent::printInfo() const {
    spdlog::debug("Incident Index: {}, Station Index: {}", incidentIndex, stationIndex);
}