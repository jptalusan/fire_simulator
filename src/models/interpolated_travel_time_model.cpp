#include "models/travel_time_model.h"
#include "services/chunks.h"
#include "config/EnvLoader.h"
#include "objects/geometry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <algorithm>
#include <random>
#include <vector>
#include <limits>
#include <cstdint>
#include <spdlog/spdlog.h>

InterpolatedTravelTimeModel::InterpolatedTravelTimeModel(const std::string& mean_matrix_path,
                                                       const std::string& std_matrix_path,
                                                       const std::string& zone_info_path)
 
{
    spdlog::info("Initializing InterpolatedTravelTimeModel with data files:");
    spdlog::info("  Mean matrix: {}", mean_matrix_path);
    spdlog::info("  Std matrix: {}", std_matrix_path);
    spdlog::info("  Zone info: {}", zone_info_path);
    
    loadMeanMatrix(mean_matrix_path);
    loadStdMatrix(std_matrix_path);
    loadZoneInfo(zone_info_path);
    loadZoneGeometries();
    
    // Initialize spatial cache
    zone_cache_.reserve(1000); // Reserve space for common lookups
    
    // Precompute stations with travel time data for faster lookup
    stations_with_data_.reserve(zone_info_.size());
    for (const auto& [zone_id, zone_data] : zone_info_) {
        if (travel_time_mean_matrix_.find(zone_id) != travel_time_mean_matrix_.end()) {
            stations_with_data_.emplace_back(zone_id, zone_data);
        }
    }
    
    spdlog::info("InterpolatedTravelTimeModel initialized successfully with {} fire station zones and {} zone geometries", 
                 zone_info_.size(), zone_polygons_.size());
}

std::vector<std::vector<double>> InterpolatedTravelTimeModel::getTravelTimeMatrix(
    const std::vector<Location>& sources, 
    const std::vector<Location>& destinations) {
    
    std::vector<std::vector<double>> matrix;
    matrix.reserve(sources.size());
    
    for (size_t i = 0; i < sources.size(); ++i) {
        std::vector<double> row;
        row.reserve(destinations.size());
        
        for (size_t j = 0; j < destinations.size(); ++j) {
            double travel_time = interpolateTravelTimeMatrixBased(
                sources[i].lat, sources[i].lon, 
                destinations[j].lat, destinations[j].lon
            );
            row.push_back(travel_time);
        }
        matrix.push_back(std::move(row));
    }
    
    return matrix;
}

std::pair<float, std::vector<Location>> InterpolatedTravelTimeModel::getTravelTimeAndRoute(
    const Location& from, const Location& to) {
    
    // Calculate travel time using our interpolated model
    double interpolated_travel_time = interpolateTravelTimeMatrixBased(from.lat, from.lon, to.lat, to.lon);
    
    // Generate route using OSRM
    auto osrm_result = generate_route(from, to);
    std::vector<Location> osrm_route = osrm_result.second;
    
    
    // Use our interpolated travel time but OSRM's route
    return {static_cast<float>(interpolated_travel_time), osrm_route};
}

double InterpolatedTravelTimeModel::interpolateTravelTimeMatrixBased(
    double source_lat, double source_lon,
    double dest_lat, double dest_lon) const {
    
    // Step 1: Get zones for incident and source
    std::string incident_zone = findZone(dest_lat, dest_lon);
    if (incident_zone.empty()) {
        return haversineDistance(source_lat, source_lon, dest_lat, dest_lon); // Direct distance fallback
    }
    
    // Step 2: Check if source is within any existing fire station zone
    std::string source_station_zone = findZone(source_lat, source_lon);
    
    // Step 3: Apply algorithm based on whether source zone exists in matrix
    if (!source_station_zone.empty() && travel_time_mean_matrix_.find(source_station_zone) != travel_time_mean_matrix_.end()) {
        // Case 1: Source zone exists in travel time matrix
        return calculateWithExistingZone(source_lat, source_lon, dest_lat, dest_lon,
                                        source_station_zone, incident_zone);
    } else {
        // Case 2: Source zone doesn't exist, use top 3 nearest stations
        return calculateWithNearestStations(source_lat, source_lon, dest_lat, dest_lon, incident_zone);
    }
}

double InterpolatedTravelTimeModel::calculateWithExistingZone(
    double source_lat, double source_lon,
    double incident_lat, double incident_lon,
    const std::string& source_zone, const std::string& incident_zone) const {
    
    // Get original fire station coordinates
    auto zone_it = zone_info_.find(source_zone);
    if (zone_it == zone_info_.end()) {
        throw std::runtime_error("Missing zone info for source zone: " + source_zone);
    }
    
    double original_station_lat = zone_it->second.centroid_lat;
    double original_station_lon = zone_it->second.centroid_lon;
    
    // Calculate distances (convert to km for consistency with Python)
    double original_distance_km = haversineDistance(original_station_lat, original_station_lon,
                                                   incident_lat, incident_lon) / 1000.0;
    double source_distance_km = haversineDistance(source_lat, source_lon,
                                                 incident_lat, incident_lon) / 1000.0;
    
    // Get travel time from matrix
    auto source_it = travel_time_mean_matrix_.find(source_zone);
    if (source_it == travel_time_mean_matrix_.end()) {
        throw std::runtime_error("Missing travel time data for source zone: " + source_zone);
    }
    
    auto dest_it = source_it->second.find(incident_zone);

    if (dest_it == source_it->second.end()) {
        throw std::runtime_error("No mean travel time data from " + source_zone + " to " + incident_zone);
    }
    
    
    double mean_time = dest_it->second;
    if (mean_time <= 0) {
        throw std::runtime_error("Non-positive mean travel time from " + source_zone + " to " + incident_zone);
    }
    
    // Apply distance ratio with bounds checking
    double distance_ratio = 1.0;
    if (original_distance_km > 0) {
        distance_ratio = source_distance_km / original_distance_km;
        // Clamp distance ratio to reasonable bounds to prevent extreme values
        distance_ratio = std::max(0.1,  distance_ratio);
    }
    
    // Sample from Gaussian distribution if std data available
    double sampled_time = mean_time;
    auto std_source_it = travel_time_std_matrix_.find(source_zone);
    if (std_source_it != travel_time_std_matrix_.end()) {
        auto std_dest_it = std_source_it->second.find(incident_zone);
        if (std_dest_it != std_source_it->second.end()) {
            double std_time = std_dest_it->second;
            if (std_time > 0) {
                sampled_time = sampleFromGaussian(mean_time, std_time);
            }
        }
    }
    
    double adjusted_time = sampled_time * distance_ratio;
    
    // Apply final minimum constraint - at least 60 seconds for any travel

    
    spdlog::debug("Existing zone: {} -> {} | Mean: {:.1f}s | Ratio: {:.3f} | Final: {:.1f}s",
                  source_zone, incident_zone, mean_time, distance_ratio, adjusted_time);
    
    return adjusted_time;
}

double InterpolatedTravelTimeModel::calculateWithNearestStations(
    double source_lat, double source_lon,
    double incident_lat, double incident_lon,
    const std::string& incident_zone) const {
    
    // Find nearest fire stations using precomputed list
    struct StationCandidate {
        std::string zone_id;
        double distance_to_station_km;
        const ZoneData* station_info;
    };
    
    std::vector<StationCandidate> station_candidates;
    station_candidates.reserve(std::min(static_cast<size_t>(10), stations_with_data_.size())); // Only check top 10 candidates
    
    // Fast distance pre-filter - only calculate exact distances for nearby stations
    for (const auto& [zone_id, zone_data] : stations_with_data_) {
        // Quick Manhattan distance filter first (much faster than haversine)
        // double lat_diff = std::abs(source_lat - zone_data.centroid_lat);
        // double lon_diff = std::abs(source_lon - zone_data.centroid_lon);
        
        // Skip obviously distant stations (rough filter ~100km in Nashville area)
        // if (lat_diff > 1.0 || lon_diff > 1.0) continue;
        
        double distance_to_station_km = haversineDistance(source_lat, source_lon,
                                                         zone_data.centroid_lat, zone_data.centroid_lon) / 1000.0;
        station_candidates.push_back({zone_id, distance_to_station_km, &zone_data});
        
        // Early termination if we have enough candidates
      
    }
    
    if (station_candidates.empty()) {
       throw std::runtime_error("No fire station zones with travel time data available for nearest station calculation");
    }
    
    // Partial sort to get only top 3 - much faster than full sort
    size_t max_stations = std::min(static_cast<size_t>(3), station_candidates.size());
    std::partial_sort(station_candidates.begin(), station_candidates.begin() + max_stations, station_candidates.end(),
              [](const StationCandidate& a, const StationCandidate& b) {
                  return a.distance_to_station_km < b.distance_to_station_km;
              });
    
    // Calculate weighted average
    double weighted_sum = 0.0;
    double total_weight = 0.0;
    
    for (size_t i = 0; i < max_stations; ++i) {
        const auto& candidate = station_candidates[i];
        
        // Calculate distances (in km)
        double station_to_incident_distance_km = haversineDistance(
            candidate.station_info->centroid_lat, candidate.station_info->centroid_lon,
            incident_lat, incident_lon) / 1000.0;
        
        double source_to_incident_distance_km = haversineDistance(
            source_lat, source_lon, incident_lat, incident_lon) / 1000.0;
        
        // Get travel time from matrix
        auto source_it = travel_time_mean_matrix_.find(candidate.zone_id);
        if (source_it == travel_time_mean_matrix_.end()) continue;
        
        auto dest_it = source_it->second.find(incident_zone);
        if (dest_it == source_it->second.end()) continue;
        
        double mean_time = dest_it->second;
        if (mean_time <= 0) continue;
        
        // Apply distance ratio with bounds checking
        double distance_ratio = 1.0;
        if (station_to_incident_distance_km > 0) {
            distance_ratio = source_to_incident_distance_km / station_to_incident_distance_km;
            // Clamp distance ratio to reasonable bounds to prevent extreme values
            distance_ratio = std::max(0.1, distance_ratio);
        }
        
        // Sample from Gaussian distribution
        double sampled_time = mean_time;
        auto std_source_it = travel_time_std_matrix_.find(candidate.zone_id);
        if (std_source_it != travel_time_std_matrix_.end()) {
            auto std_dest_it = std_source_it->second.find(incident_zone);
            if (std_dest_it != std_source_it->second.end()) {
                double std_time = std_dest_it->second;
                if (std_time > 0) {
                    sampled_time = sampleFromGaussian(mean_time, std_time);
                }
            }
        }
        
        double adjusted_time = sampled_time * distance_ratio;
        

        
        // Weight by inverse distance to fire station
        double weight = 1.0 / (candidate.distance_to_station_km + 0.01); // Add small constant to avoid div by zero
        
        weighted_sum += adjusted_time * weight;
        total_weight += weight;
    }
    
    if (total_weight > 0) {
        return weighted_sum / total_weight;
    } else {
        // Final fallback
        throw std::runtime_error("Could not calculate travel time using nearest stations for incident zone: " + incident_zone);
    }
}

void InterpolatedTravelTimeModel::loadMeanMatrix(const std::string& mean_matrix_path) {
    std::ifstream file(mean_matrix_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open mean travel time matrix file: " + mean_matrix_path);
    }
    
    nlohmann::json matrix_json;
    file >> matrix_json;
    
    spdlog::info("Loading mean travel time matrix from: {}", mean_matrix_path);
    
    // Load mean travel time matrix: {"source_zone": {"dest_zone": mean_travel_time, ...}, ...}
    for (const auto& [source_zone, destinations] : matrix_json.items()) {
        for (const auto& [dest_zone, travel_time] : destinations.items()) {
            travel_time_mean_matrix_[source_zone][dest_zone] = travel_time.get<double>();
        }
    }
    
    spdlog::info("Loaded mean travel time matrix with {} source zones", travel_time_mean_matrix_.size());
}

void InterpolatedTravelTimeModel::loadStdMatrix(const std::string& std_matrix_path) {
    std::ifstream file(std_matrix_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open std travel time matrix file: " + std_matrix_path);
    }
    
    nlohmann::json matrix_json;
    file >> matrix_json;
    
    spdlog::info("Loading std travel time matrix from: {}", std_matrix_path);
    
    // Load std deviation travel time matrix: {"source_zone": {"dest_zone": std_travel_time, ...}, ...}
    for (const auto& [source_zone, destinations] : matrix_json.items()) {
        for (const auto& [dest_zone, std_time] : destinations.items()) {
            travel_time_std_matrix_[source_zone][dest_zone] = std_time.get<double>();
        }
    }
    
    spdlog::info("Loaded std travel time matrix with {} source zones", travel_time_std_matrix_.size());
}

void InterpolatedTravelTimeModel::loadZoneInfo(const std::string& zone_info_path) {
    std::ifstream file(zone_info_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open zone info file: " + zone_info_path);
    }
    
    nlohmann::json zone_json;
    file >> zone_json;
    
    spdlog::info("Loading fire station zone info from: {}", zone_info_path);
    
    // Expect format: {"zone_id": {"lat": "latitude", "lon": "longitude"}, ...}
    for (const auto& [zone_id, zone_data] : zone_json.items()) {
        ZoneData zone;
        zone.zone_id = zone_id;
        
        // Parse lat/lon from strings
        std::string lat_str = zone_data["lat"].get<std::string>();
        std::string lon_str = zone_data["lon"].get<std::string>();
        
        zone.centroid_lat = std::stod(lat_str);
        zone.centroid_lon = std::stod(lon_str);
        
        zone_info_[zone_id] = zone;
    }
    
    spdlog::info("Loaded fire station info for {} zones", zone_info_.size());
}

void InterpolatedTravelTimeModel::loadZoneGeometries() {
    // Use environment configuration to get beats shapefile path
    std::shared_ptr<EnvLoader> env = EnvLoader::getInstance();
    std::string beats_geojson_path = env->get("BEATS_SHAPEFILE_PATH", "../data/beats_shpfile.geojson");
    
    // Use the same efficient geometry loading as loaders.cpp
    std::vector<std::pair<int, Polygon>> polygonWithZoneID = loadServiceZonesFromGeojson(beats_geojson_path);
    
    // Convert to our unordered_map format
    for (const auto& pair : polygonWithZoneID) {
        std::string zone_id = std::to_string(pair.first);
        // Convert single Polygon to MultiPolygon 
        boost::geometry::model::multi_polygon<Polygon> multi_poly;
        multi_poly.push_back(pair.second);
        zone_polygons_[zone_id] = multi_poly;
    }
    
    spdlog::info("Loaded {} zone geometries for geographical lookup", zone_polygons_.size());
}

std::string InterpolatedTravelTimeModel::findZone(double lat, double lon) const {
    // Create cache key with reduced precision for spatial locality
    int lat_key = static_cast<int>(lat * 10000); // ~10m precision
    int lon_key = static_cast<int>(lon * 10000);
    uint64_t cache_key = (static_cast<uint64_t>(lat_key) << 32) | static_cast<uint64_t>(lon_key);
    
    // Check cache first
    auto cache_it = zone_cache_.find(cache_key);
    if (cache_it != zone_cache_.end()) {
        return cache_it->second;
    }
    
    // Use efficient point-in-polygon testing like loaders.cpp
    Point point(lon, lat); // Note: boost geometry uses (x, y) = (lon, lat)
    
    std::string result = "";
    // Use the same efficient lookup as getPointToPolygonIndices but for single point
    for (const auto& [zone_id, multi_polygon] : zone_polygons_) {
        if (boost::geometry::within(point, multi_polygon)) {
            result = zone_id;
            break;
        }
    }
    
    // Cache the result
    zone_cache_[cache_key] = result;
    
    return result;
}

std::string InterpolatedTravelTimeModel::findFireStationZone(double lat, double lon) const {
    // Use polygon geometries to check if source point is within any fire station zone
    Point point(lon, lat); // Note: boost geometry uses (x, y) = (lon, lat)
    
    // Check each zone polygon to see if the point is within it
    // and if that zone also exists in our fire station zone_info_
    for (const auto& [zone_id, multi_polygon] : zone_polygons_) {
        if (boost::geometry::within(point, multi_polygon)) {
            // Check if this zone_id exists in our fire station zone_info_
            // (some zones might be geographical zones that don't have fire stations)
            if (zone_info_.find(zone_id) != zone_info_.end()) {
                // Also verify this zone has travel time data
                if (travel_time_mean_matrix_.find(zone_id) != travel_time_mean_matrix_.end()) {
                    return zone_id;
                }
            }
        }
    }
    
    return ""; // No fire station zone found containing this point
}

std::string InterpolatedTravelTimeModel::findNearbyFireStationZone(double lat, double lon, double max_distance_km) const {
    // Find fire station zone within max_distance_km
    double max_distance_m = max_distance_km * 1000.0;
    
    for (const auto& [zone_id, zone_data] : zone_info_) {
        // Check if this zone has travel time data
        if (travel_time_mean_matrix_.find(zone_id) != travel_time_mean_matrix_.end()) {
            double distance = haversineDistance(lat, lon, zone_data.centroid_lat, zone_data.centroid_lon);
            if (distance < max_distance_m) {
                return zone_id;
            }
        }
    }
    
    return ""; // No nearby fire station found
}

double InterpolatedTravelTimeModel::haversineDistance(double lat1, double lon1, double lat2, double lon2) const {
    const double R = 6371000.0; // Earth's radius in meters
    
    double lat1_rad = lat1 * M_PI / 180.0;
    double lon1_rad = lon1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double lon2_rad = lon2 * M_PI / 180.0;
    
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;
    
    double a = std::sin(dlat/2) * std::sin(dlat/2) + 
               std::cos(lat1_rad) * std::cos(lat2_rad) * 
               std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::asin(std::sqrt(a));
    
    return R * c;
}

double InterpolatedTravelTimeModel::sampleFromGaussian(double mean, double std_dev) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::normal_distribution<double> distribution(mean, std_dev/2.0); // Reduce std dev to limit extremes
    // Remove the hardcoded 120-second minimum here - let the caller decide
    return std::max(30.0, distribution(gen)); // Only prevent negative/zero times
}

