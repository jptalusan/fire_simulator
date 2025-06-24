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

std::vector<std::optional<size_t>> getPointToPolygonIndices(
    const std::vector<Point>& points,
    const std::vector<Polygon>& polygons
) {
    // Build R-tree from bounding boxes
    std::vector<RTreeEntry> index_data;
    for (size_t i = 0; i < polygons.size(); ++i) {
        bg::model::box<Point> box;
        bg::envelope(polygons[i], box);
        index_data.emplace_back(box, i);
    }

    bgi::rtree<RTreeEntry, bgi::quadratic<16>> rtree(index_data.begin(), index_data.end());

    std::vector<std::optional<size_t>> result;
    result.reserve(points.size());

    for (const auto& pt : points) {
        std::vector<RTreeEntry> candidates;
        rtree.query(bgi::intersects(pt), std::back_inserter(candidates));  // changed here

        std::optional<size_t> containing_polygon = std::nullopt;

        for (const auto& [box, idx] : candidates) {
            if (bg::covered_by(pt, polygons[idx])) {  // or use covered_by if you want boundary inclusive
                containing_polygon = idx;
                break;
            }
        }

        result.push_back(containing_polygon);
    }
    return result;
}

std::vector<std::pair<std::string, Polygon>>  loadServiceZonesFromGeojson(const std::string& filename) {
    std::ifstream file(filename);
    nlohmann::json j;
    file >> j;

    std::vector<std::pair<std::string, Polygon>> polygons;

    if (j["type"] != "FeatureCollection") {
        throw std::runtime_error("Only FeatureCollection GeoJSON supported");
    }

    for (const auto& feature : j["features"]) {
        if (feature["geometry"]["type"] == "Polygon") {
            Polygon poly;
            std::string name;
            if (feature["properties"].contains("NAME")) {
                name = feature["properties"]["NAME"];
            } else if (feature["properties"].contains("name")) {
                name = feature["properties"]["name"];
            } else {
                throw std::runtime_error("Geojson feature is missing 'NAME' or 'name' property.");
            }

            for (const auto& ring : feature["geometry"]["coordinates"]) {
                std::vector<Point> points;
                for (const auto& coord : ring) {
                    double lon = coord[0];
                    double lat = coord[1];
                    points.emplace_back(lon, lat);
                }
                // The first ring is the outer ring
                if (&ring == &feature["geometry"]["coordinates"][0]) {
                    bg::append(poly.outer(), points);
                } else {
                    poly.inners().emplace_back();
                    bg::append(poly.inners().back(), points);
                }
            }
            bg::correct(poly);

            if (!bg::is_valid(poly)) {
                std::string reason;
                bg::validity_failure_type failure;
                bg::is_valid(poly, failure);
                    std::cerr << "Polygon from MultiPolygon '" << name << "' is invalid poly!\n";
            }
            polygons.push_back({name, poly});
        } else if (feature["geometry"]["type"] == "MultiPolygon") {
            throw std::runtime_error("Geojson for zones should only have Polygons, not multipolygons.");
        }
    }

    return polygons;
}