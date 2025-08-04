#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "data/station.h"
#include "data/geometry.h"
#include "config/EnvLoader.h"
#include "utils/constants.h"
#include "utils/error.h"

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
        spdlog::warn("Attempted to set number of fire trucks to {} but max is {}", n, max_fire_trucks);
    }
}

void Station::setNumAmbulances(int n) {
    if (n <= max_ambulances) {
        num_ambulances = n;
    }
    else {
        spdlog::warn("Attempted to set number of ambulances to {} but max is {}", n, max_ambulances);
    }
}

void Station::printInfo() const {
    spdlog::debug("Station ID: {}, Lat: {}, Lon: {}, Fire Trucks: {}, Ambulances: {}",
                station_id, lat, lon, num_fire_trucks, num_ambulances);
}


void Station::initializeApparatusCount(std::vector<Apparatus>& all_apparatus) {
    for (auto& apparatus : all_apparatus) {
        if (apparatus.getStationIndex() == stationIndex) {
            total_count_[apparatus.getType()]++;
            if (apparatus.getStatus() == ApparatusStatus::Available) {
                available_count_[apparatus.getType()]++;
            }
        }
    }
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

// // Update counts when apparatus status changes
// void Station::updateApparatusStatus(ApparatusType type, ApparatusStatus new_status) {
//     if (old_status == ApparatusStatus::Available) {
//         available_count_[type]--;
//     }
//     if (new_status == ApparatusStatus::Available) {
//         available_count_[type]++;
//     }
// }

// // TODO: handle apparatus returning
// void Station::returnApparatus(ApparatusType type, int count) {
//     int total_count = total_count_[type];
//     int available_count = available_count_[type];
    
//     if (available_count + count <= total_count) {
//         available_count_[type] += count;
//         spdlog::debug("Returned {} {}(s) to station {}", count, type, station_id);
//     } else {
//         spdlog::error("Cannot return {} {}(s) to station {}: exceeds total count", count, type, station_id);
//         throw 
//     }

// }

// // TODO: handle sending apparatus
// void Station::dispatchApparatus(ApparatusType type, ApparatusStatus status) {
//     for (const auto& [type, count] : dispatchedCounts) {
//         total_count_[type] -= count;
//         available_count_[type] -= count;
//     }
// }