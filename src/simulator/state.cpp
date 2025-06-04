#include "simulator/state.h"
#include <iostream>

State::State() : system_time_(0) {}

void State::advanceTime(std::time_t new_time) {
    system_time_ = new_time;
    // Optionally, update truck timers here.
}

void State::addRespondedIncident(int incident_id) {
    responded_incidents_.insert(incident_id);
}

std::time_t State::getSystemTime() const {
    return system_time_;
}

const std::unordered_set<int>& State::getRespondedIncidents() const {
    return responded_incidents_;
}

std::vector<Station>& State::getStations() {
    return stations_;
}

std::vector<FireTruck>& State::getFireTrucks() {
    return fire_trucks_;
}
