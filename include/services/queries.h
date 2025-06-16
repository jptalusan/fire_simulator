#ifndef QUERIES_H
#define QUERIES_H

#include <string>
#include <vector>
#include <utility>
#include "config/EnvLoader.h"
#include "data/location.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

class Queries {
public:
    Queries();
    void queryTableService(const std::vector<Location>& sources,
                           const std::vector<Location>& destinations,
                           std::vector<double>& durations,
                           std::vector<double>& distances);

private:
    std::string osrm_url;
    void loadEnv();
    std::string buildQueryURL(const std::vector<Location>& sources,
                              const std::vector<Location>& destinations);

    EnvLoader env;  // Add EnvLoader instance
};

bool checkOSRM(const std::string& base_url);

#endif // QUERIES_H
