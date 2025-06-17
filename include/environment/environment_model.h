#ifndef ENVIRONMENT_MODEL_H
#define ENVIRONMENT_MODEL_H

#include "simulator/event.h"
#include "simulator/state.h"
#include "simulator/action.h"

class EnvironmentModel {
public:
    EnvironmentModel() = default;

    // Handle an event and update state
    void handleEvent(State& state, const Event& event);
    std::vector<Event> takeAction(State& state, const Action& action);
};

// create a function header for a new IncidentResolution event
void appendNewEvents(State& state, const Action& action, std::vector<Event>& newEvents);
time_t calculateResolutionTime(IncidentLevel incidentLevel);
#endif // ENVIRONMENT_MODEL_H
