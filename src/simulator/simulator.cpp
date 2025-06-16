#include "simulator/simulator.h"
#include <algorithm>
#include <iostream>
#include <spdlog/spdlog.h>

Simulator::Simulator(State& initialState, 
    const std::vector<Event>& events, EnvironmentModel& environmentModel,
    DispatchPolicy& dispatchPolicy)
    : state_(initialState), events_(events), environment_(environmentModel), dispatchPolicy_(dispatchPolicy) {}

void Simulator::run() {
    spdlog::info("[Simulator] Starting simulation at system time: {}", state_.getSystemTime());

    for (const auto& event : events_) {
        environment_.handleEvent(state_, event);
        int nextIncident = dispatchPolicy_.getAction(state_);
        // environment_.takeAction(state_, nextIncident);
        spdlog::debug("Incident ID: {}", nextIncident);
        state_history_.push_back(state_);  // Store the current state after handling the event
    }

    spdlog::info("[Simulator] Simulation complete.");
    spdlog::info("[Simulator] Final system time: {}", state_.getSystemTime());
}

void Simulator::replay() {
    spdlog::info("[Simulator] Replaying simulation...");

    for (const auto& state : state_history_) {
        spdlog::debug("Replaying state at system time: {}", state.getSystemTime());
        // Additional logic to display or process the state can be added here
    }

    spdlog::info("[Simulator] Replay complete.");
}