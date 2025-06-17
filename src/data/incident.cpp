#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "data/incident.h"
#include "data/geometry.h"
#include "config/EnvLoader.h"
#include "utils/constants.h"

Incident::Incident(int id, double latitude, double longitude,
                   const std::string& type, IncidentLevel level,
                   time_t time)
    : incident_id(id), lat(latitude), lon(longitude),
      incident_type(type), incident_level(level), reportTime(time) {}

void Incident::printInfo() const {
    spdlog::debug("Incident ID: {}, Type: {}, Level: {}, Lat: {}, Lon: {}, Time: {}",
                incident_id, incident_type, to_string(incident_level), lat, lon, reportTime);
}

std::vector<Incident> loadIncidentsFromCSV(const std::string& filename) {
    EnvLoader env("../.env");
    std::string bounds_path = env.get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");
    spdlog::info("Loading incidents from CSV file: {}", filename);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);
    std::vector<Incident> incidents;
    std::ifstream file(filename);

    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filename);
        return incidents;
    }

    std::string line;
    std::getline(file, line);  // Skip header

    std::unordered_set<int> seenIDs; // To track unique incident IDs

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

        if (isPointInPolygon(polygon, Location(lon, lat))) {
            IncidentLevel ilevel = IncidentLevel::Invalid; // Default to Invalid
            if (level == constants::INCIDENT_LEVEL_LOW) {
                ilevel = IncidentLevel::Low;
            } else if (level == constants::INCIDENT_LEVEL_MODERATE) {
                ilevel = IncidentLevel::Moderate;
            } else if (level == constants::INCIDENT_LEVEL_HIGH) {
                ilevel = IncidentLevel::High;
            } else if (level == constants::INCIDENT_LEVEL_CRITICAL) {
                ilevel = IncidentLevel::Critical;
            } else {
                ilevel = IncidentLevel::Invalid; // Handle invalid levels
            }
            if (seenIDs.count(id)) {
                spdlog::warn("Incident ID {} is duplicated and will be ignored.", id);
                continue; // Skip if ID is already seen
            } else {
                seenIDs.insert(id);
                incidents.emplace_back(id, lat, lon, type, ilevel, unix_time);
            }
        } else {
            spdlog::warn("Incident {} is out of bounds and will be ignored.", id);
        }
        
    }

    return incidents;
}

Location Incident::getLocation() const {
    return Location(lat, lon);
}