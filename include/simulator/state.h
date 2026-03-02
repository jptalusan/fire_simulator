#ifndef STATE_H
#define STATE_H

#include <ctime>
#include <unordered_map>
#include <vector>
#include <optional>
#include "objects/incident.h"
#include "objects/firestation.h"
#include "objects/vehicle.h"
#include "objects/hospital.h"

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

    // Hospital management
    void setHospitals(std::vector<Hospital> hospitals);
    const std::vector<Hospital>& getHospitals() const;
    const Hospital& getHospital(int index) const;
    int findNearestHospital(const Location& location) const;
    bool hasHospitals() const { return !hospitals_.empty(); }

private:
    std::time_t system_time_;
    std::vector<FireStation> stations_;
    std::vector<Vehicle> vehicleList_;
    std::unordered_map<int, Incident> activeIncidents_;
    std::vector<std::string> stationMetrics_;
    std::vector<Hospital> hospitals_;
};

#endif // STATE_H
