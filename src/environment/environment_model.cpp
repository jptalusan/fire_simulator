#include <iostream>
#include <spdlog/spdlog.h>
#include "environment/environment_model.h"
#include "data/incident.h"
#include "simulator/event.h"
#include "utils/constants.h"
#include "utils/helpers.h"
#include "utils/error.h"
#include <fmt/format.h>

EnvironmentModel::EnvironmentModel(FireModel& fireModel)
    : fireModel_(fireModel) {}

// TODO: When a new incident needs to be generated, add it first to state.AllIncidents_
std::vector<Event> EnvironmentModel::handleEvent(State& state, const Event& event) {
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
            auto& activeIncidents = state.getActiveIncidents();
            if (auto it = activeIncidents.find(incidentIndex); it != activeIncidents.end()) {
                Incident& incident = it->second;
                incident.resolvedTime = event.event_time; // Update the resolved time of the incident
                incident.status = IncidentStatus::hasBeenResolved; // Update the status of the incident
                state.doneIncidents_.emplace(it->first, std::move(it->second));
                state.getActiveIncidents().erase(it);
                std::vector<int> &inProgressIncidentIndices = state.inProgressIncidentIndices;
                inProgressIncidentIndices.erase(std::remove(
                    inProgressIncidentIndices.begin(),
                    inProgressIncidentIndices.end(), incidentIndex),
                    inProgressIncidentIndices.end());
            }
            break;
        }

        case EventType::ApparatusArrivalAtIncident: {
            int incidentIndex = event.incidentIndex;
            auto& activeIncidents = state.getActiveIncidents();
            if (auto it = activeIncidents.find(incidentIndex); it != activeIncidents.end()) {
                Incident& incident = it->second;
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
            // Optimized: Use fmt library (already included)
            std::string msg = fmt::format("[{}] Unknown event type: {}", 
                formatTime(event.event_time), static_cast<int>(event.event_type));
            spdlog::warn(msg);
            throw UnknownValueError(msg); // Throw an error for unknown event types
            break;
        }
    }

    state.advanceTime(event.event_time); // Update system time in state
    return {};
}

std::vector<Event> EnvironmentModel::takeActions(State& state, const std::vector<Action>& actions) {
    if (actions.empty() || actions[0].type == StationActionType::DoNothing) {
        return {};
    }

    // Pre-allocate events vector
    std::vector<Event> newEvents;
    newEvents.reserve(actions.size() * 2);  // Estimate: 2 events per action
    
    auto& activeIncidents = state.getActiveIncidents();  // Cache reference
    int incidentIndex = actions[0].payload.incidentIndex;
    
    auto incidentIt = activeIncidents.find(incidentIndex);
    if (incidentIt == activeIncidents.end()) {
        spdlog::debug("[EnvironmentModel] No unresolved incident found");
        return {};
    }
    
    Incident& incident = incidentIt->second;  // Direct reference, no copy
    int totalApparatusDispatched = 0;
    bool hasSentResolutionEvent = false;
    
    incident.timeRespondedTo = state.getSystemTime() + constants::SECONDS_IN_MINUTE; // Set the response time for the incident
    for (const auto& action : actions) {
        if (action.type == StationActionType::Dispatch) {
            processDispatchAction(state, action, incident, newEvents, 
                                totalApparatusDispatched, hasSentResolutionEvent);
        }
        else if (action.type == StationActionType::DoNothing) {
            spdlog::debug("[EnvironmentModel] No action taken.");
            continue; // No action to take, skip to next action
        } else {
            spdlog::error("[EnvironmentModel] Unknown action type: {}", static_cast<int>(action.type));
            throw UnknownValueError(); // Throw an error for unknown action types
        }
    }

    incident.currentApparatusCount += totalApparatusDispatched; // Set the number of fire trucks dispatched to the incident

    generateStationEvents(state, actions, newEvents); // Generate station events based on the actions taken
    return newEvents; // Return any new events generated by the action
}

void EnvironmentModel::handleIncident(State& state, Incident& incident, time_t eventTime) {
    // This should be the new event time already.
    int totalApparatusRequired = fireModel_.calculateApparatusCount(incident);
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
        newEvents.emplace_back(apparatusArrival);
    }
}

/* Extract dispatch logic to separate method 
TODO: This is where the dispatched apparatus is subtracted from the total count at the station.
*/
void EnvironmentModel::processDispatchAction(State& state, const Action& action, 
                                           Incident& incident, std::vector<Event>& newEvents,
                                           int& totalApparatusDispatched, bool& hasSentResolutionEvent) {
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
        newEvents.emplace_back(Event::createIncidentResolutionEvent(timeToResolveIncident, incident.incidentIndex));
    }

    time_t nextEventTime = timeToResolveIncident + static_cast<time_t>(travelTime); // Calculate the next event time
    newEvents.emplace_back(Event::createApparatusReturnEvent(nextEventTime, stationIndex, incident.incidentIndex, numberOfFireTrucksToDispatch));
}