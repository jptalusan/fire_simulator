#ifndef UTIL_H
#define UTIL_H

// Boost.Geometry common typedefs and namespaces
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <optional>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::d2::point_xy<double> Point;
typedef bg::model::polygon<Point> Polygon;
typedef bg::model::box<Point> Box;
typedef std::pair<Box, size_t> RTreeEntry;

int extractHour(const std::time_t& timestamp);
int extractDayOfWeek(const std::time_t& timestamp);
int extractMonth(const std::time_t& timestamp);
int extractQuarter(const std::time_t& timestamp);
int extractDayOfYear(const std::time_t& timestamp);
int extractSeason(const std::time_t& timestamp);
int extractShift(int hour);
bool isHoliday(const std::time_t& timestamp);
double haversineDistance(double lat1, double lon1, double lat2, double lon2);
double calculateDistanceFromCenter(double lat, double lon, double center_lat, double center_lon);
    

#endif // UTIL_H
