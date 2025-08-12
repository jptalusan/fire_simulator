#include "services/queries.h"
#include <curl/curl.h>
#include <cstdlib>
#include "utils/logger.h"
#include "services/chunks.h"
using json = nlohmann::json;


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
