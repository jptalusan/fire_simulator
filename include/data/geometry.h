#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <string>

struct Point {
    double x, y;
    Point(double x_, double y_) : x(x_), y(y_) {}
};

double crossProduct(const Point& p1, const Point& p2, const Point& p3);
bool isPointOnSegment(const Point& p, const Point& p1, const Point& p2);
int windingNumber(const std::vector<Point>& polygon, const Point& point);
bool isPointInPolygon(const std::vector<Point>& polygon, const Point& point);
std::vector<Point> loadPolygonFromGeoJSON(const std::string& filename);

#endif // GEOMETRY_H