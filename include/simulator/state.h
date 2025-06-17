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
    void addToQueue(Incident incident);
    std::time_t getSystemTime() const;
    std::queue<Incident>& getIncidentQueue();
    Station& getStation(int station_id);
    const std::vector<Station>& getAllStations() const;
    FireTruck& getFireTruck(int fire_truck_id);
    const std::unordered_map<int, FireTruck>& getAllFireTrucks() const;
    void addStations(std::vector<Station> stations);
    Incident getEarliestUnresolvedIncident() const;
    std::unordered_map<int, Incident>& getActiveIncidents();
    
private:
    std::time_t system_time_;
    std::queue<Incident> incidentQueue_;
    std::vector<Station> stations_;
    std::unordered_map<int, FireTruck> fire_trucks_;
    std::unordered_map<int, Incident> activeIncidents_;
};

#endif // STATE_H
