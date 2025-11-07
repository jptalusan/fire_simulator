#include "policy/nearest_dispatch.h"
#include "enums.h"
#include "objects/common.h"
#include "utils/logger.h"
#include "services/chunks.h"
#include "objects/firestation.h"
#include <vector>

// TODO: This will become confusing, stationID and index are different.
NearestDispatch::NearestDispatch(std::vector<FireStation> fireStations,
                                   TravelTimeModel& travelTimeModel)
    : DispatchPolicy(std::move(fireStations), travelTimeModel) {
}

NearestDispatch::~NearestDispatch() {}

const std::vector<Action> NearestDispatch::getAction(const State& state) const {
    std::vector<Action> actions = {};

    // Maybe this is expensive
    std::vector<Location> emsVehicleLocations;
    std::vector<Vehicle> emsVehicles;

    if (!state.newIncident_.has_value()) {
        return actions;
    }

    const Incident& incident = state.newIncident_.value();

    for (const auto& station : fireStations_) {
        std::vector<int> _emsVehicleIds = station.getAvailableApparatus(ApparatusType::Medic);
        
        // Add vehicles from this station to the overall collection
        for (const auto& vehicleId : _emsVehicleIds) {
            // std::cout << "Found EMS Vehicle ID: " << vehicleId << " at Station: " << station.getStationId() << std::endl;
            const Vehicle& vehicle = state.getConstVehicleList().at(vehicleId);
            if ((vehicle.getStatus() != ApparatusStatus::Available) && (vehicle.getStatus() != ApparatusStatus::ReturningToStation)) {
                continue; // Skip non-available vehicles
            }
            emsVehicles.push_back(vehicle);
            emsVehicleLocations.push_back(vehicle.getCurrentLocation());
        }
    }
    std::unordered_map<ApparatusType, int> remainingNeeded = getRemainingApparatusNeeded(incident);

    std::vector<Action> fireActions = getFireVehiclesForIncident(state, incident, remainingNeeded, fireStationLocations_);
    std::vector<Action> emsActions = getEMSForIncident(state, incident, remainingNeeded, emsVehicleLocations, emsVehicles);
    actions.insert(actions.end(), fireActions.begin(), fireActions.end());
    actions.insert(actions.end(), emsActions.begin(), emsActions.end());
    LOG_DEBUG("Total Actions dispatched: {}", actions.size());

    return actions;
}

// TODO: When are we considering an incident to be fully resolved? what if no dispatches are available? (then it wouldn't be taken as action)
std::vector<Action> NearestDispatch::getEMSForIncident([[maybe_unused]]const State& state, 
    const Incident& incident, 
    const std::unordered_map<ApparatusType, int>& remainingNeeded,
    const std::vector<Location>& locations, 
    const std::vector<Vehicle>& vehicles) const {

    std::vector<Action> actions = {};

    if (locations.size() <= 0) {
        LOG_DEBUG("No EMS vehicle locations available for incident {}", incident.incidentIndex);
        return actions;
    }
    
    std::vector<std::vector<double>> tableMatrix = 
            travelTimeModel_.getTravelTimeMatrix(locations, std::vector{incident.getLocation()});

    const std::vector<double> durationColumn = getColumn(tableMatrix, size_t(0));
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durationColumn);
    
    for (const auto& [type, neededCount] : remainingNeeded) {
        int dispatchedCount = 0;
        if (type != ApparatusType::Medic) {
            continue; // Skip EMS here
        }
        for (int index : sortedIndices) {
            if (dispatchedCount >= neededCount) {
                break; // Already dispatched enough of this type
            }
            
            if (index < 0) {
                LOG_DEBUG("[{}] No more available {}, Dispatched {}, needed {}", utils::formatTime(state.getSystemTime()), to_string(type), dispatchedCount, neededCount);
                continue;
            }
            const Vehicle& vehicle = vehicles.at(index);
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

std::vector<Action> NearestDispatch::getFireVehiclesForIncident(const State& state, 
    const Incident& incident, 
    const std::unordered_map<ApparatusType, int>& remainingNeeded, 
    const std::vector<Location>& locations) const {

    std::vector<Action> actions = {};

    // TODO: Check if there are actually available vehicles of needed types

    std::vector<std::vector<double>> tableMatrix = 
        travelTimeModel_.getTravelTimeMatrix(locations, std::vector{incident.getLocation()});
    const std::vector<double> durationColumn = getColumn(tableMatrix, size_t(0));
    std::vector<int> sortedIndices = getSortedIndicesByDuration(durationColumn);
    
    for (const auto& [type, neededCount] : remainingNeeded) {
        int dispatchedCount = 0;
        bool enoughDispatched = false;
        if (type == ApparatusType::Medic) {
            continue; // Skip EMS here
        }

        for (int index : sortedIndices) {
            
            if (index < 0) {
                LOG_DEBUG("[{}] No more available {}, Dispatched {}, needed {}", utils::formatTime(state.getSystemTime()), to_string(type), dispatchedCount, neededCount);
                continue;
            }
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