#ifndef LOCATION_H
#define LOCATION_H

struct Location {
    double lat;
    double lon;

    Location() : lat(0.0), lon(0.0) {}
    Location(double latitude, double longitude) : lat(latitude), lon(longitude) {}
};

#endif // LOCATION_H