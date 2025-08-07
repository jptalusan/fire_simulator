#include "policy/dispatch_policy.h"
#include <map>
#include <numeric>
#include "utils/helpers.h"
#include "utils/constants.h"
#include "utils/logger.h"

// TODO: We dont need to sort, just place it in a priority queue, then pop it when its resolved.
int DispatchPolicy::getNextIncidentIndex(const State& state) const {
    const std::vector<int>& inProgressIncidents = state.inProgressIncidentIndices;

    // Having it in a priority queue, will stop this nonsense of redundant looping
    int incidentIndex = -1; // Initialize incident index
    const Incident* i = nullptr; // Pointer to the incident
    for (int incidentId : inProgressIncidents) {
        const Incident& incident = state.getActiveIncidentsConst().at(incidentId);
        int id = incident.incident_id; // Get the incident ID
        
        if (incident.resolvedTime <= state.getSystemTime()) {
            continue;
        }
        if (incident.getTotalApparatusRequired() > incident.getCurrentApparatusCount()) {
            LOG_DEBUG("Found unresolved incident with index: {}", id);
            incidentIndex = incident.incidentIndex; // Set the incidentIndex to the first unresolved incident found
            i = &incident; // Store the incident details as a pointer
            break;
        }
    }

    if (incidentIndex < 0 || i == nullptr) {
        LOG_DEBUG("No unresolved incident found in the active incidents.");
        return -1; // Return -1 if no unresolved incident is found
    }
    return incidentIndex;
}


/**
 * @brief Finds the index of the minimum value in a vector of doubles.
 * @param durations The vector of durations.
 * @return The index of the smallest value, or -1 if the vector is empty.
 */
int DispatchPolicy::findMinIndex(const std::vector<double>& durations) {
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
std::vector<int> DispatchPolicy::getSortedIndicesByDuration(const std::vector<double>& durations) {
    std::vector<int> indices(durations.size());
    std::iota(indices.begin(), indices.end(), 0);  // Fill with 0, 1, 2, ...
    
    std::sort(indices.begin(), indices.end(), 
              [&durations](int a, int b) { 
                  return durations[a] < durations[b]; 
              });
    
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
std::vector<double> DispatchPolicy::getColumn(double* matrix, int width, int height, int col_index) const {
    std::vector<double> column;
    column.reserve(height); // optional: preallocate memory for performance

    // Use pointer arithmetic for better performance
    double* col_ptr = matrix + col_index;
    for (int row = 0; row < height; ++row) {
        column.push_back(*col_ptr);
        col_ptr += width;
    }

    return column;
}

std::vector<int> DispatchPolicy::getColumn(int* matrix, int width, int height, int col_index) const {
    std::vector<int> column;
    column.reserve(height); // optional: preallocate memory for performance
    int* col_ptr = matrix + col_index;
    for (int row = 0; row < height; ++row) {
        column.push_back(*col_ptr);
        col_ptr += width;
    }
    return column;
}

// TODO: This is basically the same code as in nearest_dispatch and firebeats_dispatch, only difference is the stationOrder passed in.
std::vector<Action> DispatchPolicy::getAction_(const Incident &incident, const State &state,
                              const std::vector<int> &stationOrder,
                              const std::vector<double> &durations) {
    int incidentIndex = incident.incidentIndex;
    
    if (incidentIndex < 0) {
        LOG_DEBUG("No unresolved incident found in the active incidents.");
        return {Action::createDoNothingAction()}; // No action needed
    }
    const std::unordered_map<ApparatusType, int> &requiredApparatusMap =
        incident.requiredApparatusMap;
    if (requiredApparatusMap.empty()) {
        LOG_WARN("Incident {} has no required apparatus defined.",
                     incident.incident_id);
        return {Action::createDoNothingAction()}; // No action needed
    }

    // Calculate remaining apparatus needed by type
    std::unordered_map<ApparatusType, int> remainingNeeded;
    for (const auto &[type, required] : requiredApparatusMap) {
        int currentCount = 0;
        auto currentIt = incident.currentApparatusMap.find(type);
        if (currentIt != incident.currentApparatusMap.end()) {
            currentCount = currentIt->second;
        }

        int remaining = required - currentCount;
        if (remaining > 0) {
            remainingNeeded[type] = remaining;
        }
    }

    time_t incidentResolutionTime = incident.resolvedTime;
    const std::vector<Station> &validStations = state.getAllStations();

    std::vector<Action> actions;

    for (const auto& [type, neededCount] : remainingNeeded) {
        int dispatchedCount = 0;

        // Try each station in order of proximity
        for (int stationIndex : stationOrder) {
            if (dispatchedCount >= neededCount) {
                break; // Already dispatched enough of this type
            }

            const Station &station = validStations[stationIndex];
            int availableCount = station.getAvailableCount(type);
            if (availableCount <= 0) {
                LOG_DEBUG("Station {} has no available {} apparatus.",
                              station.getStationId(), to_string(type));
                continue; // No available apparatus of this type
            }

            // Check if station can reach incident in time
            time_t timeToReach = state.getSystemTime() +
                                 static_cast<time_t>(durations[stationIndex]);
            if (timeToReach >= incidentResolutionTime) {
                LOG_DEBUG("[{}] Station {} cannot reach incident {} in "
                              "time ({} seconds).",
                              utils::formatTime(state.getSystemTime()),
                              station.getStationId(), incidentIndex,
                              durations[stationIndex]);
                break; // No point in dispatching if it can't reach in time
            }

            // Calculate how many to dispatch
            int toDispatch = std::min({
                availableCount,               // What station has
                neededCount - dispatchedCount // What we still need
            });

            if (toDispatch > 0) {
                Action dispatchAction = Action::createDispatchAction(
                    station.getStationIndex(), incidentIndex, type, toDispatch,
                    durations[stationIndex]);

                actions.push_back(dispatchAction);
                dispatchedCount += toDispatch;
                LOG_DEBUG(
                    "[{}] Dispatching {} {} from station {} to incident {}, "
                    "{:.2f} minutes away.",
                    utils::formatTime(state.getSystemTime()), toDispatch,
                    to_string(type), station.getStationId(), incidentIndex,
                    durations[stationIndex] / constants::SECONDS_IN_MINUTE);
            }
        }

        // Log if we couldn't fulfill the requirement
        if (dispatchedCount < neededCount) {
            LOG_WARN(
                "Could only dispatch {} of {} required {} for incident {}",
                dispatchedCount, neededCount, to_string(type), incidentIndex);
        }
    }

    if (actions.empty()) {
        LOG_DEBUG("No actions could be taken for incident {}", incidentIndex);
        return {Action::createDoNothingAction()}; // No action could be taken
    }
    return actions;
}