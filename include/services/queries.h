#ifndef QUERIES_H
#define QUERIES_H

#include <string>
#include <vector>
#include <utility>
#include "config/EnvLoader.h"
#include "data/location.h"

class Queries {
public:
    Queries();
    void queryTableService(const std::vector<Location>& sources,
                           const std::vector<Location>& destinations);

private:
    std::string osrm_url;
    void loadEnv();
    std::string buildQueryURL(const std::vector<Location>& sources,
                              const std::vector<Location>& destinations);

    EnvLoader env;  // Add EnvLoader instance
};

#endif // QUERIES_H
