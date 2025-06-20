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

        std::vector<Event> handleEvents = environment_.handleEvent(state_, event);
        std::vector<Action> actions = dispatchPolicy_.getAction(state_);
        std::vector<Event> newEvents = environment_.takeActions(state_, actions);

        events_.insert(events_.end(), handleEvents.begin(), handleEvents.end());
        events_.insert(events_.end(), newEvents.begin(), newEvents.end());
        
        sortEventsByTimeAndType(events_);
    }

    spdlog::debug("Number of events addressed: {}", state_.getActiveIncidents().size());
}

const std::vector<State>& Simulator::getStateHistory() const {
    return state_history_;
}

State& Simulator::getCurrentState() {
    return state_;
}