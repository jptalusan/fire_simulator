#include <iostream>
#include <spdlog/spdlog.h>
#include "environment/environment_model.h"
#include "data/incident.h"
#include "simulator/event.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/helpers.h"

// TODO: This needs models for computing travel times, resolution times etc... (during take action).
void EnvironmentModel::handleEvent(State& state, const Event& event) {
    spdlog::debug("[EnvironmentModel] Handling {} at time: {}", to_string(event.event_type), formatTime(event.event_time));

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
            int incidentID = payload->incidentID;
            // int stationIndex = payload->stationIndex;
            auto& map = state.getActiveIncidents();
            auto it = map.find(incidentID);
            if (it != map.end()) {
                Incident& incident = it->second;
                if (incident.incident_id != incidentID) {
                    spdlog::error("[EnvironmentModel] Incident ID mismatch: {} vs {}", incidentID, incident.incident_id);
                    throw MismatchError(); // Throw an error if the incident ID does not match
                }
                if (event.event_time < 0 || event.event_time > 2147483647) {
                    spdlog::error("Resolution time for incident {} out of bounds: {}", incidentID, event.event_time);
                } else {
                    incident.resolvedTime = event.event_time; // Update the resolved time of the incident
                }
            } else {
                // handle missing key
                spdlog::error("Incident {} not found", incidentID);
            }


            break;
        }

        // Catch-all for apparatus return and whatever i station related event that may occur
        case EventType::StationAction: {
            auto payload = std::dynamic_pointer_cast<FireStationEvent>(event.payload);
            int stationIndex = payload->stationIndex;
            int enginesCount = payload->enginesCount; // Number of engines involved in the event
            
            Station& station = state.getStation(stationIndex);
            station.setNumFireTrucks(station.getNumFireTrucks() + enginesCount);

            spdlog::info("[{}] {} fire trucks returned to station ID: {}", 
                         formatTime(event.event_time), enginesCount, station.getAddress());
            break;
        }

        default: {
            std::string msg = "[EnvironmentModel] Unknown event type: " + std::to_string(static_cast<int>(event.event_type));
            spdlog::warn(msg);
            throw UnknownValueError(msg); // Throw an error for unknown event types
            break;
        }
    }

    state.advanceTime(event.event_time); // Update system time in state
}

std::vector<Event> EnvironmentModel::takeActions(State& state, const std::vector<Action>& actions) {
    // Obtained to make sure that the incident we are addressing downstream matches the earliest incident in the queue.
    Incident incident = state.getEarliestUnresolvedIncident();
    if (incident.incident_id < 0) {
        spdlog::debug("[EnvironmentModel] No unresolved incident found");
        return {};
    }
    
    std::vector<Event> newEvents;

    int totalApparatusDispatched = 0;
    // TODO: Modify this to handle multiple stationIndices if needed.
    int stationIndex = -1; // Initialize station index
    // TODO: Same for this
    double travelTime = 0.0; // Initialize travel time

    for (const auto& action : actions) {
        std::unordered_map<std::string, std::string> payload = action.payload;
        switch (action.type) {
            case StationActionType::Dispatch: {
                stationIndex = std::stoi(payload[constants::STATION_INDEX]);
                Station& station = state.getStation(stationIndex);
                int numberOfFireTrucksToDispatch = std::stoi(payload[constants::ENGINE_COUNT]);

                int incidentID = std::stoi(payload[constants::INCIDENT_ID]);
                if (incidentID != incident.incident_id) {
                    spdlog::error("[EnvironmentModel] Incident ID does not match queued incident ID: {} vs {}", incidentID, incident.incident_id);
                    throw MismatchError(); // Throw an error if the incident ID does not match
                }

                travelTime = std::stod(payload[constants::TRAVEL_TIME]); // Get travel time from action payload
                station.setNumFireTrucks(station.getNumFireTrucks() - numberOfFireTrucksToDispatch); // Decrease the number of fire trucks at the station

                // Gets the list of queued incidents, and sends to the first one.
                // Create new events based on the action taken
                totalApparatusDispatched += numberOfFireTrucksToDispatch;
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

    // TODO: can put this in a single updateIncident function
    incident.responseTime = state.getSystemTime() + constants::SECONDS_IN_MINUTE; // Set the response time for the incident
    incident.hasBeenRespondedTo = true; // Mark the incident as responded to
    incident.stationIndex = stationIndex; // Set the station index that responded to the incident
    incident.currentApparatusCount += totalApparatusDispatched; // Set the number of fire trucks dispatched to the incident
    incident.oneWayTravelTimeTo = travelTime; // Set the travel time to the incident
    // Remove the incident if it has been fully addressed
    // TODO: For now we start resolution event even if the incident is not fully addressed.
    time_t resolutionTime = generateIncidentResolutionEvent(state, incident, newEvents); // Generate a resolution event for the incident
    incident.timeToResolve = resolutionTime; // Set the time to resolve the incident
    generateStationEvents(state, actions, newEvents); // Generate station events based on the actions taken

    if (incident.currentApparatusCount >= incident.totalApparatusRequired) {
        state.getIncidentQueue().pop();
        state.getActiveIncidents().insert({incident.incident_id, incident});
    }

    // spdlog::debug("[EnvironmentModel] Action taken: {}", action.toString());
    return newEvents; // Return any new events generated by the action
}

void EnvironmentModel::handleIncident(State& state, Incident& incident, time_t eventTime) {
    // This should be the new event time already.
    int totalApparatusRequired = calculateApparatusCount(incident);
    double timeToResolve = calculateResolutionTime(incident);
    spdlog::info("[{}] Incident {} | Level: {} | Requires: {} apparatus and with clear time of {:.2f} minutes.", 
                 formatTime(eventTime), 
                 incident.incident_id, 
                 to_string(incident.incident_level), 
                 totalApparatusRequired, 
                 timeToResolve / constants::SECONDS_IN_MINUTE);
    // Example logic: add the incident to the queue
    incident.totalApparatusRequired = totalApparatusRequired; // Set the number of fire trucks needed for the incident
    incident.timeToResolve = timeToResolve;
    state.addToQueue(incident);
}

time_t EnvironmentModel::generateIncidentResolutionEvent(State& state, const Incident& incident, std::vector<Event>& newEvents) {
    // Create a new IncidentResolutionEvent
    IncidentResolutionEvent resolutionEvent(incident.incident_id, incident.stationIndex);
    double resolutionTime = 1800;
    time_t nextEventTime = state.getSystemTime() + constants::SECONDS_IN_MINUTE + static_cast<time_t>(resolutionTime); // Set the resolution time based on the incident's time to resolve

    // Add the event to the new events vector
    newEvents.push_back(Event(EventType::IncidentResolution, nextEventTime, std::make_shared<IncidentResolutionEvent>(resolutionEvent)));
    return nextEventTime;
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
    
    int incidentID = std::stoi(actions[0].payload.at(constants::INCIDENT_ID)); // Get incident ID from action payload

    // Error handling
    if (incidentID < 0) {
        spdlog::error("[EnvironmentModel] Invalid incident ID: {}", incidentID);
        throw InvalidIncidentError(); // Throw an error for invalid incident IDs
    }
    Incident incident = state.getEarliestUnresolvedIncident();
    
    if (incident.incident_id != incidentID) {
        spdlog::error("[EnvironmentModel] Incident ID does not match the unresolved incident ID: {} vs {}", incidentID, incident.incident_id);
        throw MismatchError(); // Throw an error if the incident ID does not match
    }
    time_t timeToResolve = incident.timeToResolve;

    // IncidentLevel incidentLevel = incident.incident_level;
    for (const auto& action : actions) {
        spdlog::debug("[EnvironmentModel] Processing action: {}", action.toString());
        int stationIndex = std::stoi(action.payload.at(constants::STATION_INDEX)); // Get station index from action payload
        double travel_time = std::stod(action.payload.at(constants::TRAVEL_TIME)); // Get travel time from action payload
        int enginesSendCount = std::stoi(action.payload.at(constants::ENGINE_COUNT)); // Get engine count from action payload

        // TODO: Add error handling, but because of index and ID mismatch, i need to add index in station first.
        Station station = state.getStation(stationIndex);
        
        time_t nextEventTime = state.getSystemTime() + constants::SECONDS_IN_MINUTE + timeToResolve + static_cast<time_t>(travel_time); // Add travel time to resolution time
        if (nextEventTime <= 0 || nextEventTime > 2147483647) {
            spdlog::error("Resolution time for incident {} out of bounds: {}", incidentID, nextEventTime);
        }

        // Fire Engine Idle Event creation
        // TODO: Add how many engines were sent out.
        nextEventTime += static_cast<time_t>(travel_time); // Add travel time to resolution time
        FireStationEvent fireEngineIdleEvent(stationIndex, enginesSendCount);

        spdlog::debug("Inserting new events.");
        newEvents.push_back(Event(EventType::StationAction, nextEventTime, std::make_shared<FireStationEvent>(fireEngineIdleEvent)));
    }
}

double EnvironmentModel::calculateResolutionTime(const Incident& incident) {
    double resolutionTime = 0; // Placeholder for resolution time, should be calculated based on incident type and other factors

    IncidentLevel incidentLevel = incident.incident_level;
    switch (incidentLevel) {
        case IncidentLevel::Low:
            resolutionTime = 10 * constants::SECONDS_IN_MINUTE; // Example resolution time for low-level incidents
            break;
        case IncidentLevel::Moderate:
            resolutionTime = 30 * constants::SECONDS_IN_MINUTE; // Example resolution time for moderate-level incidents
            break;
        case IncidentLevel::High:
            resolutionTime = 60 * constants::SECONDS_IN_MINUTE; // Example resolution time for high-level incidents
            break;
        case IncidentLevel::Critical:
            resolutionTime = 90 * constants::SECONDS_IN_MINUTE; // Example resolution time for critical-level incidents
            break;
        default:
            spdlog::error("[EnvironmentModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }
    
    return resolutionTime;
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