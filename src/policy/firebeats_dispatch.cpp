#include <iostream>
#include <spdlog/spdlog.h>
#include "policy/firebeats_dispatch.h"
#include "services/queries.h"
#include "utils/error.h"
#include "utils/constants.h"
#include "services/chunks.h"
#include "utils/helpers.h"

// Firebeats naming convention is a bit confusing. and currently this is incomplete.
// Check the preprocess notebook for a list of beats that I have no idea what they mean (DSOP,BAR,HQ, etc.)
FireBeatsDispatch::FireBeatsDispatch(const std::string& distanceMatrixPath, 
                                     const std::string& durationMatrixPath,
                                     const std::string& fireBeatsMatrixPath)
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

        std::ifstream fireBeatsFile(fireBeatsMatrixPath);
        if (fireBeatsFile) {
            fireBeatsMatrix_ = getFireBeats(fireBeatsMatrixPath, fireBeatsHeight_, fireBeatsWidth_);
            spdlog::info("FireBeats matrix loaded with dimensions: {}x{}", fireBeatsWidth_, fireBeatsHeight_);
        } else {
            spdlog::error("FireBeats matrix file not found: {}", fireBeatsMatrixPath);
            throw std::runtime_error("FireBeats matrix file not found: " + fireBeatsMatrixPath);
        }
    }

FireBeatsDispatch::~FireBeatsDispatch() {
    delete[] durationMatrix_; // Clean up the matrix if it was allocated
    delete[] distanceMatrix_; // Clean up the matrix if it was allocated
    spdlog::info("FireBeatsDispatch policy destroyed.");
}

// Relies on the preprocessed bin, if its not correct then the key suddenly has the string "Station" that means it failed.
std::unordered_map<std::string, std::vector<std::string>> FireBeatsDispatch::getFireBeats(const std::string& filename, int& height, int& width) const {

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file for reading: " << filename << "\n";
        return {};
    }

    in.read(reinterpret_cast<char*>(&width), sizeof(int));
    in.read(reinterpret_cast<char*>(&height), sizeof(int));

    std::unordered_map<std::string, std::vector<std::string>> result;

    for (int row = 0; row < height; ++row) {
        std::string key;
        std::vector<std::string> values;

        for (int col = 0; col < width; ++col) {
            int len = 0;
            in.read(reinterpret_cast<char*>(&len), sizeof(int));

            if (len <= 0) {
                std::cerr << "Invalid string length at row " << row << ", col " << col << "\n";
                continue;
            }

            std::string str(len, '\0');
            in.read(&str[0], len);

            if (col == 0) {
                if (str.find("Station") != std::string::npos) {
                    spdlog::error("Key contains 'Station', indicating a potential error in the FireBeats matrix.");
                    throw std::runtime_error("Invalid key found in FireBeats matrix: " + str);
                }
                key = std::move(str);

            } else {
                values.push_back(std::move(str));
            }
        }

        result[key] = std::move(values);
    }

    in.close();
    return result;
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
std::vector<Action> FireBeatsDispatch::getAction(const State& state) {
    const std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidentsConst();
    int incidentIndex = -1; // Initialize incident index
    const Incident* i = nullptr; // Pointer to the incident
    for (const auto& [id, incident] : activeIncidents) {
        if (state.resolvingIncidentIndex_.count(incident.incidentIndex) > 0) {
            continue;
        }
        if (incident.resolvedTime <= state.getSystemTime()) {
            continue;
        }
        if (incident.totalApparatusRequired > incident.currentApparatusCount) {
            spdlog::debug("Found unresolved incident with index: {}", id);
            incidentIndex = incident.incidentIndex; // Set the incidentIndex to the first unresolved incident found
            i = &incident; // Store the incident details as a pointer
            break;
        }
    }

    if (incidentIndex < 0 || i == nullptr) {
        spdlog::debug("No unresolved incident found in the active incidents.");
        return { Action(StationActionType::DoNothing) }; // No action needed
    }
    
    // If matrix is loaded, use it instead of OSRM
    std::vector<double> durations = getColumn(durationMatrix_, width_, height_, incidentIndex);
    std::vector<double> distances = getColumn(distanceMatrix_, width_, height_, incidentIndex);
    
    Action dispatchAction = Action(StationActionType::DoNothing);

    std::string zone = i->zone.empty() ? "Unknown" : i->zone;
    std::vector<std::string> beats = fireBeatsMatrix_.count(zone) > 0 ? fireBeatsMatrix_[zone] : std::vector<std::string>{};
    
    std::vector<int> beatIndices;
    beatIndices.reserve(beats.size());

    for (const auto& key : beats) {
        auto it = state.stationIndexMap_.find(key);
        if (it != state.stationIndexMap_.end()) {
            beatIndices.push_back(it->second);
        } else {
            // Optional: handle missing key (e.g. push_back a default or error code)
            if (key != "None") {
                spdlog::error("Key not found: {}", key);
            }
        }
    }

    int totalApparatusRequired = i->totalApparatusRequired - i->currentApparatusCount;

    std::vector<Station> validStations = state.getAllStations();

    std::vector<Action> actions;
    actions.reserve(totalApparatusRequired);

    int totalApparatusDispatched = 0;
    for (const auto& index : beatIndices) {
        if (totalApparatusDispatched == totalApparatusRequired)
            break;

        int numberOfFireTrucks = validStations[index].getNumFireTrucks();
        if (numberOfFireTrucks > 0) {
            // spdlog::debug("Duration: {} seconds", (index >= 0 ? durations[index] : -1));
            int usedApparatusCount = 0;
            dispatchAction = Action(StationActionType::Dispatch, {
                {constants::STATION_INDEX, std::to_string(index)},
                {constants::INCIDENT_INDEX, std::to_string(incidentIndex)},
                {constants::DISPATCH_TIME, std::to_string(state.getSystemTime())},
                {constants::TRAVEL_TIME, std::to_string(durations[index])},
                {constants::DISTANCE, std::to_string(distances[index])} //DEBUG
            });
            if ((totalApparatusRequired - totalApparatusDispatched) >= numberOfFireTrucks) {
                usedApparatusCount = numberOfFireTrucks;
            } else {
                usedApparatusCount = totalApparatusRequired - totalApparatusDispatched;
            }
            dispatchAction.payload[constants::ENGINE_COUNT] = std::to_string(usedApparatusCount);
            totalApparatusDispatched += usedApparatusCount;
            actions.push_back(dispatchAction);
            spdlog::info("[{}] Dispatching {} engines from {} to incident {}, {:.2f} minutes away.", 
                formatTime(state.getSystemTime()), 
                usedApparatusCount,
                validStations[index].getAddress(),
                incidentIndex,
                durations[index] / constants::SECONDS_IN_MINUTE);
        } else {
            spdlog::warn("[{}] Station {} has no available fire trucks right now.", formatTime(state.getSystemTime()),  validStations[index].getAddress());
        }
    }
    
    return actions;
}

/**
 * @brief Finds the index of the minimum value in a vector of doubles.
 * @param durations The vector of durations.
 * @return The index of the smallest value, or -1 if the vector is empty.
 */
int FireBeatsDispatch::findMinIndex(const std::vector<double>& durations) {
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
std::vector<int> FireBeatsDispatch::getSortedIndicesByDuration(const std::vector<double>& durations) {
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
std::vector<double> FireBeatsDispatch::getColumn(double* matrix, int width, int height, int col_index) const {
    std::vector<double> column;
    column.reserve(height); // optional: preallocate memory for performance

    for (int row = 0; row < height; ++row) {
        column.push_back(matrix[row * width + col_index]);
    }

    return column;
}