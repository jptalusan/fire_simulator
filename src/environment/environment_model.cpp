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
                spdlog::debug("Handling Incident: {} | Level: {}", incident->incident_type, to_string(incident->incident_level));

                // Example logic: mark incident as responded
                state.addToQueue(*incident);
            }
            break;
        }

        case EventType::IncidentResolution: {
            auto payload = std::dynamic_pointer_cast<IncidentResolutionEvent>(event.payload);
            int incidentID = payload->incidentID;
            // int stationIndex = payload->stationIndex;
            Incident& incident = state.getActiveIncidents()[incidentID];
            if (incident.incident_id != incidentID) {
                spdlog::error("[EnvironmentModel] Incident ID mismatch: {} vs {}", incidentID, incident.incident_id);
                throw MismatchError(); // Throw an error if the incident ID does not match
            }
            incident.resolvedTime = event.event_time; // Update the resolved time of the incident
            break;
        }

        // Catch-all for apparatus return and whatever i station related event that may occur
        case EventType::StationAction: {
            auto payload = std::dynamic_pointer_cast<FireStationEvent>(event.payload);
            int stationIndex = payload->stationIndex;
            int enginesCount = payload->enginesCount; // Number of engines involved in the event
            
            Station& station = state.getStation(stationIndex);
            station.setNumFireTrucks(station.getNumFireTrucks() + enginesCount);

            spdlog::info("[EnvironmentModel] {} fire trucks returned to station ID: {}: {}", 
                         enginesCount, stationIndex, formatTime(event.event_time));
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

std::vector<Event> EnvironmentModel::takeAction(State& state, const Action& action) {
    std::unordered_map<std::string, std::string> payload = action.payload;
    // TODO: Add reserve?
    std::vector<Event> newEvents = {};
    switch (action.type) {
        case StationActionType::Dispatch: {
            int stationIndex = std::stoi(payload[constants::STATION_INDEX]);
            Station& station = state.getStation(stationIndex);
            if (station.getNumFireTrucks() <= 0) {
                spdlog::warn("[EnvironmentModel] No fire trucks available at station ID: {}", stationIndex);
                break; // Exit if no fire trucks are available
            } else {
                int numberOfFireTrucksToDispatch = std::stoi(payload[constants::ENGINE_COUNT]);
                int incidentID = std::stoi(payload[constants::INCIDENT_ID]);
                double travelTime = std::stod(payload[constants::TRAVEL_TIME]); // Get travel time from action payload
                station.setNumFireTrucks(station.getNumFireTrucks() - numberOfFireTrucksToDispatch); // Decrease the number of fire trucks at the station

                // Gets the list of queued incidents, and sends to the first one.
                std::queue<Incident>& incidents = state.getIncidentQueue();
                if (!incidents.empty()) {
                    Incident& incident = incidents.front();
                    if (incident.incident_id == incidentID) {
                        // Create new events based on the action taken
                        appendNewEvents(state, action, newEvents);

                        // TODO: can put this in a single updateIncident function
                        incident.responseTime = state.getSystemTime(); // Set the response time for the incident
                        incident.hasBeenRespondedTo = true; // Mark the incident as responded to
                        incident.stationIndex = stationIndex; // Set the station index that responded to the incident
                        incident.engineCount = numberOfFireTrucksToDispatch; // Set the number of fire trucks dispatched to the incident
                        incident.oneWayTravelTimeTo = travelTime; // Set the travel time to the incident
                        
                        state.getActiveIncidents()[incidentID] = incident;
                        // Remove the incident from the queue after dispatching
                        incidents.pop();

                        // Logic to handle dispatching fire trucks to the incident
                        // For example, update the incident status or notify other components
                    } else {
                        spdlog::error("[EnvironmentModel] Incident ID does not match queued incident ID: {} vs {}", incidentID, incident.incident_id);
                        throw MismatchError(); // Throw an error if the incident ID does not match
                    }
                } else {
                    spdlog::warn("[EnvironmentModel] No queued incidents to dispatch to for station ID: {}", stationIndex);
                }
            }
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

    spdlog::debug("[EnvironmentModel] Action taken: {}", action.toString());
    return newEvents; // Return any new events generated by the action
}

// TODO: The above functions are placeholders and should be implemented with actual logic to create events.
/**
 * @brief Appends new events to the provided vector based on the action taken.
 * @param state The simulation state (may be modified).
 * @param action The action to process.
 * @param[out] newEvents The vector to which new events will be added.
 */
void appendNewEvents(State& state, const Action& action, std::vector<Event>& newEvents) {
    int incidentID = std::stoi(action.payload.at(constants::INCIDENT_ID)); // Get incident ID from action payload

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
    IncidentLevel incidentLevel = incident.incident_level;
    
    int stationIndex = std::stoi(action.payload.at(constants::STATION_INDEX)); // Get station index from action payload
    double travel_time = std::stod(action.payload.at(constants::TRAVEL_TIME)); // Get travel time from action payload
    int enginesSendCount = std::stoi(action.payload.at(constants::ENGINE_COUNT)); // Get engine count from action payload

    // TODO: Add error handling, but because of index and ID mismatch, i need to add index in station first.
    Station station = state.getStation(stationIndex);
    
    time_t resolutionTime = std::time(nullptr);
    resolutionTime = calculateResolutionTime(incidentLevel);
    resolutionTime = state.getSystemTime() + static_cast<time_t>(resolutionTime);
    if (resolutionTime < 0 || resolutionTime > 2147483647) {
        spdlog::error("Resolution time for incident {} out of bounds: {}", incidentID, resolutionTime);
    }

    // Resolution event creation
    IncidentResolutionEvent event(incidentID, stationIndex);
    newEvents.push_back(Event(EventType::IncidentResolution, resolutionTime, std::make_shared<IncidentResolutionEvent>(event)));

    // Fire Engine Idle Event creation
    // TODO: Add how many engines were sent out.
    resolutionTime += static_cast<time_t>(travel_time); // Add travel time to resolution time
    FireStationEvent fireEngineIdleEvent(stationIndex, enginesSendCount);

    spdlog::debug("Inserting new events.");
    newEvents.push_back(Event(EventType::StationAction, resolutionTime, std::make_shared<FireStationEvent>(fireEngineIdleEvent)));
}

time_t calculateResolutionTime(IncidentLevel incidentLevel) {
    double resolutionTime = 0; // Placeholder for resolution time, should be calculated based on incident type and other factors
    
    switch (incidentLevel) {
        case IncidentLevel::Low:
            resolutionTime = 5 * 300; // Example resolution time for low-level incidents
            break;
        case IncidentLevel::Moderate:
            resolutionTime = 10 * 300; // Example resolution time for moderate-level incidents
            break;
        case IncidentLevel::High:
            resolutionTime = 15 * 300; // Example resolution time for high-level incidents
            break;
        case IncidentLevel::Critical:
            resolutionTime = 20 * 300; // Example resolution time for critical-level incidents
            break;
        default:
            spdlog::error("[EnvironmentModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }
    
    return static_cast<time_t>(resolutionTime);
}