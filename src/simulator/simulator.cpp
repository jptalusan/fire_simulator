#include "simulator/simulator.h"
#include <algorithm>
#include <iostream>

Simulator::Simulator(State& initialState, 
    const std::vector<Event>& events, EnvironmentModel& environmentModel,
    DispatchPolicy& dispatchPolicy)
    : state_(initialState), events_(events), environment_(environmentModel), dispatchPolicy_(dispatchPolicy) {}

void Simulator::run() {
    std::cout << "[Simulator] Starting simulation at system time: " << state_.getSystemTime() << "\n";
    
    for (const auto& event : events_) {
        environment_.handleEvent(state_, event);
        int nextIncident = dispatchPolicy_.getAction(state_);
        std::cout << "Incident ID:" << nextIncident << std::endl;
        state_history_.push_back(state_);  // Store the current state after handling the event
    }

    std::cout << "[Simulator] Simulation complete.\n";
    std::cout << "[Simulator] Final system time: " << state_.getSystemTime() << "\n";
}

void Simulator::replay() {
    std::cout << "[Simulator] Replaying simulation...\n";
    
    for (const auto& state : state_history_) {
        std::cout << "Replaying state at system time: " << state.getSystemTime() << "\n";
        // Additional logic to display or process the state can be added here
    }

    std::cout << "[Simulator] Replay complete.\n";
}