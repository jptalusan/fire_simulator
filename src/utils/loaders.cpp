#include <string>
#include <unordered_set>
#include <vector>
#include <spdlog/spdlog.h>
#include "utils/loaders.h"
#include "config/EnvLoader.h"
#include "data/geometry.h"
#include "utils/error.h"
#include "utils/constants.h"
#include "data/apparatus.h"

int parseIntToken(const std::string& token, int defaultValue = 0) {
    spdlog::debug("Parsing token: '{}' (length: {})", token, token.length());
    
    if (token.empty()) {
        spdlog::debug("Token is empty, returning default: {}", defaultValue);
        return defaultValue;
    }
    
    try {
        int result = std::stoi(token);
        spdlog::debug("Successfully parsed: {}", result);
        return result;
    } catch (const std::exception& e) {
        // spdlog::error("Failed to parse token '{}': {}, using default {}", token, e.what(), defaultValue);
        return defaultValue;
    }
}

std::vector<Station> loadStationsFromCSV(const EnvLoader& env) {
    std::string filename = env.get("STATIONS_CSV_PATH", "");
    // TODO: Add error checking
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

        // Skip Facility Name
        std::getline(ss, token, ',');

        // Skip Address
        std::getline(ss, token, ',');

        // Skip City
        std::getline(ss, token, ',');

        // Skip State
        std::getline(ss, token, ',');

        // Skip Zip Code
        std::getline(ss, token, ',');

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
                            num_fire_trucks,
                            num_ambulances,
                            lon,
                            lat);
            stations.emplace_back(station);
            spdlog::debug("Loaded station: {}", station_id);
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

std::vector<Incident> loadIncidentsFromCSV(const EnvLoader& env) {
    std::string filename = env.get("INCIDENTS_CSV_PATH", "");
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

/*
Idea is to just load the apparatuses from a separate apparatus.csv.
Read this first, then assign to each station after the fact.
Throw an error if there are stations mismatch between stations and apparatus.
(all apparatus stations should be in the stations list)
The goal is to only give the stations a dictionary of apparatus types and counts, while apparatuses is its own list.
This would allow us to loop through the independently from the stations (if needed).
*/
std::vector<Apparatus> loadApparatusFromCSV(const EnvLoader& env) {
    std::string filename = env.get("APPARATUS_CSV_PATH", "");
    std::string bounds_path = env.get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");

    spdlog::info("Loading apparatuses from CSV file: {}", filename);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);

    std::vector<Apparatus> apparatuses;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filename);
        return apparatuses;
    }

    // Skip header line
    std::getline(file, line);

    int ignoredCount = 0; // Count of ignored apparatuses
    int index = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // StationID
        // How to ensure that these are string that can be converted to int?
        std::getline(ss, token, ',');
        int station_id = -1;
        try {
            station_id = std::stoi(token);
        } catch (...) {
            spdlog::error("Invalid station ID: {}", token);
            throw InvalidStationError("Invalid station ID in CSV file: " + token);
        }
    
        // Skip Facility Name/Stations
        std::getline(ss, token, ',');
    
        // // Skip Station Name
        std::getline(ss, token, ',');

        // // Engine_ID (its jus count)
        std::getline(ss, token, ',');
        int engine_count = parseIntToken(token);

        // Truck
        std::getline(ss, token, ',');
        int truck_count = parseIntToken(token);

        // Rescue
        std::getline(ss, token, ',');
        int rescue_count = parseIntToken(token);

        // Hazard
        std::getline(ss, token, ',');
        int hazard_count = parseIntToken(token);

        // Squad
        std::getline(ss, token, ',');
        int squad_count = parseIntToken(token);

        // Fast
        std::getline(ss, token, ',');
        int fast_count = parseIntToken(token);

        // Medic
        std::getline(ss, token, ',');
        int medic_count = parseIntToken(token);

        // Brush
        std::getline(ss, token, ',');
        int brush_count = parseIntToken(token);

        // Boat
        std::getline(ss, token, ',');
        int boat_count = parseIntToken(token);

        // UTV
        std::getline(ss, token, ',');
        int utv_count = parseIntToken(token);

        // REACH
        std::getline(ss, token, ',');
        int reach_count = parseIntToken(token);

        // Chief
        std::getline(ss, token, ',');
        int chief_count = parseIntToken(token);

        spdlog::debug("Station Index: {}, Chief Count: {}", station_id, chief_count);

        // Loop through each apparatus type and create instances
        if (engine_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Engine);
            apparatuses.emplace_back(a);
        }

        if (truck_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Truck);
            apparatuses.emplace_back(a);
        }

        if (rescue_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Rescue);
            apparatuses.emplace_back(a);
        }

        if (hazard_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Hazard);
            apparatuses.emplace_back(a);
        }

        if (squad_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Squad);
            apparatuses.emplace_back(a);
        }

        if (fast_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Fast);
            apparatuses.emplace_back(a);
        }

        if (medic_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Medic);
            apparatuses.emplace_back(a);
        }

        if (brush_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Brush);
            apparatuses.emplace_back(a);
        }

        if (boat_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Boat);
            apparatuses.emplace_back(a);
        }

        if (utv_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::UTV);
            apparatuses.emplace_back(a);
        }

        if (reach_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Reach);
            apparatuses.emplace_back(a);
        }

        if (chief_count > 0) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Chief);
            apparatuses.emplace_back(a);
        }
    }

    file.close();
    // spdlog::info("Loaded {} stations from CSV file.", stations.size());
    // spdlog::warn("Ignored {} stations that are out of bounds.", ignoredCount);
    return apparatuses;
}
