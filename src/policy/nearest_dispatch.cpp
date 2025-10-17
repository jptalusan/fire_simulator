#include "policy/nearest_dispatch.h"
#include "services/queries.h"
#include "utils/error.h"
#include "utils/logger.h"
#include "utils/constants.h"
#include "services/chunks.h"
#include "objects/location.h"
#include "utils/helpers.h"

// TODO: This will become confusing, stationID and index are different.
NearestDispatch::NearestDispatch(const std::string& distanceMatrixPath, const std::string& durationMatrixPath)
    : distanceMatrix_(nullptr), durationMatrix_(nullptr) {
    // Validate the URL
    std::ifstream file(distanceMatrixPath);
    if (file) {
        distanceMatrix_ = load_matrix_binary_flat(distanceMatrixPath, height_, width_);
        durationMatrix_ = load_matrix_binary_flat(durationMatrixPath, height_, width_);
    } else {
        LOG_ERROR("File does not exist, defaulting to using OSRM Table API.");
        throw std::runtime_error("Distance matrix file not found: " + distanceMatrixPath);
    }
}

NearestDispatch::~NearestDispatch() {
    delete[] durationMatrix_; // Clean up the matrix if it was allocated
    delete[] distanceMatrix_; // Clean up the matrix if it was allocated
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
 * @note 3. (Planned) Select the station with the lowest travel time and available trucks.
 * @note 4. (Planned) Emit a station_action event for dispatch.
 *
 * @param state The current simulation state, containing incidents and stations.
 * @return The incident ID of the unresolved incident (placeholder; will return station ID in future).
 */
std::vector<Action> NearestDispatch::getAction(const State& state) const {
    int incidentIndex = getNextIncidentIndex(state);

    if (incidentIndex < 0) {
        LOG_DEBUG("No unresolved incident found in the active incidents.");
        return { Action::createDoNothingAction() }; // No action needed
    }

    const Incident& incident = state.getActiveIncidentsConst().at(incidentIndex);

    // If matrix is loaded, use it instead of OSRM
    std::vector<double> durations = getColumn(durationMatrix_, width_, height_, incidentIndex);
    std::vector<double> distances = getColumn(distanceMatrix_, width_, height_, incidentIndex);
    
    // int nearestStationIndex = findMinIndex(durations);
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durations);

    if (sortedIndices.empty()) {
        LOG_WARN("No valid stations found or all durations are infinite.");
    }
    
    return getAction_(incident, state, sortedIndices, durations);
}


const std::vector<Action> NearestDispatch::getAction2(const State& state) const {
    std::vector<Action> actions = {};

    // Maybe this is expensive
    std::vector<Location> fireStationLocations;
    std::vector<Location> emsVehicleLocations;
    std::vector<Vehicle> emsVehicles;

    Location incidentLocation;
    if (!state.newIncident_.has_value()) {
        return actions;
    }

    const Incident& incident = state.newIncident_.value();
    
    // incident.printInfo();
    for (const auto& [type, count] : incident.requiredApparatusMap) {
        LOG_INFO("POLICY: Incident {} ({}) requires {} of type {}", incident.incidentIndex, incident.incident_id, count, to_string(type));
    }

    for (const auto& station : state.getAllStations()) {
        fireStationLocations.push_back(station.getLocation());
        std::vector<int> _emsVehicleIds = station.getAvailableApparatus(ApparatusType::Medic);
        
        // Add vehicles from this station to the overall collection
        for (const auto& vehicleId : _emsVehicleIds) {
            // std::cout << "Found EMS Vehicle ID: " << vehicleId << " at Station: " << station.getStationId() << std::endl;
            const Vehicle& vehicle = state.getConstVehicleList().at(vehicleId);
            if (vehicle.getStatus() != ApparatusStatus::Available) {
                continue; // Skip non-available vehicles
            }
            emsVehicles.push_back(vehicle);
            emsVehicleLocations.push_back(vehicle.getCurrentLocation());
        }
    }
    std::vector<Action> fireActions = getFireVehiclesForIncident(state, incident, fireStationLocations);
    std::vector<Action> emsActions = getEMSForIncident(state, incident, emsVehicleLocations, emsVehicles);

    actions.insert(actions.end(), fireActions.begin(), fireActions.end());
    actions.insert(actions.end(), emsActions.begin(), emsActions.end());
    LOG_DEBUG("Total Actions dispatched: {}", actions.size());

    return actions;
}

// TODO: When are we considering an incident to be fully resolved? what if no dispatches are available? (then it wouldn't be taken as action)
std::vector<Action> NearestDispatch::getEMSForIncident([[maybe_unused]] const State& state, const Incident& incident, const std::vector<Location>& locations, const std::vector<Vehicle>& vehicles) const {
    Location incidentLocation = incident.getLocation();
    std::vector<Action> actions = {};
    std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> tableMatrix = 
            generate_osrm_table_chunks(locations, std::vector{incidentLocation});

    const std::vector<double> durationColumn = getColumn(tableMatrix.second, size_t(0));
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durationColumn);

    std::unordered_map<ApparatusType, int> remainingNeeded;
    for (const auto &[type, required] : incident.requiredApparatusMap) {
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
    for (const auto& [type, neededCount] : remainingNeeded) {
        int dispatchedCount = 0;

        for (int index : sortedIndices) {
            if (dispatchedCount >= neededCount) {
                break; // Already dispatched enough of this type
            }
            
            const Vehicle& vehicle = vehicles[index];
            if ((vehicle.getStatus() != ApparatusStatus::Available) && 
                (vehicle.getStatus() != ApparatusStatus::ReturningToStation)) {
                LOG_DEBUG("Skipping EMS Vehicle ID: {} as it is not available.", vehicle.getVehicleId());
                continue; // Skip non-available vehicles
            }
            if (vehicle.getType() == type) {
                // Dispatch this vehicle
                Action action = Action::createDispatchAction(vehicle.getStationIndex(),
                                                             incident.incidentIndex, 
                                                             vehicle.getVehicleId(), 
                                                             type, 1, durationColumn[index]);
                actions.push_back(action);
                dispatchedCount++;
            }
            if (dispatchedCount >= neededCount) {
                break; // Already dispatched enough of this type
            }
        }
    }
    LOG_DEBUG("Total EMS Actions dispatched: {}", actions.size());
    return actions;
}

std::vector<Action> NearestDispatch::getFireVehiclesForIncident(const State& state, const Incident& incident, const std::vector<Location>& locations) const {
    std::vector<Action> actions = {};
    Location incidentLocation = incident.getLocation();
    std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> b = 
        generate_osrm_table_chunks(locations, std::vector{incidentLocation});
    const std::vector<double> durationColumn = getColumn(b.second, size_t(0));
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durationColumn);
    
    std::unordered_map<ApparatusType, int> remainingNeeded;
    for (const auto &[type, required] : incident.requiredApparatusMap) {
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
    for (const auto& [type, neededCount] : remainingNeeded) {
        int dispatchedCount = 0;
        bool enoughDispatched = false;
        if (type == ApparatusType::Medic) {
            continue; // Skip EMS here
        }

        for (int index : sortedIndices) {
            
            const FireStation& station = state.getAllStations().at(index);

            std::vector<int> vehicleIds = station.getAvailableApparatus(type);
            // Add vehicles from this station to the overall collection
            for (const auto& vehicleId : vehicleIds) {
                const Vehicle& vehicle = state.getConstVehicleList().at(vehicleId);
                if (vehicle.getStatus() != ApparatusStatus::Available) {
                    continue; // Skip non-available vehicles
                }
                // Dispatch this vehicle
                Action action = Action::createDispatchAction(vehicle.getStationIndex(), 
                                                             incident.incidentIndex, 
                                                             vehicle.getVehicleId(),
                                                             type, 1, durationColumn[index]);
                actions.push_back(action);
                dispatchedCount++;
                if (dispatchedCount >= neededCount) {
                    enoughDispatched = true;
                    break; // Already dispatched enough of this type
                }
            }

            if (enoughDispatched) {
                break; // Already dispatched enough of this type
            }
        }
    }
    return actions;
}