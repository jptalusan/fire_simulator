#include "data/geometry.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

double crossProduct(const Point& p1, const Point& p2, const Point& p3)
{
    return (p2.x - p1.x) * (p3.y - p1.y)
           - (p2.y - p1.y) * (p3.x - p1.x);
}

bool isPointOnSegment(const Point& p, const Point& p1, const Point& p2)
{
    return crossProduct(p1, p2, p) == 0
           && p.x >= std::min(p1.x, p2.x)
           && p.x <= std::max(p1.x, p2.x)
           && p.y >= std::min(p1.y, p2.y)
           && p.y <= std::max(p1.y, p2.y);
}

int windingNumber(const std::vector<Point>& polygon, const Point& point)
{
    int n = polygon.size();
    int windingNumber = 0;
    for (int i = 0; i < n; i++) {
        Point p1 = polygon[i];
        Point p2 = polygon[(i + 1) % n];
        if (isPointOnSegment(point, p1, p2)) {
            return 0;
        }
        if (p1.y <= point.y) {
            if (p2.y > point.y && crossProduct(p1, p2, point) > 0) {
                windingNumber++;
            }
        } else {
            if (p2.y <= point.y && crossProduct(p1, p2, point) < 0) {
                windingNumber--;
            }
        }
    }
    return windingNumber;
}

bool isPointInPolygon(const std::vector<Point>& polygon, const Point& point)
{
    return windingNumber(polygon, point) != 0;
}

std::vector<Point> loadPolygonFromGeoJSON(const std::string& filename)
{
    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Failed to open GeoJSON file. Accepting all points." << std::endl;
        return {
            Point(-180.0, -90.0),
            Point(-180.0,  90.0),
            Point( 180.0,  90.0),
            Point( 180.0, -90.0),
            Point(-180.0, -90.0)
        };
    }

    json geojson;
    input >> geojson;

    std::vector<Point> polygon;
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