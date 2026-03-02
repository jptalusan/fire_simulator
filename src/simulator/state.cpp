#include "simulator/state.h"
#include "utils/helpers.h"
#include "utils/logger.h"
#include <limits>

State::State() : system_time_(std::time(nullptr)) {}

void State::advanceTime(std::time_t new_time) {
    system_time_ = new_time;
    // Optionally, update truck timers here.
}

std::time_t State::getSystemTime() const {
    return system_time_;
}

FireStation& State::getStation(int stationIndex) {
    return stations_[stationIndex];
}

const std::vector<FireStation>& State::getAllStations() const {
    return stations_;
}

std::vector<FireStation>& State::getAllStations_() {
    return stations_;
}

std::vector<Vehicle>& State::getVehicleList() {
    return vehicleList_;
}

const std::vector<Vehicle>& State::getConstVehicleList() const {
    return vehicleList_;
}

void State::setVehicleList(const std::vector<Vehicle>& vehicleList) {
    vehicleList_ = vehicleList;
}

void State::addStations(std::vector<FireStation> stations) {
    stations_ = std::move(stations);
}

void State::addStation(const FireStation& station) {
    stations_.push_back(station);
}

void State::addVehicle(const Vehicle& vehicle) {
    vehicleList_.push_back(vehicle);
}

std::unordered_map<int, Incident>& State::getActiveIncidents() {
    return activeIncidents_;
}

const std::unordered_map<int, Incident>& State::getActiveIncidentsConst() const {
    return activeIncidents_;
}

void State::updateStationMetrics(const std::string& metric) {
    stationMetrics_.push_back(metric);
}

std::vector<std::string> State::getStationMetrics() const {
    return stationMetrics_;
}

// In state.cpp
std::vector<int> State::dispatchApparatus(ApparatusType type, int count, int stationIndex) {
    // Find and update individual apparatus
    std::vector<int> dispatchedIds;
    int dispatched = 0;

    for (auto& vehicle : vehicleList_) {
        if (vehicle.getStationIndex() == stationIndex &&
            vehicle.getType() == type &&
            vehicle.getStatus() == ApparatusStatus::Available &&
            dispatched < count) {
            
            vehicle.setStatus(ApparatusStatus::Dispatched);
            dispatchedIds.push_back(vehicle.getVehicleId());
            dispatched++;
        }
    }
    
    return dispatchedIds;
}

void State::setHospitals(std::vector<Hospital> hospitals) {
    hospitals_ = std::move(hospitals);
}

const std::vector<Hospital>& State::getHospitals() const {
    return hospitals_;
}

const Hospital& State::getHospital(int index) const {
    return hospitals_.at(index);
}

int State::findNearestHospital(const Location& location) const {
    int nearestIndex = -1;
    double minDist = std::numeric_limits<double>::max();
    for (size_t i = 0; i < hospitals_.size(); ++i) {
        const auto& hLoc = hospitals_[i].getLocation();
        double dLat = hLoc.lat - location.lat;
        double dLon = hLoc.lon - location.lon;
        double dist = dLat * dLat + dLon * dLon;
        if (dist < minDist) {
            minDist = dist;
            nearestIndex = static_cast<int>(i);
        }
    }
    return nearestIndex;
}

void State::returnApparatus(ApparatusType type, int count, const std::vector<int>& apparatusIds) {
    for (int id : apparatusIds) {
        for (auto& vehicle : vehicleList_) {
            if (vehicle.getVehicleId() == id) {
                vehicle.setStatus(ApparatusStatus::Available);
                break;
            }
        }
    }
}