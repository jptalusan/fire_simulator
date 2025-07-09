#include "policy/dispatch_policy.h"
#include <spdlog/spdlog.h>
#include <map>

// TODO: This might be expensive, but we need to sort the incident by reportTime
int DispatchPolicy::getNextIncidentIndex(const State& state) const {
    const std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidentsConst();
    // Extra loop to sort the incidents by reportTime
    // This is necessary because we need to find the earliest unresolved incident
    // and we want to avoid modifying the original map structure.
    std::map<time_t, Incident> sortedIncidents;
    for (const auto& [id, incident] : activeIncidents) {
        sortedIncidents[incident.reportTime] = incident;
    }

    int incidentIndex = -1; // Initialize incident index
    const Incident* i = nullptr; // Pointer to the incident
    for (const auto& [reportTime, incident] : sortedIncidents) {
        int id = incident.incident_id; // Get the incident ID
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
std::vector<double> DispatchPolicy::getColumn(double* matrix, int width, int height, int col_index) const {
    std::vector<double> column;
    column.reserve(height); // optional: preallocate memory for performance

    for (int row = 0; row < height; ++row) {
        column.push_back(matrix[row * width + col_index]);
    }

    return column;
}

std::vector<int> DispatchPolicy::getColumn(int* matrix, int width, int height, int col_index) const {
    std::vector<int> column;
    column.reserve(height); // optional: preallocate memory for performance
    for (int row = 0; row < height; ++row) {
        column.push_back(matrix[row * width + col_index]);
    }
    return column;
}