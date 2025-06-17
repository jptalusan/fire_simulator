#include "simulator/state.h"
#include <iostream>

State::State() : system_time_(std::time(nullptr)) {}

void State::advanceTime(std::time_t new_time) {
    system_time_ = new_time;
    // Optionally, update truck timers here.
}

void State::addToQueue(Incident incident) {
    incidentQueue_.push(incident);
}

std::time_t State::getSystemTime() const {
    return system_time_;
}

std::queue<Incident>& State::getIncidentQueue() {
    return incidentQueue_;
}

Station& State::getStation(int station_id) {
    return stations_[station_id];
}

const std::vector<Station>& State::getAllStations() const {
    return stations_;
}

std::unordered_map<int, Incident>& State::getActiveIncidents() {
    return activeIncidents_;
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
    stations_ = std::move(stations);
}

Incident State::getEarliestUnresolvedIncident() const {
/*
This function retrieves the next unresolved incident from the queue.
*/
    if (!incidentQueue_.empty()) {
        Incident next = incidentQueue_.front();
        return next;
    }
    // Handle the case where the queue is empty (could throw, return default, or handle as needed)
    return Incident();
}

// std::vector<Incident> State::getAddressedIncidents() {
//     return addressedIncidents_;
// }

// void State::addToAddressedIncidents(Incident incident) {
//     addressedIncidents_.push_back(incident);
// }