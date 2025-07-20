#include <iostream>
#include <spdlog/spdlog.h>
#include "environment/environment_model.h"
#include "data/incident.h"
#include "simulator/event.h"
#include "utils/constants.h"
#include "utils/helpers.h"
#include "utils/error.h"

EnvironmentModel::EnvironmentModel(FireModel& fireModel)
    : fireModel_(fireModel) {}

// TODO: When a new incident needs to be generated, add it first to state.AllIncidents_
std::vector<Event> EnvironmentModel::handleEvent(State& state, const Event& event) {
    std::vector<Event> newEvents = {};
    spdlog::debug("[{}] Handling {} for {}", formatTime(state.getSystemTime()), to_string(event.event_type), formatTime(event.event_time));

    switch (event.event_type) {
        case EventType::Incident: {
            int incidentIndex = event.incidentIndex;
            if (incidentIndex >= 0) {
                // spdlog::info("[{}] Incident: {} | Level: {}", formatTime(event.event_time), incident->incident_type, to_string(incident->incident_level));
                const Incident& incident = state.getAllIncidents().at(incidentIndex);
                Incident modifiableIncident = incident;
                handleIncident(state, modifiableIncident, event.event_time);
            }
            break;
        }

        case EventType::IncidentResolution: {
            int incidentIndex = event.incidentIndex;
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
                // TODO: Move this erase to when we dispatch the vehicles
                // If it meets the required apparatus count, remove it from active incidents
                state.getActiveIncidents().erase(it);
                state.inProgressIncidentIndices.remove(incidentIndex);
            }
            break;
        }

        case EventType::ApparatusArrivalAtIncident: {
            int incidentIndex = event.incidentIndex;
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
            int incidentIndex = event.incidentIndex;
            int numberOfApparatus = event.enginesCount;
            int stationIndex = event.stationIndex;
            Station& station = state.getStation(stationIndex);
            station.setNumFireTrucks(station.getNumFireTrucks() + numberOfApparatus); // Increase the number of fire trucks at the station
            spdlog::info("[{}] {} fire trucks returned to station {} from incident {}", 
                            formatTime(event.event_time), numberOfApparatus, station.getStationId(), incidentIndex);
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

std::vector<Event> EnvironmentModel::takeActions(State& state, const std::vector<Action>& actions) {
    // Obtained to make sure that the incident we are addressing downstream matches the earliest incident in the queue.
    if (actions.size() == 0 || actions[0].type == StationActionType::DoNothing) {
        spdlog::debug("[EnvironmentModel] No actions provided, returning empty vector.");
        return {}; // Return an empty vector if no actions are provided
    }

    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    int incidentIndex = actions[0].payload.incidentIndex;;
    Incident& incident = activeIncidents.at(incidentIndex);
    if (incident.incidentIndex < 0) {
        spdlog::debug("[EnvironmentModel] No unresolved incident found in active incidents");
        return {}; // Return an empty vector if no unresolved incident is found
    }
    
    std::vector<Event> newEvents = {};
    int totalApparatusDispatched = 0;
    bool hasSentResolutionEvent = false;
    
    incident.timeRespondedTo = state.getSystemTime() + constants::SECONDS_IN_MINUTE; // Set the response time for the incident
    for (const auto& action : actions) {
        switch (action.type) {
            case StationActionType::Dispatch: {
                int stationIndex = action.payload.stationIndex;
                Station& station = state.getStation(stationIndex);
                int numberOfFireTrucksToDispatch = action.payload.enginesCount;

                int incidentIndex = action.payload.incidentIndex;
                if (incidentIndex != incident.incidentIndex) {
                    spdlog::error("[EnvironmentModel] Incident ID does not match target incident ID: {} vs {}", incidentIndex, incident.incidentIndex);
                    throw MismatchError(); // Throw an error if the incident ID does not match
                }

                double travelTime = action.payload.travelTime;
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

                double incidentResolutionTime = fireModel_.computeResolutionTime(state, incident);
                time_t timeToResolveIncident = state.getSystemTime() + static_cast<time_t>(incidentResolutionTime) + constants::RESPOND_DELAY_SECONDS;
                incident.resolvedTime = timeToResolveIncident; // Set the resolved time for the incident
                // HACK: We dont want to send multiple incident resolution events because we only want this to occur once, regardless of how many apparatus were sent.
                if (!hasSentResolutionEvent) {
                    hasSentResolutionEvent = true; // Ensure we only send one resolution event
                    Event resolutionEvent = Event::createIncidentResolutionEvent(timeToResolveIncident, incident.incidentIndex);
                    newEvents.push_back(resolutionEvent);
                }

                time_t nextEventTime = timeToResolveIncident + static_cast<time_t>(travelTime); // Calculate the next event time
                Event fireEvent = Event::createApparatusReturnEvent(nextEventTime, stationIndex, incident.incidentIndex, numberOfFireTrucksToDispatch);
                newEvents.push_back(fireEvent); // Create a new event for the fire truck returning to the station

                    // Compute the resolution event here immediately based on the current status
                // based on totalApparatusRequired and currentApparatusCount;

                break;
            }
            case StationActionType::DoNothing: {
                spdlog::debug("[EnvironmentModel] No action taken.");
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

    generateStationEvents(state, actions, newEvents); // Generate station events based on the actions taken
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
    state.inProgressIncidentIndices.push_back(incident.incidentIndex); // Add the incident index to the in-progress incidents list
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
    
    int incidentIndex = actions[0].payload.incidentIndex;

    // Error handling
    if (incidentIndex < 0) {
        spdlog::error("[EnvironmentModel] Invalid incident ID: {}", incidentIndex);
        throw InvalidIncidentError(); // Throw an error for invalid incident IDs
    }

    for (const auto& action : actions) {
        spdlog::debug("[EnvironmentModel] Processing action: {}", to_string(action.type));
        int stationIndex = action.payload.stationIndex; // Get station index from action payload
        double travel_time = action.payload.travelTime; // Get travel time from action payload
        int enginesSendCount = action.payload.enginesCount; // Get engine count from action payload

        Station station = state.getStation(stationIndex);
        // check if stationIndex and station.getStationIndex() are the same
        if (stationIndex != station.getStationIndex()) {
            spdlog::error("[EnvironmentModel] Station index mismatch: {} != {}", stationIndex, station.getStationIndex());
            throw StationIndexMismatchError(); // Throw an error for station index mismatches
        }
        time_t timeToArriveAtIncident = state.getSystemTime() + constants::RESPOND_DELAY_SECONDS + static_cast<time_t>(travel_time); // Add travel time to resolution time
        Event apparatusArrival = Event::createApparatusArrivalEvent(timeToArriveAtIncident, station.getStationIndex(), incidentIndex, enginesSendCount);

        spdlog::debug("Inserting new events.");
        newEvents.push_back(apparatusArrival);
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
