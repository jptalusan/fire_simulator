#ifndef ENVIRONMENT_MODEL_H
#define ENVIRONMENT_MODEL_H

#include "simulator/event.h"
#include "simulator/state.h"

class EnvironmentModel {
public:
    EnvironmentModel() = default;

    // Handle an event and update state
    void handleEvent(State& state, const Event& event);
};

#endif // ENVIRONMENT_MODEL_H
