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

#endif // UTIL_H
