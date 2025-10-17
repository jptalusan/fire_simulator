#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "simulator/event.h"
#include "simulator/state.h"
#include "policy/dispatch_policy.h"
#include "models/incident_model.h"
#include "environment/environment_model.h"
#include "models/travel_time_model.h"
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
        TravelTimeModel& travelTimeModel,
        EnvironmentModel& environmentModel,
        DispatchPolicy& dispatchPolicy
    );
    
    StepResult step(const std::vector<Action>& actions);
    State& simulate_time_step(time_t new_time);
    State& reset();
    void logState(const State& state);
    void logActions(const std::vector<Action>& actions, time_t current_time);
    std::unordered_map<int, Incident> doneIncidents_;
    void writeActionReport(const State& state) const;
    void writeIncidentReport() const;
    void writeVehicleReport() const;

private:
    State& state_;
    EnvironmentModel& environment_;
    DispatchPolicy& dispatchPolicy_;
    IncidentModel& incidentModel_;
    TravelTimeModel& travelTimeModel_;
    std::vector<std::vector<Vehicle>> vehicles_history_;
    std::vector<std::vector<FireStation>> stations_history_;
    std::vector<std::pair<time_t, std::vector<Action>>> actions_history_;
    std::vector<time_t> state_times_history_;
};

#endif // SIMULATOR_H
