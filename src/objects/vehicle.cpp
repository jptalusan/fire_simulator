#include "objects/vehicle.h"
#include "objects/location.h"
#include <iostream>
#include <iomanip>

// Implementation is mostly in the header (inline constructors)
// This file just ensures Location is fully defined for member operations

// Print vehicle details including all properties
void Vehicle::printDetails() const {
    std::cout << "=== Vehicle Details ===" << std::endl;
    std::cout << "Vehicle ID: " << vehicleId << std::endl;
    std::cout << "Station ID: " << stationId << std::endl;
    std::cout << "Station Index: " << stationIndex << std::endl;
    
    std::cout << "Location: (" << std::fixed << std::setprecision(6) 
              << currentLocation.lat << ", " << currentLocation.lon << ")" << std::endl;
    
    std::cout << "Apparatus Type: " << to_string(apparatusType) << std::endl;
    std::cout << "Apparatus Status: " << to_string(apparatusStatus) << std::endl;
    
    if (timeToIncident >= 0) {
        std::cout << "Time to Incident: " << std::fixed << std::setprecision(2) 
                  << timeToIncident << " minutes" << std::endl;
    } else {
        std::cout << "Time to Incident: Not calculated" << std::endl;
    }
    
    if (timeToReturn >= 0) {
        std::cout << "Time to Return: " << std::fixed << std::setprecision(2) 
                  << timeToReturn << " minutes" << std::endl;
    } else {
        std::cout << "Time to Return: Not calculated" << std::endl;
    }
    
    std::cout << "========================\n" << std::endl;
}