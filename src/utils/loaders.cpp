#include <string>
#include <unordered_set>
#include <vector>
#include "utils/loaders.h"
#include "config/EnvLoader.h"
#include "data/geometry.h"
#include "utils/error.h"
#include "utils/logger.h"
#include "utils/constants.h"
#include "data/apparatus.h"
#include "services/queries.h"
#include "services/chunks.h"

namespace loader {
EventQueue generateEvents(const std::vector<Incident>& incidents) {
    std::vector<Event> container;
    container.reserve(incidents.size());  // Preallocate memory for efficiency
    EventQueue events(std::greater<Event>(), std::move(container));

    for (size_t i = 0; i < incidents.size(); ++i) {
        int incidentIndex = incidents[i].incidentIndex;
        time_t reportTime = incidents[i].reportTime;
        Event incidentEvent = Event::createIncidentEvent(reportTime, incidentIndex);
        events.push(incidentEvent);
    }

    return events;
}

int parseIntToken(const std::string& token, int defaultValue = 0) {
    LOG_DEBUG("Parsing token: '{}' (length: {})", token, token.length());
    
    if (token.empty()) {
        LOG_DEBUG("Token is empty, returning default: {}", defaultValue);
        return defaultValue;
    }
    
    try {
        int result = std::stoi(token);
        LOG_DEBUG("Successfully parsed: {}", result);
        return result;
    } catch (const std::exception& e) {
        // LOG_ERROR("Failed to parse token '{}': {}, using default {}", token, e.what(), defaultValue);
        return defaultValue;
    }
}

std::vector<Station> loadStationsFromCSV() {
    std::string filename = EnvLoader::getInstance()->get("STATIONS_CSV_PATH", "");
    // TODO: Add error checking
    std::string bounds_path = EnvLoader::getInstance()->get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");

    LOG_INFO("Loading stations from CSV file: {}", filename);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);

    std::vector<Station> stations;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", filename);
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
            LOG_ERROR("Invalid station ID: {}", token);
            throw InvalidStationError("Invalid station ID in CSV file: " + token);
        }

        // Skip Facility Name
        std::getline(ss, token, ',');
        std::string name = token;
        std::cout << name << std::endl;

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
            station.setFacilityName(name);
            stations.emplace_back(station);
            LOG_DEBUG("Loaded station: {}", station_id);
            index++;
        } else {
            LOG_DEBUG("Station {} is out of bounds and will be ignored.", station_id);
            ignoredCount++;
        }
    }

    file.close();
    LOG_INFO("Loaded {} stations from CSV file.", stations.size());
    LOG_WARN("Ignored {} stations that are out of bounds.", ignoredCount);
    return stations;
}

IncidentCategory stringToIncidentCategory(const std::string& str) {
    if (str == "One") return IncidentCategory::One;
    if (str == "OneB") return IncidentCategory::OneB;
    if (str == "OneBM") return IncidentCategory::OneBM;
    if (str == "OneC") return IncidentCategory::OneC;
    if (str == "OneD") return IncidentCategory::OneD;
    if (str == "OneE") return IncidentCategory::OneE;
    if (str == "OneEM") return IncidentCategory::OneEM;
    if (str == "OneF") return IncidentCategory::OneF;
    if (str == "OneG") return IncidentCategory::OneG;
    if (str == "OneH") return IncidentCategory::OneH;
    if (str == "OneJ") return IncidentCategory::OneJ;
    if (str == "Two") return IncidentCategory::Two;
    if (str == "TwoM") return IncidentCategory::TwoM;
    if (str == "TwoMF") return IncidentCategory::TwoMF;
    if (str == "TwoA") return IncidentCategory::TwoA;
    if (str == "TwoB") return IncidentCategory::TwoB;
    if (str == "TwoC") return IncidentCategory::TwoC;
    if (str == "Three") return IncidentCategory::Three;
    if (str == "ThreeF") return IncidentCategory::ThreeF;
    if (str == "ThreeM") return IncidentCategory::ThreeM;
    if (str == "ThreeA") return IncidentCategory::ThreeA;
    if (str == "ThreeB") return IncidentCategory::ThreeB;
    if (str == "ThreeC") return IncidentCategory::ThreeC;
    if (str == "ThreeCM") return IncidentCategory::ThreeCM;
    if (str == "ThreeD") return IncidentCategory::ThreeD;
    if (str == "Four") return IncidentCategory::Four;
    if (str == "FourM") return IncidentCategory::FourM;
    if (str == "FourA") return IncidentCategory::FourA;
    if (str == "FourB") return IncidentCategory::FourB;
    if (str == "FourC") return IncidentCategory::FourC;
    if (str == "Five") return IncidentCategory::Five;
    if (str == "FiveA") return IncidentCategory::FiveA;
    if (str == "Six") return IncidentCategory::Six;
    if (str == "Seven") return IncidentCategory::Seven;
    if (str == "SevenB") return IncidentCategory::SevenB;
    if (str == "SevenBM") return IncidentCategory::SevenBM;
    if (str == "Eight") return IncidentCategory::Eight;
    if (str == "EightA") return IncidentCategory::EightA;
    if (str == "EightB") return IncidentCategory::EightB;
    if (str == "EightC") return IncidentCategory::EightC;
    if (str == "EightD") return IncidentCategory::EightD;
    if (str == "EightE") return IncidentCategory::EightE;
    if (str == "EightF") return IncidentCategory::EightF;
    if (str == "EightG") return IncidentCategory::EightG;
    if (str == "Nine") return IncidentCategory::Nine;
    if (str == "Ten") return IncidentCategory::Ten;
    if (str == "Eleven") return IncidentCategory::Eleven;
    if (str == "ElevenA") return IncidentCategory::ElevenA;
    if (str == "ElevenB") return IncidentCategory::ElevenB;
    if (str == "Thirteen") return IncidentCategory::Thirteen;
    if (str == "Fourteen") return IncidentCategory::Fourteen;
    if (str == "Fifteen") return IncidentCategory::Fifteen;
    if (str == "Sixteen") return IncidentCategory::Sixteen;
    if (str == "Eighteen") return IncidentCategory::Eighteen;
    return IncidentCategory::Invalid;
}

std::vector<Incident> loadIncidentsFromCSV() {
    std::string filename = EnvLoader::getInstance()->get("INCIDENTS_CSV_PATH", "");
    std::string bounds_path = EnvLoader::getInstance()->get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");
    LOG_INFO("Loading incidents from CSV file: {}", filename);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);
    std::vector<Incident> incidents;
    std::ifstream file(filename);

    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", filename);
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
        std::string type, level, datetime_str, category;

        std::getline(ss, token, ',');
        try {
            id = std::stoi(token);
        } catch (...) {
            LOG_ERROR("Invalid incident ID: {}", token);
            throw InvalidIncidentError("Invalid incident ID in CSV file: " + token);
        }

        std::getline(ss, token, ',');
        lat = std::stod(token);

        std::getline(ss, token, ',');
        lon = std::stod(token);

        std::getline(ss, type, ',');
        std::getline(ss, level, ',');
        std::getline(ss, datetime_str, ',');
        std::getline(ss, category, ',' ); 


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
            IncidentCategory icategory = stringToIncidentCategory(category);

            if (seenIDs.count(id)) {
                // LOG_WARN("Incident ID {} is duplicated and will be ignored.", id);
                ignoredCount++;
                continue; // Skip if ID is already seen
            } else {
                seenIDs.insert(id);
                incidents.emplace_back(index, id, lat, lon, itype, ilevel, unix_time, icategory);
                index++;
            }
        } else {
            // LOG_DEBUG("Incident {} is out of bounds and will be ignored.", id);
            ignoredCount++;
        }
        
    }
    LOG_INFO("Total incidents: {}", incidents.size() + ignoredCount);
    LOG_INFO("Loaded {} incidents from CSV file.", incidents.size());
    LOG_WARN("Ignored {} incidents that are out of bounds.", ignoredCount);
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
std::vector<Apparatus> loadApparatusFromCSV() {
    std::string filename = EnvLoader::getInstance()->get("APPARATUS_CSV_PATH", "");
    std::string bounds_path = EnvLoader::getInstance()->get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");

    LOG_INFO("Loading apparatuses from CSV file: {}", filename);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);

    std::vector<Apparatus> apparatuses;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", filename);
        return apparatuses;
    }

    // Skip header line
    std::getline(file, line);

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
            LOG_ERROR("Invalid station ID: {}", token);
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

        LOG_DEBUG("Station Index: {}, Chief Count: {}", station_id, chief_count);

        // Loop through each apparatus type and create instances
        for (int i = 0; i < engine_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Engine);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < truck_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Truck);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < rescue_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Rescue);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < hazard_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Hazard);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < squad_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Squad);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < fast_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Fast);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < medic_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Medic);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < brush_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Brush);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < boat_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Boat);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < utv_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::UTV);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < reach_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Reach);
            apparatuses.emplace_back(a);
        }

        for (int i = 0; i < chief_count; i++) {
            Apparatus a = Apparatus(index++, station_id, ApparatusType::Chief);
            apparatuses.emplace_back(a);
        }
    }

    file.close();
    // LOG_INFO("Loaded {} stations from CSV file.", stations.size());
    // LOG_WARN("Ignored {} stations that are out of bounds.", ignoredCount);
    return apparatuses;
}


// TODO: Add checking if the binary files already exist, if so, load them instead of generating them again.
void preComputingMatrices(std::vector<Station>& stations, 
                          std::vector<Incident>& incidents,
                          std::vector<Apparatus>& apparatuses,
                          size_t chunk_size) {
    LOG_INFO("Starting Precomputation...");
    //spdlog::stopwatch sw;
    // Additional logic can be added here
    std::shared_ptr<EnvLoader> env = EnvLoader::getInstance();
    std::string stations_path = env->get("STATIONS_CSV_PATH", "../data/stations.csv");
    std::string incidents_path = env->get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string matrix_csv_path = env->get("MATRIX_CSV_PATH", "../logs/matrix.csv");
    std::string distance_matrix_path = env->get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin");
    std::string duration_matrix_path = env->get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin");
    std::string beats_shapefile_path = env->get("BEATS_SHAPEFILE_PATH", "../data/beats_shpfile.geojson");
    std::string osrmUrl_ = env->get("BASE_OSRM_URL", "http://router.project-osrm.org");

    if (checkOSRM(osrmUrl_)) {
        LOG_INFO("OSRM server is reachable and working correctly.");
    } else {
        LOG_ERROR("OSRM server is not reachable.");
        throw OSRMError();
    }

    stations = loadStationsFromCSV();
    incidents = loadIncidentsFromCSV();
    apparatuses = loadApparatusFromCSV();

    // START Adding zones per incident (maybe costly?)
    std::vector<std::pair<int, Polygon>> polygonWithZoneID = loadServiceZonesFromGeojson(beats_shapefile_path);
    std::vector<Polygon> polygons;
    polygons.reserve(polygonWithZoneID.size());
    for (const auto& pair : polygonWithZoneID) {
        polygons.emplace_back(pair.second);
    }
    std::vector<Point> points;
    points.reserve(incidents.size());  // Preallocate memory for efficiency
    for (auto& incident : incidents) {
        points.emplace_back(Point(incident.lon, incident.lat));
    }
    auto results = getPointToPolygonIndices(points, polygons);
    int notThere = 0;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i]) {
            int zoneIndex = polygonWithZoneID.at(*results[i]).first;
            incidents.at(i).zoneIndex = zoneIndex;
        } else {    
            notThere++;
        }
    }
    LOG_ERROR("There are {} incidents that are not in any service zone.", notThere);
    LOG_ERROR("There are {} incidents in service zones.", results.size() - notThere);
    // END Adding zones per incident (maybe costly?)

    std::vector<Location> sources;
    sources.reserve(stations.size());  // Preallocate memory for efficiency
    for (const auto& station : stations) {
        sources.emplace_back(station.getLocation());
    }
    std::vector<Location> destinations;
    destinations.reserve(incidents.size());  // Preallocate memory for efficiency
    for (const auto& incident : incidents) {
        destinations.emplace_back(incident.getLocation());
    }

    auto result = generate_osrm_table_chunks(sources, destinations, chunk_size);
    const std::vector<std::vector<double>>& full_distance_matrix = result.first;
    const std::vector<std::vector<double>>& full_duration_matrix = result.second;

    std::cout << full_duration_matrix.size() << " sources, " 
              << full_duration_matrix[0].size() << " destinations.\n";
    // print_matrix(full_duration_matrix, 5, 5);
    // For readability
    write_matrix_to_csv(full_duration_matrix, matrix_csv_path, 2, false);
    save_matrix_binary(full_duration_matrix, duration_matrix_path);
    save_matrix_binary(full_distance_matrix, distance_matrix_path);
   // LOG_INFO("Preprocessing completed successfully in {:.3} s.", sw);
}

} // namespace loader

