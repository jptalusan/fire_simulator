#ifndef COMMON_H
#define COMMON_H

#include <sstream>
#include <iomanip>

struct Location {
    double lat;
    double lon;

    Location() : lat(0.0), lon(0.0) {}
    Location(double latitude, double longitude) : lat(latitude), lon(longitude) {}
};

inline std::string locationToString(const Location& location) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << location.lon << "," << location.lat; // lon,lat
    return oss.str();
}
#endif