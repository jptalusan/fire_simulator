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
                 const std::string& facility_name,
                 const std::string& address,
                 const std::string& city,
                 const std::string& state,
                 const std::string& zip_code,
                 double lon,
                 double lat,
                 int num_fire_trucks,
                 int num_ambulances)
    : stationIndex(stationIndex),
      station_id(station_id),
      facility_name(facility_name),
      address(address),
      city(city),
      state(state),
      zip_code(zip_code),
      lon(lon),
      lat(lat),
      num_fire_trucks(num_fire_trucks),
      num_ambulances(num_ambulances),
      max_ambulances(num_ambulances),
      max_fire_trucks(num_fire_trucks) {}

int Station::getStationIndex() const { return stationIndex; }
int Station::getStationId() const { return station_id; }
std::string Station::getFacilityName() const { return facility_name; }
std::string Station::getAddress() const { return address; }
std::string Station::getCity() const { return city; }
std::string Station::getState() const { return state; }
std::string Station::getZipCode() const { return zip_code; }
double Station::getLon() const { return lon; }
double Station::getLat() const { return lat; }
int Station::getNumFireTrucks() const { return num_fire_trucks; }
int Station::getNumAmbulances() const { return num_ambulances; }
Location Station::getLocation() const {
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
    spdlog::debug("Station ID: {}, Name: {}, Address: {}, City: {}, State: {}, Zip: {}, Lat: {}, Lon: {}, Fire Trucks: {}, Ambulances: {}",
                station_id, facility_name, address, city, state, zip_code, lat, lon, num_fire_trucks, num_ambulances);
}

std::vector<Station> loadStationsFromCSV(const std::string& filename) {
    EnvLoader env("../.env");
    std::string bounds_path = env.get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");

    spdlog::info("Loading stations from CSV file: {}", filename);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);

    std::vector<Station> stations;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filename);
        return stations;
    }

    // Skip header line
    std::getline(file, line);

    int ignoredCount = 0; // Count of ignored stations
    int index = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // Skip OBJECTID
        // How to ensure that these are string that can be converted to int?
        std::getline(ss, token, ',');
        int station_id = -1;
        try {
            station_id = std::stoi(token);
        } catch (...) {
            spdlog::error("Invalid station ID: {}", token);
            throw InvalidStationError("Invalid station ID in CSV file: " + token);
        }

        // Facility Name
        std::getline(ss, token, ',');
        std::string facility_name = token;

        // Address
        std::string address;
        std::getline(ss, address, ',');

        // City
        std::string city;
        std::getline(ss, city, ',');

        // State
        std::string state;
        std::getline(ss, state, ',');

        // Zip Code
        std::string zip_code;
        std::getline(ss, zip_code, ',');

        // Skip GLOBALID
        std::getline(ss, token, ',');

        // x
        std::getline(ss, token, ',');
        double lat = std::stod(token);

        // y
        std::getline(ss, token, ',');
        double lon = std::stod(token);

        int num_fire_trucks = constants::DEFAULT_NUM_FIRE_TRUCKS; // Default value, can be updated later
        int num_ambulances = constants::DEFAULT_NUM_AMBULANCES;  // Default value, can be updated later

        if (isPointInPolygon(polygon, Location(lon, lat))) {
            Station station(index,
                            station_id,
                            facility_name,
                            address,
                            city,
                            state,
                            zip_code,
                            lon,
                            lat,
                            num_fire_trucks,
                            num_ambulances);
            stations.emplace_back(station);
            spdlog::debug("Loaded station: {} - {} {}", station_id, facility_name, address);
            index++;
        } else {
            spdlog::debug("Station {} is out of bounds and will be ignored.", station_id);
            ignoredCount++;
        }
    }

    file.close();
    spdlog::info("Loaded {} stations from CSV file.", stations.size());
    spdlog::warn("Ignored {} stations that are out of bounds.", ignoredCount);
    return stations;
}
