#ifndef STATE_H
#define STATE_H

#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include "data/incident.h"
#include "data/station.h"
#include "data/fire_truck.h"

class State {
public:
    State();

    void advanceTime(std::time_t new_time);
    std::time_t getSystemTime() const;
    Station& getStation(int stationIndex);
    const std::vector<Station>& getAllStations() const;
    FireTruck& getFireTruck(int fire_truck_id);
    const std::unordered_map<int, FireTruck>& getAllFireTrucks() const;
    void addStations(std::vector<Station> stations);
    Incident getEarliestUnresolvedIncident() const;
    std::unordered_map<int, Incident>& getActiveIncidents();
    void updateStationMetrics(const std::string& metric);
    std::vector<std::string> getStationMetrics() const;
    const std::unordered_map<int, Incident>& getActiveIncidentsConst() const;

    // TODO: Improve this, ignored stations and incidents should be handled better
    std::vector<int> ignoredStations;
    std::vector<int> ignoredIncidents;
    std::unordered_map<int, Incident> doneIncidents_;
    std::unordered_map<std::string, int> stationIndexMap_; // Maps station address to index
    std::vector<int> inProgressIncidentIndices;
    const std::unordered_map<int, Incident>& getAllIncidents() const;
    void populateAllIncidents(const std::vector<Incident>& incidents);
    
private:
    std::time_t system_time_;
    std::vector<Station> stations_;
    std::unordered_map<int, FireTruck> fire_trucks_;
    std::unordered_map<int, Incident> activeIncidents_;
    std::vector<std::string> stationMetrics_;
    int lastEventId = 0; // Used to generate unique event IDs
    std::unordered_map<int, Incident> allIncidents_; //policies should not access this
};

#endif // STATE_H
