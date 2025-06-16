#include "policy/nearest_dispatch.h"
#include "services/queries.h"
#include <iostream>
#include "utils/error.hpp"

NearestDispatch::NearestDispatch(const std::string& osrmUrl)
    : osrmUrl_(osrmUrl) {
        // Validate the URL
        if (checkOSRM(osrmUrl_)) {
            std::cout << "OSRM server is reachable and working correctly." << std::endl;
        } else {
            throw OSMError();
        }
    }

int NearestDispatch::getAction(State& state) {
    // Get unresolved incidents
    Incident i = state.getUnresolvedIncident();
    return i.incident_id;
    // 1. Loop through all stations in state
    // 2. For each, use OSRM table query to get travel time
    // 3. Pick the one with lowest travel time and has available trucks
    // 4. Create and emit a station_action event
}

std::shared_ptr<Incident> NearestDispatch::extractIncident(const Event& event) const {
    return std::dynamic_pointer_cast<Incident>(event.payload);
}
