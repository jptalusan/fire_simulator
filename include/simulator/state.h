#ifndef STATE_H
#define STATE_H

#include <ctime>
#include <unordered_map>
#include <vector>
#include "data/incident.h"
#include "data/station.h"
#include "data/apparatus.h"

class State {
public:
    State();

    void advanceTime(std::time_t new_time);
    std::time_t getSystemTime() const;
    Station& getStation(int stationIndex);
    const std::vector<Station>& getAllStations() const;
    std::vector<Apparatus>& getApparatusList();
    void setApparatusList(const std::vector<Apparatus>& apparatusList);
    void addStations(std::vector<Station> stations);
    void addStation(const Station& station);
    void addApparatus(const Apparatus& apparatus);
    std::unordered_map<int, Incident>& getActiveIncidents();
    void updateStationMetrics(const std::string& metric);
    std::vector<std::string> getStationMetrics() const;
    const std::unordered_map<int, Incident>& getActiveIncidentsConst() const;
    std::vector<Station>& getAllStations_();

    // TODO: Improve this, ignored stations and incidents should be handled better
    std::vector<int> ignoredStations;
    std::vector<int> ignoredIncidents;
    std::unordered_map<int, Incident> doneIncidents_;
    std::unordered_map<std::string, int> stationIndexMap_; // Maps station address to index
    std::vector<int> inProgressIncidentIndices;
    const std::unordered_map<int, Incident>& getAllIncidents() const;
    void populateAllIncidents(const std::vector<Incident>& incidents);
    std::vector<int> dispatchApparatus(ApparatusType type, int count, int stationIndex);
    void returnApparatus(ApparatusType type, int count, const std::vector<int>& apparatusIds);
    void matchApparatusesWithStations();

private:
    std::time_t system_time_;
    std::vector<Station> stations_;
    std::vector<Apparatus> apparatusList_;
    std::unordered_map<int, Incident> activeIncidents_;
    std::vector<std::string> stationMetrics_;
    std::unordered_map<int, Incident> allIncidents_; //policies should not access this
};

#endif // STATE_H
