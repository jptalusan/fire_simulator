#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "data/location.h"
#include <vector>
#include <string>

double crossProduct(const Location& p1, const Location& p2, const Location& p3);
bool isPointOnSegment(const Location& p, const Location& p1, const Location& p2);
int windingNumber(const std::vector<Location>& polygon, const Location& point);
bool isPointInPolygon(const std::vector<Location>& polygon, const Location& point);
std::vector<Location> loadPolygonFromGeoJSON(const std::string& filename);

#endif // GEOMETRY_H