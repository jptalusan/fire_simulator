#include <string>
#include <unordered_set>
#include <vector>
#include "io/loaders.h"
#include "config/EnvLoader.h"
#include "objects/geometry.h"
#include "utils/error.h"
#include "utils/logger.h"
#include "utils/constants.h"
#include "objects/vehicle.h"
#include "services/queries.h"
#include "services/chunks.h"
#include "objects/firestation.h"

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

std::pair<std::vector<FireStation>, std::vector<Vehicle>> loadStationsFromCSV() {
    std::string filename = EnvLoader::getInstance()->get("APPARATUS_CSV_PATH", "");
    // TODO: Add error checking
    std::string bounds_path = EnvLoader::getInstance()->get("BOUNDS_GEOJSON_PATH", "../data/bounds.geojson");

    std::cout<<bounds_path<<";"<<filename<<std::endl;
    LOG_INFO("Loading stations from CSV file: {}", filename);
    LOG_INFO("Loading polygon from geojson file: {}", bounds_path);
    std::vector<Location> polygon = loadPolygonFromGeoJSON(bounds_path);
    
    std::vector<FireStation> stations;
    std::vector<Vehicle> vehicles;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", filename);
        return std::make_pair(stations, vehicles);
    }

    // Skip header line
    std::getline(file, line);

    int ignoredCount = 0; // Count of ignored stations
    int vehicleIndex = 0;
    int _stationIndex = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // How to ensure that these are string that can be converted to int?
        // Station Index
        std::getline(ss, token, ',');
        int stationIndex = -1;
        try {
            stationIndex = std::stoi(token);
            if (stationIndex != _stationIndex) {
                throw InvalidStationError("Invalid station ID in CSV file: " + token);
            }
        } catch (...) {
            LOG_ERROR("Invalid station ID: {}", token);
            throw InvalidStationError("Invalid station ID in CSV file: " + token);
        }

        // Stations ID
        std::getline(ss, token, ',');
        std::string stationId = token;

        // // Skip Address
        // std::getline(ss, token, ',');

        // // Skip City
        // std::getline(ss, token, ',');

        // // Skip State
        // std::getline(ss, token, ',');

        // // Skip Zip Code
        // std::getline(ss, token, ',');

        // // Skip GLOBALID
        // std::getline(ss, token, ',');

        // x
        std::getline(ss, token, ',');
        double lat = std::stod(token);

        // y
        std::getline(ss, token, ',');
        double lon = std::stod(token);
        Location location;
        location.lat = lat;
        location.lon = lon;

        if (!isPointInPolygon(polygon, location)) {
            LOG_DEBUG("Station {} is out of bounds and will be ignored.", stationId);
            ignoredCount++;
            continue;
        } else {
            LOG_DEBUG("Station {} is inside the polygon bounds.", stationId);
        }

        // Skip the address
        std::getline(ss, token, ',');

        // Engine_ID (its just count)
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

        std::vector<Vehicle> fireEngines = {};
        std::vector<Vehicle> trucks = {};
        std::vector<Vehicle> rescues = {};
        std::vector<Vehicle> hazards = {};
        std::vector<Vehicle> squads = {};
        std::vector<Vehicle> fasts = {};
        std::vector<Vehicle> brushes = {};
        std::vector<Vehicle> boats = {};
        std::vector<Vehicle> utvs = {};
        std::vector<Vehicle> reaches = {};
        std::vector<Vehicle> chiefs = {};

        std::vector<Vehicle> medics = {};

        // EMS are special case
        for (int i = 0; i < medic_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                  vehicleIndex++, location,
                                ApparatusType::Medic, 
                                  ApparatusStatus::Available);
            medics.emplace_back(a);
            vehicles.emplace_back(a);
        }

        // Loop through each apparatus type and create instances
        for (int i = 0; i < engine_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Engine, 
                                ApparatusStatus::Available);
            fireEngines.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < truck_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Truck, 
                                ApparatusStatus::Available);
            trucks.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < rescue_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Rescue, 
                                ApparatusStatus::Available);
            rescues.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < hazard_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Hazard, 
                                ApparatusStatus::Available);
            hazards.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < squad_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Squad, ApparatusStatus::Available);
            squads.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < fast_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Fast, 
                                ApparatusStatus::Available);
            fasts.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < brush_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Brush, 
                                ApparatusStatus::Available);
            brushes.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < boat_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Boat, 
                                ApparatusStatus::Available);
            boats.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < utv_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::UTV, 
                                ApparatusStatus::Available);
            utvs.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < reach_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Reach, 
                                ApparatusStatus::Available);
            reaches.push_back(a);
            vehicles.emplace_back(a);
        }

        for (int i = 0; i < chief_count; i++) {
            Vehicle a = Vehicle(stationIndex, stationId,
                                vehicleIndex++, location,
                                ApparatusType::Chief, 
                                ApparatusStatus::Available);
            chiefs.push_back(a);
            vehicles.emplace_back(a);
        }

        FireStation fireStation(stationId,
                        stationIndex,
                        location);
        // station.setFacilityName(name);
        fireStation.addApparatusToMap(ApparatusType::Engine, fireEngines);
        fireStation.addApparatusToMap(ApparatusType::Truck, trucks);
        fireStation.addApparatusToMap(ApparatusType::Rescue, rescues);
        fireStation.addApparatusToMap(ApparatusType::Hazard, hazards);
        fireStation.addApparatusToMap(ApparatusType::Squad, squads);
        fireStation.addApparatusToMap(ApparatusType::Fast, fasts);
        fireStation.addApparatusToMap(ApparatusType::Brush, brushes);
        fireStation.addApparatusToMap(ApparatusType::Boat, boats);
        fireStation.addApparatusToMap(ApparatusType::UTV, utvs);
        fireStation.addApparatusToMap(ApparatusType::Reach, reaches);
        fireStation.addApparatusToMap(ApparatusType::Chief, chiefs);

        fireStation.addApparatusToMap(ApparatusType::Medic, medics);
        fireStation.updateApparatusCounts();
        stations.emplace_back(fireStation);
        LOG_DEBUG("Loaded station: {}", stationId);
        stationIndex++;
        _stationIndex++;
    }

    file.close();
    LOG_INFO("Loaded {} stations from CSV file.", stations.size());
    LOG_WARN("Ignored {} stations that are out of bounds.", ignoredCount);

    return std::make_pair(stations, vehicles);
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
    std::cout<<"READ:" << filename <<std::endl;
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
        Location location;
        location.lat = lat;
        location.lon = lon;
        
        if (isPointInPolygon(polygon, location)) {
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

            IncidentType itype = mapIncidentType(type); // Use the new mapping function
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

// TODO: Add checking if the binary files already exist, if so, load them instead of generating them again.
void preComputingMatrices(std::vector<FireStation>& stations, 
                          std::vector<Incident>& incidents,
                          std::vector<Vehicle>& vehicles) {
    LOG_INFO("Starting Precomputation...");
    //spdlog::stopwatch sw;
    // Additional logic can be added here
    std::shared_ptr<EnvLoader> env = EnvLoader::getInstance();
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

    auto [_stations, _vehicles] = loadStationsFromCSV();
    stations = std::move(_stations);
    // Note: Type conversion may be needed here if Vehicle != Apparatus
    vehicles = std::move(_vehicles);

    incidents = loadIncidentsFromCSV();

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
}

} // namespace loader

