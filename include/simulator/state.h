#ifndef STATE_H
#define STATE_H

#include <ctime>
#include <unordered_map>
#include <vector>
#include <memory>
#include <queue>
#include "data/incident.h"
#include "data/station.h"
#include "data/fire_truck.h"

class State {
public:
    State();

    void advanceTime(std::time_t new_time);
    void addIncident(Incident incident);

    std::time_t getSystemTime() const;
    const std::queue<Incident>& getRespondedIncidents() const;
    Station& getStation(int station_id);
    std::vector<Station>& getAllStations();
    FireTruck& getFireTruck(int fire_truck_id);
    std::unordered_map<int, FireTruck>& getAllFireTrucks();
    void addStations(std::vector<Station> stations);
    Incident getUnresolvedIncident();
    
private:
    std::time_t system_time_;
    std::queue<Incident> queuedIncidents_;
    std::vector<Station> stations_;
    std::unordered_map<int, FireTruck> fire_trucks_;
    std::vector<Incident> addressedIncidents_;
};

#endif // STATE_H
