#include "objects/location.h"
#include "objects/vehicle.h"
#include "utils/logger.h"
#include <iostream>
#include <iomanip>

// Add an apparatus (vehicle) to the appropriate map
void FireStation::addApparatusToMap(ApparatusType type, const std::vector<Vehicle>& vehicles) {
    std::vector<int> vehicleIds = {};
    for (const auto& v : vehicles) {
        vehicleIds.push_back(v.getVehicleId());
    }
    if (type == ApparatusType::Medic) {
        auto featureMapping = availableEMSApparatusMap.find(type);
        if (featureMapping != availableEMSApparatusMap.end()) {
            LOG_WARN("Current {} vector is not empty.", to_string(type));
        } else {
            availableEMSApparatusMap[type] = vehicleIds;
        }
    } else {
        auto featureMapping = availableFireApparatusMap.find(type);
        if (featureMapping != availableFireApparatusMap.end()) {
            LOG_WARN("Current {} vector is not empty.", to_string(type));
        } else {
            availableFireApparatusMap[type] = vehicleIds;
        }
    }
}

// Return up to `count` available apparatus of the given type
std::vector<int> FireStation::getAvailableApparatus(ApparatusType type) const {
    std::vector<int> availableVehicleIds = {};
    if (type == ApparatusType::Medic) {
        auto featureMapping = availableEMSApparatusMap.find(type);
        if (featureMapping != availableEMSApparatusMap.end()) {
            availableVehicleIds = featureMapping->second;
        }
    } else {
        auto featureMapping = availableFireApparatusMap.find(type);
        if (featureMapping != availableFireApparatusMap.end()) {
            availableVehicleIds = featureMapping->second;
        }
    }
    return availableVehicleIds;
}

std::vector<int> FireStation::getAllVehicles() const {
    std::vector<int> allVehicles = {};
    
    // Add all fire apparatus vehicles
    for (const auto& [type, vehicleVector] : availableFireApparatusMap) {
        for (const auto& vehicle : vehicleVector) {
            allVehicles.push_back(vehicle);
        }
    }
    
    // Add all EMS apparatus vehicles
    for (const auto& [type, vehicleVector] : availableEMSApparatusMap) {
        for (const auto& vehicle : vehicleVector) {
            allVehicles.push_back(vehicle);
        }
    }
    
    return allVehicles;
}

Location FireStation::getLocation() const noexcept {
    return location;
}

// Print station details including name, location, and vehicle counts by type
void FireStation::printStationDetails() const {
    std::cout << "=== Fire Station Details ===" << std::endl;
    std::cout << "Station ID: " << stationId << std::endl;
    std::cout << "Station Index: " << stationIndex << std::endl;
    std::cout << "Location: (" << std::fixed << std::setprecision(6) 
              << location.lat << ", " << location.lon << ")" << std::endl;
    
    std::cout << "\n--- Fire Apparatus ---" << std::endl;
    int totalFireVehicles = 0;
    for (const auto& pair : availableFireApparatusMap) {
        const std::vector<int>& vehicles = pair.second;
        totalFireVehicles += vehicles.size();
    }
    if (totalFireVehicles == 0) {
        std::cout << "  No fire apparatus available" << std::endl;
    }
    
    std::cout << "\n--- EMS Apparatus ---" << std::endl;
    int totalEMSVehicles = 0;
    for (const auto& pair : availableEMSApparatusMap) {
        const std::vector<int>& vehicles = pair.second;
        totalEMSVehicles += vehicles.size();
    }
    if (totalEMSVehicles == 0) {
        std::cout << "  No EMS apparatus available" << std::endl;
    }
    
    std::cout << "\nTotal Vehicles: " << (totalFireVehicles + totalEMSVehicles) << std::endl;
    std::cout << "==============================\n" << std::endl;
}


// Fast lookups - O(1)
int FireStation::getAvailableCount(ApparatusType type) const {
    auto it = available_count_.find(type);
    return it != available_count_.end() ? it->second : 0;
}

void FireStation::updateApparatusCounts() {
    // Iterate read-only (no copies)
    for (const auto& [type, vehicle_vector] : availableFireApparatusMap) {
        updateAvailableCount(type, vehicle_vector.size());
    }
    for (const auto& [type, vehicle_vector] : availableEMSApparatusMap) {
        updateAvailableCount(type, vehicle_vector.size());
    }

}

void FireStation::updateAvailableCount(ApparatusType type, int count) {
    available_count_[type] += count;
    if (available_count_[type] < 0) {
        LOG_ERROR("Available count for {} at station {} cannot be negative. Resetting to 0.", 
                      to_string(type), stationIndex);
        available_count_[type] = 0;
    }
}
