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
#include "utils/error.h"

Incident::Incident(int index, int id, double latitude, double longitude,
                   IncidentType type, IncidentLevel level,
                   time_t time)
    : incidentIndex(index), incident_id(id), lat(latitude), lon(longitude),
      incident_type(type), incident_level(level), reportTime(time) {
        timeRespondedTo = std::time(nullptr);
        resolvedTime = std::time(nullptr);
        currentApparatusCount = 0;
        totalApparatusRequired = 0; // This can be set later based on the
        status = IncidentStatus::hasBeenReported; // Initially, the incident is not resolved
        zoneIndex = -1;
      }

void Incident::printInfo() const {
    spdlog::error("Incident Index: {}, ID: {}, Type: {}, Level: {}, Lat: {}, Lon: {}, Time: {}",
                incidentIndex, incident_id, to_string(incident_type), to_string(incident_level), lat, lon, reportTime);
}

Location Incident::getLocation() const {
    return Location(lat, lon);
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

    int ignoredCount = 0; // Count of ignored incidents
    int index = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        ss.precision(6); // Set precision for floating-point numbers
        std::string token;

        int id;
        double lat, lon;
        std::string type, level, datetime_str;

        std::getline(ss, token, ',');
        try {
            id = std::stoi(token);
        } catch (...) {
            spdlog::error("Invalid incident ID: {}", token);
            throw InvalidIncidentError("Invalid incident ID in CSV file: " + token);
        }

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
            throw std::runtime_error("Invalid datetime format: " + datetime_str);
            // continue;
        }
        tm.tm_isdst = -1;  // Let mktime() determine DST
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

            IncidentType itype = IncidentType::Invalid; // Default to Invalid
            if (type == "Fire") {
                itype = IncidentType::Fire;
            } else if (type == "Medical") {
                itype = IncidentType::Medical;
            } else {
                itype = IncidentType::Invalid; // Handle invalid types
            }

            if (seenIDs.count(id)) {
                // spdlog::warn("Incident ID {} is duplicated and will be ignored.", id);
                ignoredCount++;
                continue; // Skip if ID is already seen
            } else {
                seenIDs.insert(id);
                incidents.emplace_back(index, id, lat, lon, itype, ilevel, unix_time);
                index++;
            }
        } else {
            // spdlog::debug("Incident {} is out of bounds and will be ignored.", id);
            ignoredCount++;
        }
        
    }
    spdlog::info("Total incidents: {}", incidents.size() + ignoredCount);
    spdlog::info("Loaded {} incidents from CSV file.", incidents.size());
    spdlog::warn("Ignored {} incidents that are out of bounds.", ignoredCount);
    return incidents;
}
