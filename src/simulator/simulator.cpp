#include "simulator/simulator.h"
#include "simulator/action.h"
#include "utils/constants.h"
#include "utils/helpers.h"
#include <spdlog/spdlog.h>

Simulator::Simulator(State &initialState, EventQueue &events,
                     EnvironmentModel &environmentModel,
                     DispatchPolicy &dispatchPolicy)
    : state_(initialState), events_(events), environment_(environmentModel),
      dispatchPolicy_(dispatchPolicy) {}

void Simulator::run() {
  spdlog::info("[{}] Starting simulation.", formatTime(state_.getSystemTime()));

  while (!events_.empty()) {
    // Process the first event in the sorted list
    Event currentEvent = events_.top();

    std::vector<Event> handleEvents =
        environment_.handleEvent(state_, currentEvent);

    events_.pop();
    std::vector<Action> actions = dispatchPolicy_.getAction(state_);

    std::vector<Event> newEvents = environment_.takeActions(state_, actions);

    for (const auto& event : handleEvents) {
        events_.push(event);
    }
    for (const auto& event : newEvents) {
        events_.push(event);
    }
  }

  spdlog::debug("Number of events addressed: {}",
                state_.getActiveIncidents().size());
}

const std::vector<State> &Simulator::getStateHistory() const {
  return state_history_;
}

State &Simulator::getCurrentState() { return state_; }
