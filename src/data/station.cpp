#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "data/station.h"
#include "data/geometry.h"
#include "config/EnvLoader.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/logger.h"

// TODO: Need to change the name of the stationID to be unique and incrementally start from 0...N
Station::Station(int stationIndex, //simulator internal ID, not the one from the CSV
                 int station_id,
                 int num_fire_trucks,
                 int num_ambulances,
                 double lon,
                 double lat)
    : stationIndex(stationIndex),
      station_id(station_id),
      num_fire_trucks(num_fire_trucks),
      num_ambulances(num_ambulances),
      max_ambulances(num_ambulances),
      max_fire_trucks(num_fire_trucks),
      lon(lon),
      lat(lat) {}

int Station::getStationIndex() const { return stationIndex; }
int Station::getStationId() const noexcept { return station_id; }
int Station::getNumFireTrucks() const noexcept { return num_fire_trucks; }
int Station::getNumAmbulances() const noexcept { return num_ambulances; }
double Station::getLon() const noexcept { return lon; }
double Station::getLat() const noexcept { return lat; }
Location Station::getLocation() const noexcept {
    return Location(lat, lon);
}

// Make sure we don't exceed the maximum allowed fire trucks or ambulances
// TODO: Right now we consider the number in the CSV as the max as well, but this can be changed
void Station::setNumFireTrucks(int n) { 
    if (n <= max_fire_trucks) {
        num_fire_trucks = n;
    }
    else {
        LOG_WARN("Attempted to set number of fire trucks to {} but max is {}", n, max_fire_trucks);
    }
}

void Station::setNumAmbulances(int n) {
    if (n <= max_ambulances) {
        num_ambulances = n;
    }
    else {
        LOG_WARN("Attempted to set number of ambulances to {} but max is {}", n, max_ambulances);
    }
}

void Station::printInfo() const {
    LOG_DEBUG("Station ID: {}, Lat: {}, Lon: {}, Fire Trucks: {}, Ambulances: {}",
                station_id, lat, lon, num_fire_trucks, num_ambulances);
}

// Fast lookups - O(1)
int Station::getAvailableCount(ApparatusType type) const {
    auto it = available_count_.find(type);
    return it != available_count_.end() ? it->second : 0;
}

int Station::getTotalCount(ApparatusType type) const {
    auto it = total_count_.find(type);
    return it != total_count_.end() ? it->second : 0;
}

// Update counts when apparatus status changes
void Station::updateApparatusStatus(ApparatusType type, ApparatusStatus old_status, ApparatusStatus new_status) {
    if (old_status == ApparatusStatus::Available) {
        available_count_[type]--;
    }
    if (new_status == ApparatusStatus::Available) {
        available_count_[type]++;
    }
}

// In station.cpp - cleaner implementation:
bool Station::dispatchApparatus(ApparatusType type, int count) {
    if (available_count_[type] < count) {
        LOG_WARN("Station {} cannot dispatch {} {} apparatus - only {} available", 
                    stationIndex, count, to_string(type), available_count_[type]);
        return false;
    }

    for (int i = 0; i < count; ++i) {
        updateApparatusStatus(type, ApparatusStatus::Available, ApparatusStatus::Dispatched);
    }
    LOG_DEBUG("Station {} dispatched {} {} apparatus. Remaining: {}", 
                 stationIndex, count, to_string(type), available_count_[type]);
    return true;
}

void Station::returnApparatus(ApparatusType type, int count) {
    // Ensure we don't exceed total capacity
    int maxReturnable = total_count_[type] - available_count_[type];
    int actualReturn = std::min(count, maxReturnable);
    
    if (actualReturn != count) {
        LOG_WARN("Station {} can only accept {} of {} returning {} apparatus", 
                    stationIndex, actualReturn, count, to_string(type));
    }

    for (int i = 0; i < actualReturn; ++i) {
        updateApparatusStatus(type, ApparatusStatus::Dispatched, ApparatusStatus::Available);
    }
    LOG_DEBUG("Station {} received {} returning {} apparatus. Available: {}", 
                 stationIndex, actualReturn, to_string(type), available_count_[type]);
}

bool Station::canDispatch(ApparatusType type, int count) const {
    return available_count_.count(type) && available_count_.at(type) >= count;
}

void Station::updateTotalCount(ApparatusType type, int count) {
    total_count_[type] += count;
    if (total_count_[type] < 0) {
        LOG_ERROR("Total count for {} at station {} cannot be negative. Resetting to 0.", 
                      to_string(type), stationIndex);
        total_count_[type] = 0;
    }
}

void Station::updateAvailableCount(ApparatusType type, int count) {
    available_count_[type] += count;
    if (available_count_[type] < 0) {
        LOG_ERROR("Available count for {} at station {} cannot be negative. Resetting to 0.", 
                      to_string(type), stationIndex);
        available_count_[type] = 0;
    }
}