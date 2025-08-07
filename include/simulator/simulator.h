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
        EventQueue& events, 
        EnvironmentModel& environmentModel,
        DispatchPolicy& dispatchPolicy
    );
    void run();
    const std::vector<State>& getStateHistory() const;
    const std::vector<Action>& getActionHistory() const;
    State& getCurrentState();

    void writeActions();
    void writeReportToCSV();

private:
    State& state_;
    EventQueue& events_;
    EnvironmentModel& environment_;
    DispatchPolicy& dispatchPolicy_;
    std::vector<State> state_history_;
    std::vector<Action> action_history_;
};

#endif // SIMULATOR_H
