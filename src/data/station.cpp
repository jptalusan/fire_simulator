#include "data/station.h"
#include "data/geometry.h"
#include "config/EnvLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Station::Station(int station_id,
                 const std::string& facility_name,
                 const std::string& address,
                 const std::string& city,
                 const std::string& state,
                 const std::string& zip_code,
                 double lon,
                 double lat,
                 int num_fire_trucks,
                 int num_ambulances)
    : station_id(station_id),
      facility_name(facility_name),
      address(address),
      city(city),
      state(state),
      zip_code(zip_code),
      lon(lon),
      lat(lat),
      num_fire_trucks(num_fire_trucks),
      num_ambulances(num_ambulances) {}

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
void Station::setNumFireTrucks(int n) { num_fire_trucks = n; }
void Station::setNumAmbulances(int n) { num_ambulances = n; }

void Station::printInfo() const {
    std::cout << "Station ID: " << station_id
              << ", Name: " << facility_name
              << ", Address: " << address
              << ", City: " << city
              << ", State: " << state
              << ", Zip: " << zip_code
              << ", Lat: " << lat
              << ", Lon: " << lon
              << ", Fire Trucks: " << num_fire_trucks
              << ", Ambulances: " << num_ambulances
              << std::endl;
}

std::unordered_map<int, Station> loadStationsFromCSV(const std::string& filename) {
    EnvLoader env("../.env");
    std::string bounds_path = env.get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");

    std::vector<Point> polygon = loadPolygonFromGeoJSON(bounds_path);

    std::unordered_map<int, Station> stations;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return stations;
    }

    // Skip header line
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // Skip OBJECTID
        std::getline(ss, token, ',');

        // Facility Name
        std::getline(ss, token, ',');
        std::string facility_name = token;

        // Extract station_id from "Station 39"
        int station_id = -1;
        std::istringstream id_ss(token);
        std::string word;
        while (id_ss >> word) {
            try {
                station_id = std::stoi(word);
                break;
            } catch (...) {
                // Not a number
            }
        }

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

        int num_fire_trucks = 2; // Default value, can be updated later
        int num_ambulances = 0;  // Default value, can be updated later

        if (isPointInPolygon(polygon, Point(lon, lat))) {
            Station station(station_id,
                            facility_name,
                            address,
                            city,
                            state,
                            zip_code,
                            lon,
                            lat,
                            num_fire_trucks,
                            num_ambulances);
            stations.emplace(station_id, station);
        } else {
            std::cout << "Station " << station_id << " is out of bounds and will be ignored." << std::endl;
        }
    }

    file.close();
    return stations;
}
