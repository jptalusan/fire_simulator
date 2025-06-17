#include <iostream>
#include <spdlog/spdlog.h>
#include "policy/nearest_dispatch.h"
#include "services/queries.h"
#include "utils/error.h"
#include "utils/constants.h"
#include "services/chunks.h"

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
    delete[] distanceMatrix_; // Clean up the matrix if it was allocated
    spdlog::info("NearestDispatch policy destroyed.");
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
Action NearestDispatch::getAction(const State& state) {
    // Get unresolved incident
    Incident i = state.getEarliestUnresolvedIncident();
    int incidentID = i.incident_id;
    if (incidentID < 0) {
        spdlog::debug("No unresolved incident found.");
        return Action(StationActionType::DoNothing);
    }
    
    // Using OSRM Table API to get travel times
    // Location incidentLoc(i.lat, i.lon);

    // // Get all station locations
    // std::vector<Location> station_locs;
    // auto& stations = state.getAllStations();
    // for (const auto& station : stations) {
    //     station_locs.push_back(station.getLocation());
    // }

    // // Use Queries to get travel times from all stations to the incident
    // std::vector<double> durations, distances;
    // std::vector<Location> destinations = { incidentLoc }; // single destination
    // queries_.queryTableService(station_locs, destinations, durations, distances);

    // If matrix is loaded, use it instead of OSRM
    std::vector<double> durations = getColumn(durationMatrix_, width_, height_, incidentID);
    std::vector<double> distances = getColumn(distanceMatrix_, width_, height_, incidentID);
    
    Action dispatchAction = Action(StationActionType::DoNothing);
    // int nearestStationIndex = findMinIndex(durations);
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durations);

    if (sortedIndices.empty()) {
        spdlog::warn("No valid stations found or all durations are infinite.");
    }

    std::vector<Station> validStations = state.getAllStations();
    for (const auto& index : sortedIndices) {
        int numberOfFireTrucks = validStations[index].getNumFireTrucks();
        if (numberOfFireTrucks > 0) {
            spdlog::info("Nearest station to incident {} is {} with index {}, {} sec away.", incidentID, validStations[index].getAddress(), index, durations[index]);
            // spdlog::debug("Duration: {} seconds", (index >= 0 ? durations[index] : -1));

            dispatchAction = Action(StationActionType::Dispatch, {
                {constants::STATION_INDEX, std::to_string(index)},
                {constants::ENGINE_COUNT, "1"}, // Assuming we dispatch one engine
                {constants::INCIDENT_ID, std::to_string(incidentID)},
                {constants::DISPATCH_TIME, std::to_string(state.getSystemTime())},
                {constants::TRAVEL_TIME, std::to_string(durations[index])},
                {constants::DISTANCE, std::to_string(distances[index])} //DEBUG
            });
            break;
        } else {
            spdlog::warn("Station {} has no available fire trucks right now.", validStations[index].getAddress());
        }
    }
    
    return dispatchAction;
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

// TODO: Very expensive operation but only cheap because limited stations.
/**
 * @brief Returns a vector of (index, duration) pairs sorted by duration (smallest to largest).
 * @param durations The vector of durations.
 * @return A vector of pairs (index, duration), sorted by duration.
 */
std::vector<int> NearestDispatch::getSortedIndicesByDuration(const std::vector<double>& durations) {
    std::vector<std::pair<int, double>> indexed;
    for (int i = 0; i < static_cast<int>(durations.size()); ++i) {
        indexed.emplace_back(i, durations[i]);
    }
    std::sort(indexed.begin(), indexed.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    std::vector<int> indices;
    for (const auto& pair : indexed) {
        indices.push_back(pair.first);
    }
    return indices;
}

/*
* @brief Extracts a specific column from a 2D matrix represented as a flat array.
 * @param matrix The flat array representing the matrix.
 * @param width The number of columns in the matrix.
 * @param height The number of rows in the matrix.
 * @param col_index The index of the column to extract.
 * @return A vector containing the values of the specified column.
 * @note col_index represent the incidents.
 * @note each row represents a station.
*/
std::vector<double> NearestDispatch::getColumn(double* matrix, int width, int height, int col_index) const {
    std::vector<double> column;
    column.reserve(height); // optional: preallocate memory for performance

    for (int row = 0; row < height; ++row) {
        column.push_back(matrix[row * width + col_index]);
    }

    return column;
}