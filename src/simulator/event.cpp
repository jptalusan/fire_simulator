// event.cpp
#include <iostream>
#include <spdlog/spdlog.h>
#include "enums.h"
#include "simulator/event.h"

Event::Event(EventType type, time_t time, std::shared_ptr<EventData> data, int id)
    : event_type(type), event_time(time), payload(std::move(data)), eventId(id) {}

void Event::print() const {
    spdlog::debug("Event Type: {}, Time: {}", static_cast<int>(event_type), event_time);
    if (payload) {
        payload->printInfo();
    } else {
        spdlog::debug("No payload");
    }
}

// TODO: This might be entirely unnecessary, but it is here for now.
FireStationEvent::FireStationEvent(const int& stationIndex, const int& incidentIndex, const int& enginesCount)
    : stationIndex(stationIndex), incidentIndex(incidentIndex), enginesCount(enginesCount) {}

void FireStationEvent::printInfo() const {
    spdlog::debug(", Station Index: {}, Incident Index: {}, Engines Count: {}", stationIndex, incidentIndex, enginesCount);
}


IncidentResolutionEvent::IncidentResolutionEvent(const int& incidentIndex)
    : incidentIndex(incidentIndex) {}

void IncidentResolutionEvent::printInfo() const {
    spdlog::debug("Incident Index: {}", incidentIndex);
}

// TODO: need to add an event id to all events.
void Event::updateEventTime(int id, time_t time) {

    return;
}
