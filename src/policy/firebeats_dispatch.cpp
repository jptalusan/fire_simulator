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
                                     const std::string& fireBeatsMatrixPath,
                                     const std::string& zoneIDToNameMapPath)
    : queries_(), distanceMatrix_(nullptr), durationMatrix_(nullptr), fireBeatsMatrix_(nullptr) {
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

        beatsIndexToNameMap_ = readZoneIndexToNameMapCSV(zoneIDToNameMapPath);
    }

FireBeatsDispatch::~FireBeatsDispatch() {
    delete[] durationMatrix_; // Clean up the matrix if it was allocated
    delete[] distanceMatrix_; // Clean up the matrix if it was allocated
    delete[] fireBeatsMatrix_; // Clean up the fire beats data if it was allocated
    spdlog::info("FireBeatsDispatch policy destroyed.");
}

// Relies on the preprocessed bin, if its not correct then the key suddenly has the string "Station" that means it failed.
int* FireBeatsDispatch::getFireBeats(const std::string& filename, int& height, int& width) const {
    // Open the file for reading
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file for reading: " << filename << "\n";
        return nullptr;
    }

    in.read(reinterpret_cast<char*>(&width), sizeof(int));
    in.read(reinterpret_cast<char*>(&height), sizeof(int));
    std::cout << "FireBeats matrix width: " << width << ", height: " << height << std::endl;
    if (width <= 0 || height <= 0 || width > 10000 || height > 10000) {
        std::cerr << "Invalid matrix dimensions!" << std::endl;
        return nullptr;
}

    // Allocate memory for the fire beats data
    int* fireBeatsData = new int[width * height];
    if (!fireBeatsData) {
        std::cerr << "Failed to allocate memory for fire beats data.\n";
        return nullptr;
    }

    // Read the fire beats data from the file
    in.read(reinterpret_cast<char*>(fireBeatsData), sizeof(int) * width * height);

    in.close();
    return fireBeatsData;
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
    int zoneIndex = i->zoneIndex;
    std::vector<int> beatStationIndices = getColumn(fireBeatsMatrix_, fireBeatsWidth_, fireBeatsHeight_, zoneIndex);
    // std::vector<int> beatStationIndices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; // Placeholder for actual beat station indices

    int totalApparatusRequired = i->totalApparatusRequired - i->currentApparatusCount;

    std::vector<Station> validStations = state.getAllStations();

    std::vector<Action> actions;
    actions.reserve(totalApparatusRequired);

    int totalApparatusDispatched = 0;
    for (const auto& index : beatStationIndices) {
        if (index < 0 || index >= static_cast<int>(validStations.size())) {
            spdlog::warn("Skipping invalid station index {} in beatStationIndices.", index);
            continue;
        }
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

std::vector<int> FireBeatsDispatch::getColumn(int* matrix, int width, int height, int col_index) const {
    std::vector<int> column;
    column.reserve(height); // optional: preallocate memory for performance
    for (int row = 0; row < height; ++row) {
        column.push_back(matrix[row * width + col_index]);
    }
    return column;
}

std::unordered_map<int, std::string> FireBeatsDispatch::readZoneIndexToNameMapCSV(const std::string& filename) const {
    std::unordered_map<int, std::string> zoneMap;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return zoneMap;
    }

    std::string line;
    // Skip the header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // Read ZoneID
        std::getline(ss, token, ',');
        int zoneID = std::stoi(token);

        // Read Zone Name
        std::getline(ss, token);
        std::string zoneName = token;

        zoneMap[zoneID] = zoneName;
    }

    return zoneMap;
}