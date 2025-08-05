#include <iostream>
#include <spdlog/spdlog.h>
#include "policy/nearest_dispatch.h"
#include "services/queries.h"
#include "utils/error.h"
#include "utils/constants.h"
#include "services/chunks.h"
#include "utils/helpers.h"

// TODO: This will become confusing, stationID and index are different.
NearestDispatch::NearestDispatch(const std::string& distanceMatrixPath, const std::string& durationMatrixPath)
    : queries_(), distanceMatrix_(nullptr), durationMatrix_(nullptr) {
        // Validate the URL
        std::ifstream file(distanceMatrixPath);
        if (file) {
            distanceMatrix_ = load_matrix_binary_flat(distanceMatrixPath, height_, width_);
            durationMatrix_ = load_matrix_binary_flat(durationMatrixPath, height_, width_);
        } else {
            spdlog::error("File does not exist, defaulting to using OSRM Table API.");
            throw std::runtime_error("Distance matrix file not found: " + distanceMatrixPath);
        }
    }

NearestDispatch::~NearestDispatch() {
    delete[] durationMatrix_; // Clean up the matrix if it was allocated
    delete[] distanceMatrix_; // Clean up the matrix if it was allocated
    spdlog::info("NearestDispatch policy destroyed.");
}

// TODO: This is very similar to firebeats except for a couple of lines.
 /**
 * @brief Determines the best station to dispatch to an unresolved incident.
 *
 * This function retrieves the next unresolved incident from the simulation state,
 * gathers the locations of all stations, and queries the OSRM service to obtain
 * travel times from each station to the incident location. It is designed to help
 * select the nearest available station for dispatching resources.
 *
 * Steps:
 * @note 1. Retrieve the next unresolved incident from the state.
 * @note 2. Collect all station locations from the state.
 * @note 3. Use the OSRM Table API (via Queries::queryTableService) to get travel times
 * @note    from all stations (sources) to the incident (destination).
 * @note 4. (Planned) Select the station with the lowest travel time and available trucks.
 * @note 5. (Planned) Emit a station_action event for dispatch.
 *
 * @param state The current simulation state, containing incidents and stations.
 * @return The incident ID of the unresolved incident (placeholder; will return station ID in future).
 */
std::vector<Action> NearestDispatch::getAction(const State& state) {
    int incidentIndex = getNextIncidentIndex(state);

    if (incidentIndex < 0) {
        spdlog::debug("No unresolved incident found in the active incidents.");
        return { Action::createDoNothingAction() }; // No action needed
    }

    const Incident& incident = state.getActiveIncidentsConst().at(incidentIndex);

    // If matrix is loaded, use it instead of OSRM
    std::vector<double> durations = getColumn(durationMatrix_, width_, height_, incidentIndex);
    std::vector<double> distances = getColumn(distanceMatrix_, width_, height_, incidentIndex);
    
    // int nearestStationIndex = findMinIndex(durations);
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durations);

    if (sortedIndices.empty()) {
        spdlog::warn("No valid stations found or all durations are infinite.");
    }
    
    return getAction_(incident, state, sortedIndices, durations);
}
