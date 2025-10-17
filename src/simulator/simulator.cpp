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
    logActions(actions, state_.getSystemTime());
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

    // Log the state after taking actions and simulating time step

    // Advance the system time to the next incident's report time
    state_.advanceTime(nextIncidentTime);
    logState(state_);

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
                        vehicle.timeStartedToDispatch = -1;
                        vehicle.timeToStartedReturning = -1; // Resetting as we don't need it until next return
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
                        vehicle.setTimeToIncident(-1);
                        LOG_INFO("[{}] Vehicle {} is done and returning to {} by {}", utils::formatTime(sim_time), vehicle.getVehicleId(), vehicle.getStationId(), utils::formatTime(vehicle.getTimeToReturn()));
                        // state_.getActiveIncidents().at(incidentIndex) = incident; // Update the incident in the active incidents map
                        doneIncidents_.insert({incidentIndex, incident});
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
                        
                        // Reset timers for returning and dispatching
                        LOG_INFO("[{}] Vehicle {} has returned to station and is now available", utils::formatTime(sim_time), vehicle.getVehicleId());
                        vehicle.timeToStartedReturning = -1; // Resetting as we don't need it until next return
                        vehicle.timeStartedToDispatch = -1; // Resetting as we don't need it until next dispatch
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
    vehicles_history_.clear();
    stations_history_.clear();
    actions_history_.clear();
    doneIncidents_.clear();
    return state_;
}

void Simulator::logActions(const std::vector<Action>& actions, time_t current_time) {
    actions_history_.emplace_back(current_time, actions);
}

void Simulator::logState(const State& state) {
    vehicles_history_.push_back(state.getConstVehicleList());
    stations_history_.push_back(state.getAllStations());
    state_times_history_.push_back(state.getSystemTime());
}

// TODO: CurrLat and CurrLon are wrong here, should be location upon dispatch
void Simulator::writeActionReport(const State& state) const {
    std::string report_path = EnvLoader::getInstance()->get("STATION_REPORT_CSV_PATH", "../logs/station_report.csv");
    std::ofstream csv(report_path);

    csv << "Time,VehicleID,Status,StationIndex,StationID,IncidentID,Type,Count,TravelTimeToIncident,CurrLat,CurrLon,IncidentLat,IncidentLon\n";

    for (size_t t = 0; t < vehicles_history_.size(); ++t) {
        const auto& vehicles = vehicles_history_[t];
        // const auto& stations = stations_history_[t];
        // const time_t current_time = state_times_history_[t];
        const auto& actions = actions_history_[t];
        for (const auto& action : actions.second) {
            if (action.type != StationActionType::Dispatch) continue;
            Incident incident = state.getActiveIncidentsConst().at(action.payload.incidentIndex);
            csv << std::fixed << std::setprecision(6);
            csv << utils::formatTime(actions.first) << ","
                << action.payload.vehicleIndex << ","
                << to_string(ApparatusStatus::Dispatched) << ","
                << action.payload.stationIndex << ","
                << state.getAllStations().at(action.payload.stationIndex).getStationId() << ","
                << action.payload.incidentIndex << ","
                << to_string(action.payload.apparatusType) << ","
                << action.payload.apparatusCount << ","
                << action.payload.travelTime << ","
                << vehicles.at(action.payload.vehicleIndex).getStationLocation().lat << ","
                << vehicles.at(action.payload.vehicleIndex).getStationLocation().lon << ","
                << incident.getLocation().lat << ","
                << incident.getLocation().lon << "\n";
        }
    }
    csv.close();
}

void Simulator::writeIncidentReport() const {
    std::string report_path = EnvLoader::getInstance()->get("REPORT_CSV_PATH", "../logs/incident_report.csv");
    std::ofstream csv(report_path);
    csv << "IncidentIndex,IncidentID,Reported,Responded,Resolved";
    for (const auto& type : apparatusTypes) {
        csv << "," << to_string(type) << "Required," << to_string(type) << "Received";
    }
    csv << ",Zone,Status\n";
    std::vector<Incident> sortedIncidents;
    sortedIncidents.reserve(doneIncidents_.size());  // Preallocate memory for efficiency
    for (const auto& [id, incident] : doneIncidents_) {
        sortedIncidents.emplace_back(incident);
    }
    std::sort(sortedIncidents.begin(), sortedIncidents.end(),
        [](const Incident& a, const Incident& b) {
            return a.reportTime < b.reportTime;
        });

    for (size_t i = 0; i < sortedIncidents.size(); ++i) {
        const auto& incident = sortedIncidents[i];
        if (incident.resolvedTime < 0 || incident.resolvedTime > 2147483647) {
            LOG_ERROR("Incident {} has a resolved time out of bounds: {}", incident.incidentIndex, incident.resolvedTime);
            continue; // Skip this incident
        }
        // TODO: Fix apparatus count and add type.
        csv << std::fixed << std::setprecision(6);
        csv << incident.incidentIndex << ","
            << incident.incident_id << ","
            << utils::formatTime(incident.reportTime) << ","
            << utils::formatTime(incident.timeRespondedTo) << ","
            << utils::formatTime(incident.resolvedTime);

         // Output required and received for each apparatus type
         for (const auto& type : apparatusTypes) {
             int required = 0;
             int received = 0;
             auto reqIt = incident.requiredApparatusMap.find(type);
             if (reqIt != incident.requiredApparatusMap.end()) required = reqIt->second;
             auto recIt = incident.currentApparatusMap.find(type);
             if (recIt != incident.currentApparatusMap.end()) received = recIt->second;
             csv << "," << required << "," << received;
         }
         csv << "," << incident.zoneIndex << "," << to_string(incident.status) << "\n";
    }
    csv.close();
}

void Simulator::writeVehicleReport() const {
    std::string report_path = EnvLoader::getInstance()->get("STATION_REPORT_CSV_PATH", "../logs/station_report.csv");
    std::ofstream csv(report_path);

    csv << "Time,VehicleID,Status,StationID,IncidentID,Type,TravelTimeToIncident,TravelTimeToStation,Lat,Lon\n";

    for (size_t t = 0; t < vehicles_history_.size(); ++t) {
        const auto& vehicles = vehicles_history_[t];
        // const auto& stations = stations_history_[t];
        const time_t current_time = state_times_history_[t];
        for (const auto& vehicle : vehicles) {
            if (vehicle.getVehicleId() != 23) {
                continue; // Only log vehicle 23 for now
            }
            if (vehicle.getStatus() == ApparatusStatus::Available) {
                continue; // Only log non-available vehicles for now
            }
            time_t travelTimeToIncident = -1;;
            time_t travelTimeToStation = -1;
            if ((vehicle.getTimeToIncident() > 0) && (vehicle.timeStartedToDispatch > 0)) {
                travelTimeToIncident = difftime(vehicle.getTimeToIncident(), vehicle.timeStartedToDispatch);
            }
            if ((vehicle.getTimeToReturn() > 0) && (vehicle.timeToStartedReturning > 0)) {
                travelTimeToStation = difftime(vehicle.getTimeToReturn(), vehicle.timeToStartedReturning);
            }
            csv << std::fixed << std::setprecision(6);
            csv << utils::formatTime(current_time) << ","
                << vehicle.getVehicleId() << ","
                << to_string(vehicle.getStatus()) << ","
                << vehicle.getStationId() << ","
                << vehicle.getIncidentIndex() << ","
                << to_string(vehicle.getType()) << ","
                << travelTimeToIncident << ","
                << travelTimeToStation << ","
                << vehicle.getCurrentLocation().lat << ","
                << vehicle.getCurrentLocation().lon << "\n";
        }
    }
    csv.close();
}