#ifndef STATE_H
#define STATE_H

#include <ctime>
#include <unordered_map>
#include <vector>
#include "objects/incident.h"
#include "objects/location.h"
#include "objects/vehicle.h"

class State {
public:
    State();

    void advanceTime(std::time_t new_time);
    std::time_t getSystemTime() const;
    FireStation& getStation(int stationIndex);
    const std::vector<FireStation>& getAllStations() const;
    std::vector<Vehicle>& getVehicleList();
    const std::vector<Vehicle>& getConstVehicleList() const;
    void setVehicleList(const std::vector<Vehicle>& vehicleList);
    void addStations(std::vector<FireStation> stations);
    void addStation(const FireStation& station);
    void addVehicle(const Vehicle& vehicle);
    std::unordered_map<int, Incident>& getActiveIncidents();
    void updateStationMetrics(const std::string& metric);
    std::vector<std::string> getStationMetrics() const;
    const std::unordered_map<int, Incident>& getActiveIncidentsConst() const;
    std::vector<FireStation>& getAllStations_();

    // TODO: Improve this, ignored stations and incidents should be handled better
    std::vector<int> ignoredStations;
    std::vector<int> ignoredIncidents;
    std::unordered_map<std::string, int> stationIndexMap_; // Maps station address to index
    std::vector<int> inProgressIncidentIndices;
    std::vector<int> dispatchApparatus(ApparatusType type, int count, int stationIndex);
    void returnApparatus(ApparatusType type, int count, const std::vector<int>& apparatusIds);
    
    std::optional<Incident> newIncident_ = std::nullopt;
private:
    std::time_t system_time_;
    std::vector<FireStation> stations_;
    std::vector<Vehicle> vehicleList_;
    std::unordered_map<int, Incident> activeIncidents_;
    std::vector<std::string> stationMetrics_;
};

#endif // STATE_H
