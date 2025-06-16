#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include "data/location.h"

class Station {
public:
    // TODO: Will result in garbage values. Add a default constructor
    Station() = default; // Default constructor
    Station(int station_id,
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
};

std::vector<Station> loadStationsFromCSV(const std::string& filename);

#endif // STATION_H
