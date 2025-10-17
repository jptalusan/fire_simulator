#include "environment/environment_model.h"
#include "models/fire_model.h"
#include "objects/incident.h"
#include "utils/constants.h"
#include "utils/helpers.h"
#include <fmt/format.h>
#include "utils/logger.h"

EnvironmentModel::EnvironmentModel(ServiceTimeAndApparatusModel& fireModel)
    : fireModel_(fireModel) {}

State& EnvironmentModel::takeActions(State& state, const std::vector<Action>& actions) {
    if (actions.empty() || actions[0].type == StationActionType::DoNothing) {
        return state;
    }
    
    Incident incident = state.newIncident_.value();  // This creates a modifiable copy
    
    double incidentResolutionTime = fireModel_.computeResolutionTime(state, incident);
    time_t currentTime = state.getSystemTime();
    // Update the vehicles
    for (const auto& action : actions) {
        if (action.type == StationActionType::Dispatch) {
            double travelTime = action.payload.travelTime;
            int vehicleIndex = action.payload.vehicleIndex;
            Vehicle& vehicle = state.getVehicleList().at(vehicleIndex);
            vehicle.setStatus(ApparatusStatus::Dispatched);
            time_t arrivalTime = currentTime + static_cast<time_t>(travelTime);
            vehicle.setTimeToIncident(arrivalTime);
            vehicle.timeStartedToDispatch = currentTime;
            // We don't know yet when the vehicle will return, set it when the vehicle actually arrives at the incident
            // vehicle.setTimeToReturn(currentTime + static_cast<time_t>(travelTime) + incidentResolutionTime + constants::RESPOND_DELAY_SECONDS);
            vehicle.setIncidentIndex(incident.incidentIndex);
            state.getVehicleList().at(vehicleIndex) = vehicle; // Update the vehicle in the state
            LOG_DEBUG("[{}] Dispatched {} Vehicle {} of {} to incident {}, will arrive at {}, resolution time: {:.2f} s", utils::formatTime(currentTime), to_string(vehicle.getType()),vehicle.getVehicleId(), vehicle.getStationId(), incident.incidentIndex, utils::formatTime(arrivalTime), incidentResolutionTime);

            // Update the station's available vehicle count
            FireStation& station = state.getStation(vehicle.getStationIndex());
            station.updateAvailableCount(vehicle.getType(), -1);
            state.getAllStations_().at(vehicle.getStationIndex()) = station; // Update the station in the state

            incident.currentApparatusMap[vehicle.getType()] += 1;
        }
    }
    // Update the incident
    LOG_DEBUG("[{}] Calculated incident resolution time: {}", utils::formatTime(currentTime), incidentResolutionTime);
    incident.timeRespondedTo = currentTime; // Set the time responded to current system time
    incident.status = IncidentStatus::hasBeenRespondedTo;
    incident.resolvedTime = currentTime + static_cast<time_t>(incidentResolutionTime) + constants::RESPOND_DELAY_SECONDS; // Set the resolved time for the incident
    state.getActiveIncidents().insert({incident.incidentIndex, incident}); // Add the incident to the active incidents map
    return state;
}
