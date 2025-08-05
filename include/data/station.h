#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include "data/location.h"
#include "data/apparatus.h" // Assuming Apparatus is defined in this header
#include "enums.h"

class Station {
public:
    Station()
        : stationIndex(-1),
          station_id(-1),
          num_fire_trucks(0),
          num_ambulances(0),
          max_ambulances(0),
          max_fire_trucks(0),
          lon(0.0),
          lat(0.0)
    {}
    Station(int stationIndex,
            int station_id,
            int num_fire_trucks,
            int num_ambulances,
            double lon,
            double lat);

    // Getters
    int getStationIndex() const;
    int getStationId() const noexcept;
    int getNumFireTrucks() const noexcept;
    int getNumAmbulances() const noexcept;
    double getLon() const noexcept;
    double getLat() const noexcept;

    Location getLocation() const noexcept;

    // Setters
    void setNumFireTrucks(int n);
    void setNumAmbulances(int n);

    // Helper function example
    void printInfo() const;

    void initializeApparatusCount(std::vector<Apparatus>& all_apparatus);
    int getAvailableCount(ApparatusType type) const;
    int getTotalCount(ApparatusType type) const;
    void updateTotalCount(ApparatusType type, int count);
    void updateAvailableCount(ApparatusType type, int count);
    
    bool dispatchApparatus(ApparatusType type, int count = 1);
    void returnApparatus(ApparatusType type, int count = 1);
    bool canDispatch(ApparatusType type, int count = 1) const;
    

private:
    // Integers first (4 bytes each)
    int stationIndex;
    int station_id;
    int num_fire_trucks;
    int num_ambulances;
    int max_ambulances;
    int max_fire_trucks;

    // Doubles next (8 bytes each)
    double lon;
    double lat;

    // Maps for fast lookups
    std::unordered_map<ApparatusType, int> available_count_; // Assuming ApparatusType is defined elsewhere
    std::unordered_map<ApparatusType, int> total_count_;

    void updateApparatusStatus(ApparatusType type, ApparatusStatus old_status, ApparatusStatus new_status);
};

#endif // STATION_H
