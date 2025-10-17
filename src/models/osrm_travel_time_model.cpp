#include "services/chunks.h"
#include "models/travel_time_model.h"


std::pair<float, std::vector<Location>> OSRMTravelTimeModel::getTravelTimeAndRoute(const Location& from, const Location& to) {
    // Implementation for getting travel time and route using OSRM
    std::pair<float, std::vector<Location>> travelTimeAndRoute = generate_route(from, to);
    return travelTimeAndRoute;
}

std::vector<std::vector<double>> OSRMTravelTimeModel::getTravelTimeMatrix(const std::vector<Location>& sources, const std::vector<Location>& destinations) {
    // Implementation for getting travel time matrix using OSRM
    size_t chunk_size = 100; // Define chunk size for OSRM table queries
    auto duration_matrix = generate_duration_traveltime_matrix(sources, destinations, chunk_size);
    return duration_matrix;
}