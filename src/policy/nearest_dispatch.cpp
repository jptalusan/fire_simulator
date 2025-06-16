#include "policy/nearest_dispatch.h"
#include "services/queries.h"
#include <iostream>
#include "utils/error.hpp"
#include <spdlog/spdlog.h>

NearestDispatch::NearestDispatch(const std::string& osrmUrl)
    : osrmUrl_(osrmUrl), queries_() {
        // Validate the URL
        if (checkOSRM(osrmUrl_)) {
            spdlog::info("OSRM server is reachable and working correctly.");
        } else {
            spdlog::error("OSRM server is not reachable.");
            throw OSMError();
        }
    }

/**
 * @brief Determines the best station to dispatch to an unresolved incident.
 *
 * This function retrieves the next unresolved incident from the simulation state,
 * gathers the locations of all stations, and queries the OSRM service to obtain
 * travel times from each station to the incident location. It is designed to help
 * select the nearest available station for dispatching resources.
 *
 * Steps:
 * 1. Retrieve the next unresolved incident from the state.
 * 2. Collect all station locations from the state.
 * 3. Use the OSRM Table API (via Queries::queryTableService) to get travel times
 *    from all stations (sources) to the incident (destination).
 * 4. (Planned) Select the station with the lowest travel time and available trucks.
 * 5. (Planned) Emit a station_action event for dispatch.
 *
 * @param state The current simulation state, containing incidents and stations.
 * @return The incident ID of the unresolved incident (placeholder; will return station ID in future).
 */
int NearestDispatch::getAction(State& state) {
    // Get unresolved incident
    Incident i = state.getUnresolvedIncident();
    int incidentID = i.incident_id;
    Location incident_loc(i.lat, i.lon);

    // Get all station locations
    std::vector<Location> station_locs;
    auto& stations = state.getAllStations();
    for (const auto& station : stations) {
        station_locs.push_back(station.getLocation());
    }

    // Use Queries to get travel times from all stations to the incident
    std::vector<double> durations, distances;
    std::vector<Location> destinations = { incident_loc }; // single destination
    queries_.queryTableService(station_locs, destinations, durations, distances);

    int min_idx = findMinIndex(durations);
    spdlog::info("Nearest station to incident {} is {} with index {}", incidentID, stations[min_idx].getAddress(), min_idx);
    spdlog::info("Duration: {} seconds", (min_idx >= 0 ? durations[min_idx] : -1));
    // std::cout << "" << std::endl;
    // For now, just return the incident id (replace with actual dispatch logic)
    return i.incident_id;
    // 1. Loop through all stations in state
    // 2. For each, use OSRM table query to get travel time
    // 3. Pick the one with lowest travel time and has available trucks
    // 4. Create and emit a station_action event
}

std::shared_ptr<Incident> NearestDispatch::extractIncident(const Event& event) const {
    return std::dynamic_pointer_cast<Incident>(event.payload);
}

/**
 * @brief Finds the index of the minimum value in a vector of doubles.
 * @param durations The vector of durations.
 * @return The index of the smallest value, or -1 if the vector is empty.
 */
int NearestDispatch::findMinIndex(const std::vector<double>& durations) {
    if (durations.empty()) return -1;
    auto min_it = std::min_element(durations.begin(), durations.end());
    return static_cast<int>(std::distance(durations.begin(), min_it));
}