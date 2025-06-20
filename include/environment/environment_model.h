#ifndef ENVIRONMENT_MODEL_H
#define ENVIRONMENT_MODEL_H

#include "simulator/event.h"
#include "simulator/state.h"
#include "simulator/action.h"
#include "models/fire.h"

class EnvironmentModel {
public:
    EnvironmentModel(FireModel& fireModel);

    // Handle an event and update state
    void handleEvent(State& state, const Event& event);
    std::vector<Event> takeActions(State& state, const std::vector<Action>& actions);
    // create a function header for a new IncidentResolution event
    void generateStationEvents(State& state, const std::vector<Action>& actions, std::vector<Event>& newEvents);
    time_t generateIncidentResolutionEvent(State& state, const Incident& incident, std::vector<Event>& newEvents);
    int calculateApparatusCount(const Incident& incident);
    double calculateResolutionTime(const Incident& incident);
    void handleIncident(State& state, Incident& incident, time_t eventTime);

private:
    FireModel& fireModel_;
};

#endif // ENVIRONMENT_MODEL_H
