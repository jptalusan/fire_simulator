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
    
    for (const auto& action : actions) {
      if (action.type == StationActionType::DoNothing) {
        continue;
      }
      action_history_.push_back(action);
    }
  }

  spdlog::debug("Number of events addressed: {}", state_.getActiveIncidents().size());
}

const std::vector<State> &Simulator::getStateHistory() const {
  return state_history_;
}

const std::vector<Action> &Simulator::getActionHistory() const {
  return action_history_;
}

State &Simulator::getCurrentState() { return state_; }
