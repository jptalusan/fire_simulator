#include "services/queries.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <cstdlib>

// Helper for libcurl response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Queries::Queries() : env("../.env") {
    osrm_url = env.get("OSRM_URL", "http://router.project-osrm.org/table/v1/driving/");
}

std::string Queries::buildQueryURL(const std::vector<Location>& sources,
                                   const std::vector<Location>& destinations) {
    std::ostringstream url;
    url << osrm_url;

    // Combine sources and destinations into one coordinate list
    std::vector<Location> all_coords;
    all_coords.insert(all_coords.end(), sources.begin(), sources.end());
    all_coords.insert(all_coords.end(), destinations.begin(), destinations.end());

    for (size_t i = 0; i < all_coords.size(); ++i) {
        if (i > 0) url << ";";
        // OSRM expects lon,lat
        url << all_coords[i].lon << "," << all_coords[i].lat;
    }

    // Add sources and destinations parameters
    url << "?sources=";
    for (size_t i = 0; i < sources.size(); ++i) {
        if (i > 0) url << ";";
        url << i;
    }
    url << "&destinations=";
    for (size_t i = 0; i < destinations.size(); ++i) {
        if (i > 0) url << ";";
        url << (sources.size() + i);
    }

    return url.str();
}

void Queries::queryTableService(const std::vector<Location>& sources,
                                const std::vector<Location>& destinations) {
    std::string url = buildQueryURL(sources, destinations);
    std::cout << "Query URL: " << url << std::endl;

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to init curl\n";
        return;
    }

    std::string response_string;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << "\n";
    } else {
        std::cout << "Response:\n" << response_string << std::endl;
    }

    curl_easy_cleanup(curl);
}
