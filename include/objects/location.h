#ifndef LOCATION_H
#define LOCATION_H

#include <sstream>
#include <iomanip>
#include <unordered_map>
#include "enums.h"
#include <vector>
#include "objects/vehicle.h" // need full Vehicle type for vectors/members
#include "objects/common.h"

inline std::string locationToString(const Location& location) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << location.lon << "," << location.lat; // lon,lat
    return oss.str();
}

class FireStation {
    public:
        std::string stationId;
        int stationIndex;
        Location location;

        FireStation(): stationId(""), stationIndex(-1), location() {}
        FireStation(const std::string& stationId, int stationIndex, const Location& location) :
            stationId(stationId), stationIndex(stationIndex), location(location) {}
        
        // Add an apparatus (vehicle) to the appropriate map
        void addApparatusToMap(ApparatusType type, const std::vector<Vehicle>& vehicles);
        // Return up to `count` available apparatus of the given type
        std::vector<int> getAvailableApparatus(ApparatusType type) const;
        
        Location getLocation() const noexcept;

        // Print station details including name, location, and vehicle counts by type
        void printStationDetails() const;

        int getAvailableCount(ApparatusType type) const;
        void updateAvailableCount(ApparatusType type, int count);
    
        const std::string& getStationId() const noexcept { return stationId; }
        int getStationIndex() const noexcept { return stationIndex; }
        void updateApparatusCounts();
        std::vector<int> getAllVehicles() const;
    private:
        std::unordered_map<ApparatusType, std::vector<int>> availableFireApparatusMap;
        std::unordered_map<ApparatusType, std::vector<int>> availableEMSApparatusMap;

        // Maps for fast lookups
        std::unordered_map<ApparatusType, int> available_count_;
};

#endif // LOCATION_H