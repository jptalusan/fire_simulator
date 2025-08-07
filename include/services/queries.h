#ifndef QUERIES_H
#define QUERIES_H

#include <string>
#include <vector>
#include <utility>
#include "config/EnvLoader.h"
#include "data/location.h"
#include <nlohmann/json.hpp>

class Queries {
public:
    Queries();
    void queryTableService(const std::vector<Location>& sources,
                           const std::vector<Location>& destinations,
                           std::vector<double>& durations,
                           std::vector<double>& distances);

private:
    std::string osrm_url;
    std::string buildQueryURL(const std::vector<Location>& sources,
                              const std::vector<Location>& destinations);
};

bool checkOSRM(const std::string& base_url);

#endif // QUERIES_H
