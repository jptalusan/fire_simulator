#include "policy/nearest_dispatch.h"
#include <iostream>

NearestDispatch::NearestDispatch(const std::string& osrmUrl)
    : osrmUrl_(osrmUrl) {}

void NearestDispatch::dispatch(State& state, const Event& event) {
    auto incident = extractIncident(event);
    if (!incident) {
        std::cerr << "[Dispatch] Not an incident event. Skipping dispatch.\n";
        return;
    }

    // Placeholder: logic to find nearest station using OSRM
    std::cout << "[Dispatch] Handling dispatch for incident at "
              << incident->lat << ", " << incident->lon << "\n";

    // 1. Loop through all stations in state
    // 2. For each, use OSRM table query to get travel time
    // 3. Pick the one with lowest travel time and has available trucks
    // 4. Create and emit a station_action event
}

std::shared_ptr<Incident> NearestDispatch::extractIncident(const Event& event) const {
    return std::dynamic_pointer_cast<Incident>(event.payload);
}
