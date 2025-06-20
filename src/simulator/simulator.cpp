#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include "simulator/action.h"
#include "simulator/simulator.h"
#include "utils/helpers.h"

Simulator::Simulator(State& initialState, 
    const std::vector<Event>& events, EnvironmentModel& environmentModel,
    DispatchPolicy& dispatchPolicy)
    : state_(initialState), events_(events), environment_(environmentModel), dispatchPolicy_(dispatchPolicy) {}

void Simulator::run() {
    spdlog::info("[{}] Starting simulation.", formatTime(state_.getSystemTime()));

    // Use index-based loop to allow safe modification of events_
    // Avoiding invalidating iterators by always creating new events that are at least a minute in the future.
    for (size_t i = 0; i < events_.size(); ++i) {
        const Event& event = events_[i];

        environment_.handleEvent(state_, event);
        std::vector<Action> actions = dispatchPolicy_.getAction(state_);
        std::vector<Event> newEvents = environment_.takeActions(state_, actions);

        if (newEvents.empty()) {
            spdlog::debug("[Simulator] No new events generated from action: {}", actions[0].toString());
            continue; // Skip to the next iteration if no new events
        }
        // Add new events to the end of the vector (you should only either add an incident with a later time, or a resolution/station event with the same current time)
        // This is important to avoid modifying the vector while iterating over it.
        events_.insert(events_.end(), newEvents.begin(), newEvents.end());

        // Optionally, sort if you want to keep events_ ordered by event_time
        sortEventsByTimeAndType(events_);
    }

    // Store the current state for historical tracking
    // TODO: I probably just need the last state.
    // state_history_.push_back(state_);
    spdlog::debug("Number of events addressed: {}", state_.getActiveIncidents().size());
}

// TODO: Storing and saving activeIncidents may be expensive when there are many incidents.
void Simulator::replay() {
    spdlog::info("[Simulator] Replaying simulation...");
    State state = state_history_.back();
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    for (const auto& [id, incident] : activeIncidents) {
        spdlog::info("Incident ID: {}, Reported: {}, Responded: {}, Resolved: {}, TravelTime: {}", 
                     incident.incidentIndex, 
                     formatTime(incident.reportTime), 
                     formatTime(incident.timeRespondedTo), 
                     formatTime(incident.resolvedTime),
                     incident.oneWayTravelTimeTo);
    }

    spdlog::info("[Simulator] Replay complete.");
}

const std::vector<State>& Simulator::getStateHistory() const {
    return state_history_;
}

State& Simulator::getCurrentState() {
    return state_;
}