#ifndef TRAVEL_TIME_MODEL_H
#define TRAVEL_TIME_MODEL_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "objects/common.h"
#include "objects/geometry.h"
#include "utils/util.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>

class TravelTimeModel {
public:
    virtual std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) = 0;
    virtual std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) = 0;
    virtual ~TravelTimeModel() = default;
};

class OSRMTravelTimeModel : public TravelTimeModel {
public:
    OSRMTravelTimeModel(const std::string& base_url = "") : base_url_(base_url) {}
    std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) override;
    std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) override;
private:
    std::string base_url_;
};

class InterpolatedTravelTimeModel : public TravelTimeModel {
public:
    InterpolatedTravelTimeModel(const std::string& mean_matrix_path,
                               const std::string& std_matrix_path,
                               const std::string& zone_info_path);
    
    std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) override;
    std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) override;

private:
    struct ZoneData {
        std::string zone_id;
        double centroid_lat;
        double centroid_lon;
    };
    
    // Fire station zone information (for travel time matrix lookups)
    std::unordered_map<std::string, ZoneData> zone_info_;
    
    // Zone geometries from beats shapefile (for point-in-polygon zone lookup by ZONE_ID)
    // Using same types as loaders.cpp for efficiency
    std::unordered_map<std::string, boost::geometry::model::multi_polygon<Polygon>> zone_polygons_;
    
    // Travel time matrices (mean and std deviation)
    std::unordered_map<std::string, std::unordered_map<std::string, double>> travel_time_mean_matrix_;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> travel_time_std_matrix_;
    
    // Spatial cache for zone lookups
    mutable std::unordered_map<uint64_t, std::string> zone_cache_;
    
    // Precomputed stations with travel time data for faster nearest station lookup
    std::vector<std::pair<std::string, ZoneData>> stations_with_data_;
    

    
    // Helper methods
    void loadMeanMatrix(const std::string& mean_matrix_path);
    void loadStdMatrix(const std::string& std_matrix_path);
    void loadZoneInfo(const std::string& zone_info_path);
    void loadZoneGeometries(); // Load zone geometries from beats shapefile using environment
    std::string findZone(double lat, double lon) const;
    std::string findFireStationZone(double lat, double lon) const;
    std::string findNearbyFireStationZone(double lat, double lon, double max_distance_km = 2.0) const;
    double haversineDistance(double lat1, double lon1, double lat2, double lon2) const;
    double sampleFromGaussian(double mean, double std_dev) const;
    double interpolateTravelTimeMatrixBased(double source_lat, double source_lon,
                                           double dest_lat, double dest_lon) const;
    double calculateWithExistingZone(double source_lat, double source_lon,
                                    double incident_lat, double incident_lon,
                                    const std::string& source_zone, const std::string& incident_zone) const;
    double calculateWithNearestStations(double source_lat, double source_lon,
                                       double incident_lat, double incident_lon,
                                       const std::string& incident_zone) const;
};

class ESRITravelTimeModel : public TravelTimeModel {
public:
    ESRITravelTimeModel(const std::string& api_key) : api_key_(api_key) {}
    std::pair<float, std::vector<Location>> getTravelTimeAndRoute(const Location& from, const Location& to) override;
    std::vector<std::vector<double>> getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) override;
private:
    std::string api_key_;
};

#endif // TRAVEL_TIME_MODEL_H