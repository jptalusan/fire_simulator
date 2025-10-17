#include "simulator/simulator.h"
#include "simulator/action.h"
#include "utils/helpers.h"
#include "utils/logger.h"
#include "services/chunks.h"

std::vector<ApparatusType> apparatusTypes = {
    ApparatusType::Engine,
    ApparatusType::Truck,
    ApparatusType::Rescue,
    ApparatusType::Hazard,
    ApparatusType::Squad,
    ApparatusType::Fast,
    ApparatusType::Medic,
    ApparatusType::Brush,
    ApparatusType::Boat,
    ApparatusType::UTV,
    ApparatusType::Reach,
    ApparatusType::Chief
};

Simulator::Simulator(State &initialState, 
                     IncidentModel &incidentModel,
                     EnvironmentModel &environmentModel,
                     DispatchPolicy &dispatchPolicy)
    : state_(initialState), environment_(environmentModel),
      dispatchPolicy_(dispatchPolicy), incidentModel_(incidentModel) {
}

StepResult Simulator::step(const std::vector<Action>& actions) {
    LOG_INFO("[{}] Taking {} actions.", utils::formatTime(state_.getSystemTime()), actions.size());
    // Take the actions from the policy and update the environment
    state_ = environment_.takeActions(state_, actions);

    // Get the next incident
    const std::optional<Incident> nextIncident = incidentModel_.getNextIncident(state_, state_.getSystemTime());
    
    if (!nextIncident.has_value()) {
        LOG_WARN("No more incidents available");
        return StepResult(state_, 0.0, true, {{"info", true}});
    }
    
    state_.newIncident_ = nextIncident.value();
    time_t nextIncidentTime = nextIncident.value().reportTime;

    state_ = simulate_time_step(nextIncidentTime);
    state_.advanceTime(nextIncidentTime);

    LOG_INFO("[{}] Incident {} is reported.", utils::formatTime(state_.getSystemTime()), nextIncident.value().incidentIndex);
    return StepResult(state_, 0.0, false, {});
}

State& Simulator::simulate_time_step(time_t end_time) {
    // LOG_ERROR("[{}] Advancing simulation to time: {}", utils::formatTime(state_.getSystemTime()), utils::formatTime(end_time));

    time_t current_time = state_.getSystemTime();
    std::vector<Vehicle>& vehicles = state_.getVehicleList();
    std::unordered_map<int, Incident>& incidentsMap = state_.getActiveIncidents();
    LOG_DEBUG("[{}] Moving forward by: {} seconds", utils::formatTime(current_time), difftime(end_time, current_time));
    for (auto& vehicle : vehicles) {
        time_t sim_time = current_time;
        Location sim_location = vehicle.getCurrentLocation();
        while (sim_time < end_time) {
            // Update vehicle status based on time
            // std::cout << to_string(vehicle.getStatus()) << "\n";
            switch (vehicle.getStatus()) {
                case ApparatusStatus::Available:
                    sim_time = end_time;
                    break; // No update needed for available vehicles
                case ApparatusStatus::Dispatched: {
                    int incidentIndex = vehicle.getIncidentIndex();
                    Incident& incident = incidentsMap.at(incidentIndex);
                    
                    // If it will take longer than end_time to reach the incident, calculate the intermediate location
                    if (vehicle.getTimeToIncident() > end_time) {
                        // sim_location = utils::calculateIntermediateLocation(vehicle.getLocation(), incident.getLocation(), end_time - current_time);
                        sim_time = end_time;
                        time_t _traveledTime = difftime(sim_time, vehicle.timeStartedToDispatch);
                        time_t _totalTime = difftime(vehicle.getTimeToIncident(), vehicle.timeStartedToDispatch);
                        double percentTraveled = static_cast<double>(_traveledTime) / static_cast<double>(_totalTime);
                        LOG_INFO("[{}] Vehicle {} is en route to incident: {}, traveled {:.2f}%, will arrive by: {}", utils::formatTime(sim_time), vehicle.getVehicleId(), incidentIndex, percentTraveled * 100.0, utils::formatTime(vehicle.getTimeToIncident()));
                        std::cout << std::endl;
                    // Vehicle already arrived at the incident
                    } else if (vehicle.getTimeToIncident() <= end_time) {
                        vehicle.setStatus(ApparatusStatus::AtIncident);
                        vehicle.setCurrentLocation(incident.getLocation());
                        sim_time = vehicle.getTimeToIncident();
                        LOG_INFO("[{}] Vehicle {} has arrived at from ({}) incident: {} at ({})", utils::formatTime(sim_time), vehicle.getVehicleId(), locationToString(sim_location), incidentIndex, locationToString(incident.getLocation()));
                        vehicle.timeStartedToDispatch = 0;
                        vehicle.timeToStartedReturning = 0; // Resetting as we don't need it until next return
                    } else {
                        sim_time = end_time;
                    }
                    break;
                }
                case ApparatusStatus::AtIncident: {
                    int incidentIndex = vehicle.getIncidentIndex();
                    Incident& incident = incidentsMap.at(incidentIndex);
                    time_t timeToResolve = incident.resolvedTime;
                    if (timeToResolve > end_time) {
                        sim_time = end_time;
                        incident.status = IncidentStatus::isBeingResolved;
                        LOG_INFO("[{}] Vehicle {} is at the incident, resolving...", utils::formatTime(sim_time), vehicle.getVehicleId());
                        state_.getActiveIncidents().at(incidentIndex) = incident; // Update the incident in the active incidents map
                    } else if (timeToResolve <= end_time) {
                        sim_time = timeToResolve;
                        // Incident has been resolved, vehicle is returning to station
                        vehicle.setStatus(ApparatusStatus::ReturningToStation);
                        incident.status = IncidentStatus::hasBeenResolved;
                        // Calculate vehicle travel time back to station...
                        std::pair<float, std::vector<double>> routeInfo = generate_route(sim_location, vehicle.getStationLocation());
                        float timeToReturn = routeInfo.first; // in seconds
                        vehicle.setTimeToReturn(sim_time + static_cast<time_t>(timeToReturn));
                        vehicle.setIncidentIndex(-1); // Clear incident index as vehicle is leaving
                        vehicle.timeToStartedReturning = sim_time;
                        LOG_INFO("[{}] Vehicle {} is done and returning to {} by {}", utils::formatTime(sim_time), vehicle.getVehicleId(), vehicle.getStationId(), utils::formatTime(vehicle.getTimeToReturn()));
                        // state_.getActiveIncidents().at(incidentIndex) = incident; // Update the incident in the active incidents map
                        state_.doneIncidents_.insert({incidentIndex, incident});
                        // state_.getActiveIncidents().erase(incidentIndex); // Remove the incident from active incidents
                    } else {
                        sim_time = end_time;
                    }
                    break;
                }
                case ApparatusStatus::ReturningToStation: {
                    // Vehicle takes longer than end_time to return to station
                    if (vehicle.getTimeToReturn() > end_time) {
                        sim_time = end_time;
                        time_t _traveledTime = difftime(sim_time, vehicle.timeToStartedReturning);
                        time_t _totalTime = difftime(vehicle.getTimeToReturn(), vehicle.timeToStartedReturning);
                        double percentTraveled = static_cast<double>(_traveledTime) / static_cast<double>(_totalTime);
                        LOG_INFO("[{}] Vehicle {} is returning to {}, traveled {:.2f}%", utils::formatTime(sim_time), vehicle.getVehicleId(), vehicle.getStationId(), percentTraveled * 100.0);
                        std::pair<float, std::vector<double>> routeInfo = generate_route(sim_location, vehicle.getStationLocation());
                        // Divide by 2 because its a flat array of [lon, lat, lon, lat, ...]
                        size_t routeSize = static_cast<size_t>(routeInfo.second.size() / 2);
                        if (routeSize >= 2) {
                            size_t index = static_cast<size_t>(percentTraveled * (routeSize - 1));
                            if (index >= routeSize) index = routeSize - 1;
                            double lat = routeInfo.second[index * 2];
                            double lon = routeInfo.second[index * 2 + 1];
                            LOG_INFO("[{}] Vehicle {} current location updated from ({}) to ({}, {})", utils::formatTime(sim_time), vehicle.getVehicleId(), locationToString(sim_location), lat, lon);
                            vehicle.setCurrentLocation(Location(lat, lon));
                        }
                    // Vehicle has returned to station
                    } else if (vehicle.getTimeToReturn() <= end_time) {
                        sim_time = vehicle.getTimeToReturn();
                        vehicle.setStatus(ApparatusStatus::Available);
                        vehicle.setCurrentLocation(vehicle.getStationLocation());

                        // Update the station's available vehicle count (How should EMS handle this?)
                        FireStation& station = state_.getStation(vehicle.getStationIndex());
                        station.updateAvailableCount(vehicle.getType(), 1);
                        state_.getAllStations_().at(vehicle.getStationIndex()) = station; // Update the station in the state
                        
                        LOG_INFO("[{}] Vehicle {} has returned to station and is now available", utils::formatTime(sim_time), vehicle.getVehicleId());
                        vehicle.timeToStartedReturning = 0; // Resetting as we don't need it until next return
                    } else {
                        sim_time = end_time;
                    }
                    break;
                }
                default:
                    sim_time = end_time;
                    break;
            }
        }
        int vehicleIndex = vehicle.getVehicleId();
        state_.getVehicleList().at(vehicleIndex) = vehicle; // Update the vehicle in the state
    }
    return state_;
}

// void Simulator::getIntermediateLocation() {
// }

State& Simulator::reset() {
    state_.advanceTime(0); // Reset time to 0 or initial time
    state_.getActiveIncidents().clear();
    std::optional<Incident> incident = incidentModel_.getNextIncident(state_, 0);
    
    // This will throw std::bad_optional_access if incident is nullopt
    const Incident& incidentRef = incident.value();
    
    state_.newIncident_ = incidentRef;  // Store by value
    state_.advanceTime(incidentRef.reportTime);

    LOG_INFO("[{}] Simulation reset, incident {} is reported.", utils::formatTime(state_.getSystemTime()), incidentRef.incidentIndex);
    state_history_.clear();
    station_history_.clear();
    action_history_.clear();
    return state_;
}

// const std::vector<State>& Simulator::getStateHistory() const {
//   return state_history_;
// }

// const std::vector<Action>& Simulator::getActionHistory() const {
//   return action_history_;
// }

// State& Simulator::getCurrentState() { return state_; }

// void Simulator::writeReportToCSV() {
//   //All required apparatus counts and all recieved appartus counts
//     std::unordered_map<int, Incident>& activeIncidents = state_.getActiveIncidents();
//     std::unordered_map<int, Incident>& doneIncidents = state_.doneIncidents_;

//     // Insert all elements from doneIncidents into activeIncidents
//     activeIncidents.insert(doneIncidents.begin(), doneIncidents.end()); // Existing keys in activeIncidents are NOT overwritten

//     std::string report_path = EnvLoader::getInstance()->get("REPORT_CSV_PATH", "../logs/incident_report.csv");

//     std::ofstream csv(report_path);

//     // Write header
//     csv << "IncidentIndex,IncidentID,Reported,Responded,Resolved";
//     for (const auto& type : apparatusTypes) {
//         csv << "," << to_string(type) << "Required," << to_string(type) << "Received";
//     }
//     csv << ",Zone,Status\n";
//     std::vector<Incident> sortedIncidents;
//     sortedIncidents.reserve(activeIncidents.size());  // Preallocate memory for efficiency
//     for (const auto& [id, incident] : activeIncidents) {
//         sortedIncidents.emplace_back(incident);
//     }
//     std::sort(sortedIncidents.begin(), sortedIncidents.end(),
//         [](const Incident& a, const Incident& b) {
//             return a.reportTime < b.reportTime;
//         });

//     for (size_t i = 0; i < sortedIncidents.size(); ++i) {
//         const auto& incident = sortedIncidents[i];
//         if (incident.resolvedTime < 0 || incident.resolvedTime > 2147483647) {
//             LOG_ERROR("Incident {} has a resolved time out of bounds: {}", incident.incidentIndex, incident.resolvedTime);
//             continue; // Skip this incident
//         }
//         // TODO: Fix apparatus count and add type.
//         csv << std::fixed << std::setprecision(6);
//         csv << incident.incidentIndex << ","
//             << incident.incident_id << ","
//             << utils::formatTime(incident.reportTime) << ","
//             << utils::formatTime(incident.timeRespondedTo) << ","
//             << utils::formatTime(incident.resolvedTime);

//          // Output required and received for each apparatus type
//          for (const auto& type : apparatusTypes) {
//              int required = 0;
//              int received = 0;
//              auto reqIt = incident.requiredApparatusMap.find(type);
//              if (reqIt != incident.requiredApparatusMap.end()) required = reqIt->second;
//              auto recIt = incident.currentApparatusMap.find(type);
//              if (recIt != incident.currentApparatusMap.end()) received = recIt->second;
//              csv << "," << required << "," << received;
//          }
//          csv << "," << incident.zoneIndex << "," << to_string(incident.status) << "\n";
//     }
//     csv.close();
// }

// void Simulator::writeActions() {
//   //TODO: Remaining appaaratus count for the station and what they dispatched and how many they dispatched for the incident.
//     std::string station_report_path = EnvLoader::getInstance()->get("STATION_REPORT_CSV_PATH", "../logs/station_report.csv");

//     std::vector<Action> actionHistory = getActionHistory();

//     std::unordered_map<int, Incident>& activeIncidents = state_.getActiveIncidents();
//     std::unordered_map<int, Incident>& doneIncidents = state_.doneIncidents_;

//     // Insert all elements from doneIncidents into activeIncidents
//     activeIncidents.insert(doneIncidents.begin(), doneIncidents.end()); // Existing keys in activeIncidents are NOT overwritten

//     std::ofstream station_csv(station_report_path);

//     station_csv << "DispatchTime,StationID,StationName";
//     for (const auto& type : apparatusTypes) {
//         station_csv << "," << to_string(type) << "Dispatched," << to_string(type) << "Remaining";
//         }
//     station_csv << ",TravelTime,IncidentIndex,IncidentID\n";

//     for (size_t i = 0; i < actionHistory.size(); ++i) {
//         const auto& action = actionHistory[i];
//         if (action.type != StationActionType::Dispatch) continue;
//         std::string metrics;
//         metrics.reserve(256)
//         ;
//         // Get the relevant station snapshot at the time of dispatch
//         const FireStation& station = station_history_[i];


//         // Get the incident for dispatch time
//         const Incident& incident = activeIncidents.at(action.payload.incidentIndex);
//         fmt::format_to(std::back_inserter(metrics), "{},{},{}", 
//             utils::formatTime(incident.timeRespondedTo), station.getStationIndex(), station.getStationId());

//         for (const auto& type : apparatusTypes) {
//             int dispatched = 0;
//             int remaining = station.getAvailableCount(type);

//             // Only fill dispatched for the type in this action
//             if (type == action.payload.apparatusType) {
//                 dispatched = action.payload.apparatusCount;
//             }
//             fmt::format_to(std::back_inserter(metrics), ",{},{}", dispatched, remaining);
//         }

//         fmt::format_to(std::back_inserter(metrics), ",{:.2f},{},{}", 
//             action.payload.travelTime, action.payload.incidentIndex, incident.incident_id);

//         station_csv << metrics << "\n";
//     }
//     station_csv.close();
// }