#include "data/geometry.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

double crossProduct(const Location& p1, const Location& p2, const Location& p3)
{
    return (p2.lat - p1.lat) * (p3.lon - p1.lon)
           - (p2.lon - p1.lon) * (p3.lat - p1.lat);
}

bool isPointOnSegment(const Location& p, const Location& p1, const Location& p2)
{
    return crossProduct(p1, p2, p) == 0
           && p.lat >= std::min(p1.lat, p2.lat)
           && p.lat <= std::max(p1.lat, p2.lat)
           && p.lon >= std::min(p1.lon, p2.lon)
           && p.lon <= std::max(p1.lon, p2.lon);
}

int windingNumber(const std::vector<Location>& polygon, const Location& point)
{
    int n = polygon.size();
    int windingNumber = 0;
    for (int i = 0; i < n; i++) {
        Location p1 = polygon[i];
        Location p2 = polygon[(i + 1) % n];
        if (isPointOnSegment(point, p1, p2)) {
            return 0;
        }
        if (p1.lon <= point.lon) {
            if (p2.lon > point.lon && crossProduct(p1, p2, point) > 0) {
                windingNumber++;
            }
        } else {
            if (p2.lon <= point.lon && crossProduct(p1, p2, point) < 0) {
                windingNumber--;
            }
        }
    }
    return windingNumber;
}

bool isPointInPolygon(const std::vector<Location>& polygon, const Location& point)
{
    return windingNumber(polygon, point) != 0;
}

std::vector<Location> loadPolygonFromGeoJSON(const std::string& filename)
{
    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Failed to open GeoJSON file. Accepting all points." << std::endl;
        return {
            Location(-180.0, -90.0),
            Location(-180.0,  90.0),
            Location( 180.0,  90.0),
            Location( 180.0, -90.0),
            Location(-180.0, -90.0)
        };
    }

    json geojson;
    input >> geojson;

    std::vector<Location> polygon;
    auto coords = geojson["features"][0]["geometry"]["coordinates"];
    if (!coords.empty() && coords[0].is_array()) {
        for (const auto& coord : coords[0]) {
            double lon = coord[0];
            double lat = coord[1];
            polygon.emplace_back(lon, lat);
        }
    }
    return polygon;
}