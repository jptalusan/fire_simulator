#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "simulator/event.h"
#include "simulator/state.h"
#include "policy/dispatch_policy.h"
#include "models/incident_model.h"
#include "environment/environment_model.h"
#include <vector>

struct StepResult {
    State& state;  // Reference instead of copy
    double reward;
    bool done;
    std::unordered_map<std::string, bool> info; //optional metadata
    
    // Constructor to initialize the reference
    StepResult(State& s, double r = 0.0, bool d = false, const std::unordered_map<std::string, bool>& i = {})
        : state(s), reward(r), done(d), info(i) {}
};


class Simulator {
public:
    Simulator(State& initialState, 
        IncidentModel& incidentModel, 
        EnvironmentModel& environmentModel,
        DispatchPolicy& dispatchPolicy
    );
    // void run();
    // const std::vector<State>& getStateHistory() const;
    // const std::vector<Action>& getActionHistory() const;
    // const std::vector<FireStation>& getStationHistory() const;
    // State& getCurrentState();

    // void writeActions();
    // void writeReportToCSV();
    // void setNextEvent();
    // const Event* getNextEvent() const;
    StepResult step(const std::vector<Action>& actions);
    State& simulate_time_step(time_t new_time);
    State& reset();

private:
    State& state_;
    EnvironmentModel& environment_;
    DispatchPolicy& dispatchPolicy_;
    IncidentModel& incidentModel_;
    std::vector<State> state_history_;
    std::vector<FireStation> station_history_;
    std::vector<Action> action_history_;
};

#endif // SIMULATOR_H
