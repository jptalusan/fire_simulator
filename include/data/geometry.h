#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <nlohmann/json.hpp> // https://github.com/nlohmann/json
#include "data/location.h"
#include "utils/util.h"

double crossProduct(const Location& p1, const Location& p2, const Location& p3);
bool isPointOnSegment(const Location& p, const Location& p1, const Location& p2);
int windingNumber(const std::vector<Location>& polygon, const Location& point);
bool isPointInPolygon(const std::vector<Location>& polygon, const Location& point);
std::vector<Location> loadPolygonFromGeoJSON(const std::string& filename);
std::vector<std::optional<size_t>> getPointToPolygonIndices(
    const std::vector<Point>& points,
    const std::vector<Polygon>& polygons
);
std::vector<std::pair<int, Polygon>> loadServiceZonesFromGeojson(const std::string& filename);
#endif // GEOMETRY_H