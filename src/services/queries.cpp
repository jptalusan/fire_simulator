#include "services/queries.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <cstdlib>
#include "utils/logger.h"
using json = nlohmann::json;

// Helper for libcurl response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// TODO: Modify this so it can be used for other OSRM services, right now it is only for table service
Queries::Queries() {
    osrm_url = EnvLoader::getInstance()->get("OSRM_URL", "http://router.project-osrm.org/table/v1/driving/");
}

std::vector<double> parseResponse(const std::string& response, const std::string& feature) {
    std::vector<double> array;
    try {
        auto json_response = json::parse(response);
        if (json_response.contains(feature)) {
            for (const auto& row : json_response[feature]) {
                for (const auto& duration : row) {
                    if (duration.is_null()) {
                        // Handle null values (e.g., unreachable routes)
                        std::cerr << "Unreachable route detected.\n";
                        array.push_back(-1.0); // Indicate unreachable
                    } else {
                        array.push_back(duration.get<double>());
                    }
                }
            }
        } else {
            std::cerr << "Response does not contain '" << feature << "' field.\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }
    return array;
}

/**
 * @brief Builds the OSRM query URL for the table service.
 * @note Requires `std::setprecision(10)` to ensure high precision in coordinates.
 * 
 * @param sources Vector of source locations.
 * @param destinations Vector of destination locations.
 * @return The constructed URL as a string.
 */
std::string Queries::buildQueryURL(const std::vector<Location>& sources,
                                   const std::vector<Location>& destinations) {
    std::ostringstream url;
    url << std::setprecision(10); // Set precision to 10 decimal place
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

    // Add annotations parameter to get durations
    // url << "?overview=false";
    url << "&annotations=duration,distance";
    return url.str();
}

void Queries::queryTableService(const std::vector<Location>& sources,
                                const std::vector<Location>& destinations,
                                std::vector<double>& durations,
                                std::vector<double>& distances) {
    std::string url = buildQueryURL(sources, destinations);
    LOG_DEBUG("OSRM Query URL: {}", url);

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to init curl");
        return;
    }

    std::string response_string;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    CURLcode res = curl_easy_perform(curl);

    LOG_DEBUG("OSRM Response: {}", response_string);

    if (res != CURLE_OK) {
        LOG_ERROR("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    } else {
        durations = parseResponse(response_string, "durations");
        distances = parseResponse(response_string, "distances");
    }

    curl_easy_cleanup(curl);
}

// TODO: Update this to it picks the centroid of the bounds you are using.
bool checkOSRM(const std::string& base_url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string query_url = base_url + "/route/v1/driving/-86.7844,36.1659;-86.8005,36.1447";
    LOG_DEBUG("Checking OSRM with URL: {}", query_url);

    curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize curl.");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, query_url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    bool is_ok = false;

    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 200) {
            try {
                auto json_response = json::parse(readBuffer);
                if (json_response.contains("code") && json_response["code"] == "Ok") {
                    is_ok = true;
                }
            } catch (const json::parse_error& e) {
                LOG_ERROR("JSON parse error: {}", e.what());
            }
        } else {
            LOG_ERROR("HTTP error code: {}", http_code);
        }
    } else {
        LOG_ERROR("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return is_ok;
}