#include "policy/dispatch_policy.h"
#include <spdlog/spdlog.h>
#include <map>
#include <numeric>
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