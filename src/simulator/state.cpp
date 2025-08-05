#include "simulator/state.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include "utils/helpers.h"

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

void State::setApparatusList(const std::vector<Apparatus>& apparatusList) {
    apparatusList_ = apparatusList;
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

// In state.cpp
std::vector<int> State::dispatchApparatus(ApparatusType type, int count, int stationIndex) {
    // Find and update individual apparatus
    std::vector<int> dispatchedIds;
    int dispatched = 0;
    
    for (auto& apparatus : apparatusList_) {
        if (apparatus.getStationIndex() == stationIndex && 
            apparatus.getType() == type && 
            apparatus.getStatus() == ApparatusStatus::Available &&
            dispatched < count) {
            
            apparatus.setStatus(ApparatusStatus::Dispatched);
            dispatchedIds.push_back(apparatus.getId());
            dispatched++;
        }
    }
    
    return dispatchedIds;
}

void State::returnApparatus(ApparatusType type, int count, const std::vector<int>& apparatusIds) {
    if (static_cast<size_t>(count) != apparatusIds.size()) {
        spdlog::warn("[{}] Mismatch in returnApparatus: count {} vs apparatusIds size {}", 
                     formatTime(system_time_), count, apparatusIds.size());
    }
    for (int id : apparatusIds) {
        auto it = std::find_if(apparatusList_.begin(), apparatusList_.end(),
                               [id](const Apparatus& a) { return a.getId() == id; });
        if (it != apparatusList_.end()) {
            if (it->getType() != type) {
                spdlog::warn("[{}] Apparatus ID {} type mismatch: expected {}, found {}", 
                             formatTime(system_time_), id, to_string(type), to_string(it->getType()));
            }
            it->setStatus(ApparatusStatus::Available);
            spdlog::info("[{}] Apparatus {} returned to available status", formatTime(system_time_), id);
        } else {
            spdlog::warn("[{}] Apparatus ID {} not found in the list", formatTime(system_time_), id);
        }
    }
}