#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include "data/location.h"

class Station {
public:
    Station()
        : stationIndex(-1),
          station_id(-1),
          lon(0.0),
          lat(0.0),
          num_fire_trucks(0),
          num_ambulances(0),
          max_ambulances(0),
          max_fire_trucks(0)
    {}
    Station(int stationIndex,
            int station_id,
            double lon,
            double lat,
            int num_fire_trucks,
            int num_ambulances);

    // Getters
    int getStationIndex() const;
    int getStationId() const noexcept;
    double getLon() const noexcept;
    double getLat() const noexcept;
    Location getLocation() const noexcept;
    int getNumFireTrucks() const noexcept;
    int getNumAmbulances() const noexcept;

    // Setters
    void setNumFireTrucks(int n);
    void setNumAmbulances(int n);

    // Helper function example
    void printInfo() const;

private:
    int stationIndex;
    int station_id;
    double lon;
    double lat;
    int num_fire_trucks;
    int num_ambulances;
    int max_ambulances;
    int max_fire_trucks;
};

std::vector<Station> loadStationsFromCSV(const std::string& filename);

#endif // STATION_H
