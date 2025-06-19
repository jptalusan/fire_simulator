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
          facility_name(""),
          address(""),
          city(""),
          state(""),
          zip_code(""),
          lon(0.0),
          lat(0.0),
          num_fire_trucks(0),
          num_ambulances(0),
          max_ambulances(0),
          max_fire_trucks(0)
    {}
    Station(int stationIndex,
            int station_id,
            const std::string& facility_name,
            const std::string& address,
            const std::string& city,
            const std::string& state,
            const std::string& zip_code,
            double lon,
            double lat,
            int num_fire_trucks,
            int num_ambulances);

    // Getters
    int getStationIndex() const;
    int getStationId() const;
    std::string getFacilityName() const;
    std::string getAddress() const;
    std::string getCity() const;
    std::string getState() const;
    std::string getZipCode() const;
    double getLon() const;
    double getLat() const;
    Location getLocation() const;
    int getNumFireTrucks() const;
    int getNumAmbulances() const;

    // Setters
    void setNumFireTrucks(int n);
    void setNumAmbulances(int n);

    // Helper function example
    void printInfo() const;

private:
    int stationIndex;
    int station_id;
    std::string facility_name;
    std::string address;
    std::string city;
    std::string state;
    std::string zip_code;
    double lon;
    double lat;
    int num_fire_trucks;
    int num_ambulances;
    int max_ambulances;
    int max_fire_trucks;
};

std::vector<Station> loadStationsFromCSV(const std::string& filename);

#endif // STATION_H
