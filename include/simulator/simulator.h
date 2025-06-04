#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "simulator/event.h"
#include "simulator/state.h"
#include "environment/environment_model.h"
#include <vector>

class Simulator {
public:
    Simulator(State& initialState, const std::vector<Event>& events, EnvironmentModel& environmentModel);
    void run();
    void replay();

private:
    State& state_;
    EnvironmentModel& environment_;
    std::vector<Event> events_;
    std::vector<State> state_history_;
};

#endif // SIMULATOR_H
