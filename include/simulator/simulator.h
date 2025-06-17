#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "simulator/event.h"
#include "simulator/state.h"
#include "policy/dispatch_policy.h"
#include "environment/environment_model.h"
#include <vector>

class Simulator {
public:
    Simulator(State& initialState, 
        const std::vector<Event>& events, 
        EnvironmentModel& environmentModel,
        DispatchPolicy& dispatchPolicy
    );
    void run();
    void replay();
    const std::vector<State>& getStateHistory() const;

private:
    State& state_;
    std::vector<Event> events_;
    EnvironmentModel& environment_;
    DispatchPolicy& dispatchPolicy_;
    std::vector<State> state_history_;
};

#endif // SIMULATOR_H
