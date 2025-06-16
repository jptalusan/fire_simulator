#include "data/incident.h"
#include "data/geometry.h"
#include "config/EnvLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

Incident::Incident(int id, double latitude, double longitude,
                   const std::string& type, const std::string& level,
                   time_t time)
    : incident_id(id), lat(latitude), lon(longitude),
      incident_type(type), incident_level(level), unix_time(time) {}

void Incident::printInfo() const {
    std::cout << "Incident ID: " << incident_id
              << ", Type: " << incident_type
              << ", Level: " << incident_level
              << ", Lat: " << lat << ", Lon: " << lon
              << ", Time: " << unix_time << "\n";
}

std::vector<Incident> loadIncidentsFromCSV(const std::string& filename) {
    EnvLoader env("../.env");
    std::string bounds_path = env.get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");
    std::vector<Point> polygon = loadPolygonFromGeoJSON(bounds_path);
    std::vector<Incident> incidents;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << '\n';
        return incidents;
    }

    std::string line;
    std::getline(file, line);  // Skip header

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        int id;
        double lat, lon;
        std::string type, level, datetime_str;

        std::getline(ss, token, ',');
        id = std::stoi(token);

        std::getline(ss, token, ',');
        lat = std::stod(token);

        std::getline(ss, token, ',');
        lon = std::stod(token);

        std::getline(ss, type, ',');
        std::getline(ss, level, ',');
        std::getline(ss, datetime_str);

        // Parse datetime string to Unix time
        std::tm tm = {};
        std::istringstream datetime_ss(datetime_str);
        datetime_ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (datetime_ss.fail()) {
            std::cerr << "Failed to parse datetime: " << datetime_str << '\n';
            continue;
        }
        time_t unix_time = std::mktime(&tm);

        if (isPointInPolygon(polygon, Point(lon, lat))) {
            incidents.emplace_back(id, lat, lon, type, level, unix_time);
        } else {
            std::cout << "Incident " << id << " is out of bounds and will be ignored." << std::endl;
        }
        
    }

    return incidents;
}

Location Incident::getLocation() const {
    return Location(lat, lon);
}