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

std::vector<Apparatus>& State::getApparatusList() {
    return apparatusList_;
}

void State::addStations(std::vector<Station> stations) {
    stations_ = std::move(stations);
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

void State::updateStationMetrics(const std::string& metric) {
    stationMetrics_.push_back(metric);
}

std::vector<std::string> State::getStationMetrics() const {
    return stationMetrics_;
}
