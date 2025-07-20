#include "simulator/state.h"
#include <iostream>

State::State() : system_time_(std::time(nullptr)) {}

void State::advanceTime(std::time_t new_time) {
    system_time_ = new_time;
    // Optionally, update truck timers here.
}

std::time_t State::getSystemTime() const {
    return system_time_;
}

Station& State::getStation(int stationIndex) {
    return stations_[stationIndex];
}

const std::vector<Station>& State::getAllStations() const {
    return stations_;
}

std::unordered_map<int, Incident>& State::getActiveIncidents() {
    return activeIncidents_;
}

const std::unordered_map<int, Incident>& State::getActiveIncidentsConst() const {
    return activeIncidents_;
}

const std::unordered_map<int, Incident>& State::getAllIncidents() const {
    return allIncidents_;
}

// Runs once to store all incidents for faster reference later.
void State::populateAllIncidents(const std::vector<Incident>& incidents) {
    allIncidents_.reserve(incidents.size()); // Preallocate memory for efficiency
    for (const auto& incident : incidents) {
        allIncidents_[incident.incidentIndex] = incident;
    }
}

/*
You are default-constructing a FireTruck
(e.g., FireTruck truck; or std::unordered_map<int, FireTruck> fire_trucks_; 
will default-construct elements), 
but your FireTruck class does not have a default constructor 
(i.e., a constructor with no arguments).
*/
FireTruck& State::getFireTruck(int fire_truck_id) {
    auto it = fire_trucks_.find(fire_truck_id);
    if (it == fire_trucks_.end()) {
        // TODO: Check if this makes sense: If the fire truck does not exist, create it
        fire_trucks_[fire_truck_id] = FireTruck();
    }
    return fire_trucks_[fire_truck_id];
}

const std::unordered_map<int, FireTruck>& State::getAllFireTrucks() const {
    return fire_trucks_;
}

void State::addStations(std::vector<Station> stations) {
    // TODO: Clean this up, unnecessary loops maybe?
    for (const auto& station : stations) {
        stationIndexMap_.emplace(station.getFacilityName(), station.getStationIndex());
    }
    stations_ = std::move(stations);
}

void State::updateStationMetrics(const std::string& metric) {
    stationMetrics_.push_back(metric);
}

std::vector<std::string> State::getStationMetrics() const {
    return stationMetrics_;
}