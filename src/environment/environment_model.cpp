#include <iostream>
#include <spdlog/spdlog.h>
#include "environment/environment_model.h"
#include "data/incident.h"
#include "simulator/event.h"
#include "utils/constants.h"
#include "utils/helpers.h"

EnvironmentModel::EnvironmentModel(FireModel& fireModel)
    : fireModel_(fireModel) {}

// TODO: This needs models for computing travel times, resolution times etc... (during take action).
std::vector<Event> EnvironmentModel::handleEvent(State& state, const Event& event) {
    std::vector<Event> newEvents = {};
    spdlog::debug("[{}] Handling {} for {}", formatTime(state.getSystemTime()), to_string(event.event_type), formatTime(event.event_time));

    checkIncidentStatus(state, event.event_time, newEvents); // Check the status of incidents and generate new events if needed

    switch (event.event_type) {
        case EventType::Incident: {
            auto incident = std::dynamic_pointer_cast<Incident>(event.payload);
            if (incident) {
                // spdlog::info("[{}] Incident: {} | Level: {}", formatTime(event.event_time), incident->incident_type, to_string(incident->incident_level));
                handleIncident(state, *incident, event.event_time);
            }
            break;
        }

        case EventType::IncidentResolution: {
            auto payload = std::dynamic_pointer_cast<IncidentResolutionEvent>(event.payload);
            int incidentIndex = payload->incidentIndex;
            auto& map = state.getActiveIncidents();
            auto it = map.find(incidentIndex);
            if (it != map.end()) {
                Incident& incident = it->second;
                if (incident.incidentIndex != incidentIndex) {
                    spdlog::error("[EnvironmentModel] Incident ID mismatch: {} vs {}", incidentIndex, incident.incidentIndex);
                    throw MismatchError(); // Throw an error if the incident ID does not match
                }
                incident.resolvedTime = event.event_time; // Update the resolved time of the incident
                incident.status = IncidentStatus::hasBeenResolved; // Update the status of the incident
                spdlog::info("[{}] Resolving incident {}",formatTime(event.event_time), incidentIndex);
                // Suppose you want to move the element with key incidentIndex
                state.doneIncidents_.emplace(it->first, std::move(it->second));
                // Remove from the original map
                state.getActiveIncidents().erase(it);
            }
            break;
        }

        case EventType::ApparatusArrivalAtIncident: {
            auto payload = std::dynamic_pointer_cast<FireStationEvent>(event.payload);
            int incidentIndex = payload->incidentIndex;
            auto& map = state.getActiveIncidents();
            auto it = map.find(incidentIndex);
            if (it != map.end()) {
                Incident& incident = it->second; // Get the incident from the active incidents map
                incident.status = IncidentStatus::isBeingResolved; // Update the status of the incident to being resolved
            }
            break;
        }
        
        case EventType::CheckIncident: {
            // Do nothing.
            break;
        }

        case EventType::ApparatusReturnToStation: {
            auto payload = std::dynamic_pointer_cast<FireStationEvent>(event.payload);
            int incidentIndex = payload->incidentIndex;
            int numberOfApparatus = payload->enginesCount;
            int stationIndex = payload->stationIndex;
            Station& station = state.getStation(stationIndex);
            station.setNumFireTrucks(station.getNumFireTrucks() + numberOfApparatus); // Increase the number of fire trucks at the station
            spdlog::info("[{}] {} fire trucks returned to station ID {} from incident {}", 
                            formatTime(event.event_time), numberOfApparatus, station.getAddress(), incidentIndex);
            break;
        }

        default: {
            std::string msg = "[" + formatTime(event.event_time) + "] Unknown event type: " + std::to_string(static_cast<int>(event.event_type));
            spdlog::warn(msg);
            throw UnknownValueError(msg); // Throw an error for unknown event types
            break;
        }
    }

    state.advanceTime(event.event_time); // Update system time in state
    return newEvents;
}

void EnvironmentModel::checkIncidentStatus(State& state, time_t eventTime, std::vector<Event>& newEvents) {
    // Check the status of incidents and generate new events if needed
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    for (const auto& [id, incident] : activeIncidents) {
        if (state.resolvingIncidentIndex_.count(incident.incidentIndex) > 0) {
            continue;
        }
        if (incident.status == IncidentStatus::isBeingResolved) {
            int incidentIndex = incident.incidentIndex;
            bool isIncidentResolved = fireModel_.computeResolutionTime(state, incident);
            if (isIncidentResolved) {
                IncidentResolutionEvent resolutionEvent(incident.incidentIndex);
                newEvents.push_back(Event(EventType::IncidentResolution, eventTime + constants::RESPOND_DELAY_SECONDS, std::make_shared<IncidentResolutionEvent>(resolutionEvent)));
                state.resolvingIncidentIndex_.insert(incident.incidentIndex);

                // Sending back each vehicle.
                for (const auto& item : incident.apparatusReceived) {
                    // Access each element of the tuple by reference
                    const int& stationIndex = std::get<0>(item);
                    const int& numberOfApparatus = std::get<1>(item);
                    const double& travelTime = std::get<2>(item);

                    FireStationEvent fireEngineIdleEvent(stationIndex, incidentIndex, numberOfApparatus);
                    time_t nextEventTime = eventTime + static_cast<time_t>(travelTime) + constants::RESPOND_DELAY_SECONDS; // Calculate the next event time
                    newEvents.push_back(Event(EventType::ApparatusReturnToStation, nextEventTime, std::make_shared<FireStationEvent>(fireEngineIdleEvent)));
                }
            }
        }
    }
}

std::vector<Event> EnvironmentModel::takeActions(State& state, const std::vector<Action>& actions) {
    // Obtained to make sure that the incident we are addressing downstream matches the earliest incident in the queue.
    if (actions.size() == 0 || actions[0].type == StationActionType::DoNothing) {
        spdlog::debug("[EnvironmentModel] No actions provided, returning empty vector.");
        return {}; // Return an empty vector if no actions are provided
    }

    std::unordered_map<std::string, std::string> payload = actions[0].payload;
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    int incidentIndex = std::stoi(payload[constants::INCIDENT_INDEX]);
    Incident incident = activeIncidents.at(incidentIndex);
    if (incident.incidentIndex < 0) {
        spdlog::debug("[EnvironmentModel] No unresolved incident found in active incidents");
        return {}; // Return an empty vector if no unresolved incident is found
    }
    
    std::vector<Event> newEvents = {};
    int totalApparatusDispatched = 0;
    
    incident.timeRespondedTo = state.getSystemTime() + constants::SECONDS_IN_MINUTE; // Set the response time for the incident
    for (const auto& action : actions) {
        std::unordered_map<std::string, std::string> payload = action.payload;
        switch (action.type) {
            case StationActionType::Dispatch: {
                int stationIndex = std::stoi(payload[constants::STATION_INDEX]);
                Station& station = state.getStation(stationIndex);
                int numberOfFireTrucksToDispatch = std::stoi(payload[constants::ENGINE_COUNT]);

                int incidentIndex = std::stoi(payload[constants::INCIDENT_INDEX]);
                if (incidentIndex != incident.incidentIndex) {
                    spdlog::error("[EnvironmentModel] Incident ID does not match target incident ID: {} vs {}", incidentIndex, incident.incidentIndex);
                    throw MismatchError(); // Throw an error if the incident ID does not match
                }

                double travelTime = std::stod(payload[constants::TRAVEL_TIME]); // Get travel time from action payload
                station.setNumFireTrucks(station.getNumFireTrucks() - numberOfFireTrucksToDispatch); // Decrease the number of fire trucks at the station
                
                // Writing to metrics
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2);
                oss << formatTime(incident.timeRespondedTo) << ",";
                oss << station.getStationIndex() << ",";
                oss << numberOfFireTrucksToDispatch << ",";
                oss << (station.getNumFireTrucks()) << ",";
                oss << travelTime << ",";
                oss << incident.incident_id << ",";
                oss << incident.incidentIndex; // Append incident index to the metric string
                state.updateStationMetrics(oss.str()); // Update station metrics with the new data

                // Gets the list of queued incidents, and sends to the first one.
                // Create new events based on the action taken
                totalApparatusDispatched += numberOfFireTrucksToDispatch;
                auto apparatusDispatched = std::make_tuple(stationIndex, numberOfFireTrucksToDispatch, travelTime);
                incident.apparatusReceived.push_back(apparatusDispatched);
                incident.status = IncidentStatus::hasBeenRespondedTo; // Update the status of the incident to dispatched
                break;
            }
            case StationActionType::DoNothing: {
                spdlog::debug("[EnvironmentModel] No action taken for station ID: {}", action.toString());
                break; // No action to take, exit early
            }
            default: {
                spdlog::error("[EnvironmentModel] Unknown action type: {}", static_cast<int>(action.type));
                throw UnknownValueError(); // Throw an error for unknown action types
                break; // Exit if action type is unknown
            }
        }
    }

    incident.currentApparatusCount += totalApparatusDispatched; // Set the number of fire trucks dispatched to the incident

    state.getActiveIncidents()[incident.incidentIndex] = incident; // Insert the incident into the active incidents map
    generateStationEvents(state, actions, newEvents); // Generate station events based on the actions taken
    // spdlog::debug("[EnvironmentModel] Action taken: {}", action.toString());
    return newEvents; // Return any new events generated by the action
}

void EnvironmentModel::handleIncident(State& state, Incident& incident, time_t eventTime) {
    // This should be the new event time already.
    int totalApparatusRequired = calculateApparatusCount(incident);
    spdlog::info("[{}] Incident {} | Level: {} | Requires: {} apparatus.", 
                 formatTime(eventTime), 
                 incident.incidentIndex, 
                 to_string(incident.incident_level), 
                 totalApparatusRequired);
    incident.totalApparatusRequired = totalApparatusRequired; // Set the number of fire trucks needed for the incident
    state.getActiveIncidents().insert({incident.incidentIndex, incident}); // Add the incident to the active incidents map
}

// TODO: The above functions are placeholders and should be implemented with actual logic to create events.
/**
 * @brief Appends new events to the provided vector based on the action taken.
 * @param state The simulation state (may be modified).
 * @param action The action to process.
 * @param[out] newEvents The vector to which new events will be added.
 */
void EnvironmentModel::generateStationEvents(State& state, 
    const std::vector<Action>& actions, 
    std::vector<Event>& newEvents) {
    if (actions.empty()) {
        spdlog::warn("[EnvironmentModel] No actions provided to generate station events.");
        return; // Exit early if no actions are provided
    }
    
    int incidentIndex = std::stoi(actions[0].payload.at(constants::INCIDENT_INDEX)); // Get incident ID from action payload

    // Error handling
    if (incidentIndex < 0) {
        spdlog::error("[EnvironmentModel] Invalid incident ID: {}", incidentIndex);
        throw InvalidIncidentError(); // Throw an error for invalid incident IDs
    }

    for (const auto& action : actions) {
        spdlog::debug("[EnvironmentModel] Processing action: {}", action.toString());
        int stationIndex = std::stoi(action.payload.at(constants::STATION_INDEX)); // Get station index from action payload
        double travel_time = std::stod(action.payload.at(constants::TRAVEL_TIME)); // Get travel time from action payload
        int enginesSendCount = std::stoi(action.payload.at(constants::ENGINE_COUNT)); // Get engine count from action payload

        Station station = state.getStation(stationIndex);
        time_t timeToArriveAtIncident = state.getSystemTime() + constants::RESPOND_DELAY_SECONDS + static_cast<time_t>(travel_time); // Add travel time to resolution time
        FireStationEvent apparatusArrival(stationIndex, incidentIndex, enginesSendCount);
        spdlog::debug("Inserting new events.");
        newEvents.push_back(Event(EventType::ApparatusArrivalAtIncident, timeToArriveAtIncident, std::make_shared<FireStationEvent>(apparatusArrival)));
    }
}

int EnvironmentModel::calculateApparatusCount(const Incident& incident) {
    int apparatusCount = 0; // Placeholder for apparatus count
    IncidentLevel incidentLevel = incident.incident_level;
    switch (incidentLevel) {
        case IncidentLevel::Low:
            apparatusCount = 1; // Example apparatus count for low-level incidents
            break;
        case IncidentLevel::Moderate:
            apparatusCount = 2; // Example apparatus count for moderate-level incidents
            break;
        case IncidentLevel::High:
            apparatusCount = 3; // Example apparatus count for high-level incidents
            break;
        case IncidentLevel::Critical:
            apparatusCount = 4; // Example apparatus count for critical-level incidents
            break;
        default:
            spdlog::error("[EnvironmentModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }
    return apparatusCount; // Return the calculated apparatus count
}