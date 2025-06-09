#ifndef STATE_H
#define STATE_H

#include <ctime>
#include <vector>
#include <memory>
#include <unordered_set>

#include "data/station.h"
#include "data/fire_truck.h"

class State {
public:
    State();

    void advanceTime(std::time_t new_time);
    void addRespondedIncident(int incident_id);

    std::time_t getSystemTime() const;
    const std::unordered_set<int>& getRespondedIncidents() const;
    std::vector<Station>& getStations();
    std::vector<FireTruck>& getFireTrucks();

private:
    std::time_t system_time_;
    std::unordered_set<int> responded_incidents_;
    std::vector<Station> stations_;
    std::vector<FireTruck> fire_trucks_;
};

#endif // STATE_H
