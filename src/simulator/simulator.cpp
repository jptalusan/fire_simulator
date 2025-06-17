#include <algorithm>
#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include "simulator/action.h"
#include "simulator/simulator.h"
#include "utils/helpers.h"

Simulator::Simulator(State& initialState, 
    const std::vector<Event>& events, EnvironmentModel& environmentModel,
    DispatchPolicy& dispatchPolicy)
    : state_(initialState), events_(events), environment_(environmentModel), dispatchPolicy_(dispatchPolicy) {}

void Simulator::run() {
    spdlog::info("[Simulator] Starting simulation at system time: {}", state_.getSystemTime());

    // Use index-based loop to allow safe modification of events_
    for (size_t i = 0; i < events_.size(); ++i) {
        const Event& event = events_[i];

        // Process the event
        environment_.handleEvent(state_, event);
        Action action = dispatchPolicy_.getAction(state_);
        std::vector<Event> newEvents = environment_.takeAction(state_, action);

        // Add new events to the end of the vector
        events_.insert(events_.end(), newEvents.begin(), newEvents.end());

        // Optionally, sort if you want to keep events_ ordered by event_time
        sortEventsByTime(events_);

        // Store the current state for historical tracking
        // TODO: I probably just need the last state.
        state_history_.push_back(state_);
    }

    spdlog::info("Number of events unresolved: {}", state_.getIncidentQueue().size());
    spdlog::info("Number of events addressed: {}", state_.getActiveIncidents().size());
    

    spdlog::info("[Simulator] Simulation complete.");
    spdlog::info("[Simulator] Final system time: {}", state_.getSystemTime());
}

// TODO: Storing and saving activeIncidents may be expensive when there are many incidents.
void Simulator::replay() {
    spdlog::info("[Simulator] Replaying simulation...");

    // for (const auto& state : state_history_) {
    //     spdlog::debug("Replaying state at system time: {}", state.getSystemTime());
    //     // spdlog::info("Addressing incidents: {} at time {}", state.getUnresolvedIncident().incident_id, state.getSystemTime());
    //     // Additional logic to display or process the state can be added here
    // }
    State state = state_history_.back();
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    for (const auto& [id, incident] : activeIncidents) {
        // spdlog::info("Incident ID: {}, Type: {}, Level: {}, Lat: {}, Lon: {}, Report Time: {}, Response Time: {}, Resolved Time: {}",
        //              incident.incident_id, incident.incident_type, to_string(incident.incident_level),
        //              incident.lat, incident.lon, incident.reportTime, incident.responseTime, incident.resolvedTime);
        spdlog::info("Incident ID: {}, Reported: {}, Responded: {}, Resolved: {}, TravelTime: {}", 
                     incident.incident_id, 
                     formatTime(incident.reportTime), 
                     formatTime(incident.responseTime), 
                     formatTime(incident.resolvedTime),
                     incident.oneWayTravelTimeTo);
    }

    spdlog::info("[Simulator] Replay complete.");
}

const std::vector<State>& Simulator::getStateHistory() const {
    return state_history_;
}