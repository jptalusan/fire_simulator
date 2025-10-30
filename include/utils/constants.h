#pragma once

#include <string>

namespace constants {
    // String constants
    inline static constexpr const char* INCIDENT_ID = "INCIDENT_ID";
    inline static constexpr const char* INCIDENT_INDEX = "INCIDENT_INDEX";
    inline static constexpr const char* STATION_ID = "STATION_ID";
    inline static constexpr const char* STATION_INDEX = "STATION_INDEX";
    inline static constexpr const char* ENGINE_COUNT = "ENGINE_COUNT";
    inline static constexpr const char* DISPATCH_TIME = "DISPATCH_TIME";
    inline static constexpr const char* TRAVEL_TIME = "TRAVEL_TIME";
    inline static constexpr const char* DISTANCE = "DISTANCE";
    inline static constexpr const char* RESOLUTION_TIME = "RESOLUTION_TIME";
    // TODO: Should change these other non-policy names.
    inline static constexpr const char* POLICY_FIREBEATS = "FIREBEATS";
    inline static constexpr const char* POLICY_NEAREST = "NEAREST";
    inline static constexpr const char* POLICY_EMPIRICAL = "EMPIRICAL";
    inline static constexpr const char* POLICY_OSRM = "OSRM";
    inline static constexpr const char* POLICY_GIS = "GIS";

    // Numeric constants
    inline static constexpr int DEFAULT_NUM_FIRE_TRUCKS = 2;
    inline static constexpr int DEFAULT_NUM_AMBULANCES = 0;
    inline static constexpr double EARTH_RADIUS_KM = 6371.0;
    inline static constexpr double SECONDS_IN_MINUTE = 60.0;
    inline static constexpr double RESPOND_DELAY_SECONDS = 60.0;
    inline static constexpr double DISPATCH_BUFFER_SECONDS = 600.0;

    inline static constexpr time_t STEP_FORWARD_TIME = 300; // 5 minutes

    // Incident Levels
    inline static constexpr const char* INCIDENT_LEVEL_LOW = "Low";
    inline static constexpr const char* INCIDENT_LEVEL_MODERATE = "Moderate";
    inline static constexpr const char* INCIDENT_LEVEL_HIGH = "High";
    inline static constexpr const char* INCIDENT_LEVEL_CRITICAL = "Critical";

    // ENV variable keys
    inline static constexpr const char* INCIDENTS_CSV_PATH = "INCIDENTS_CSV_PATH";
    inline static constexpr const char* POLICY_DISPATCH = "DISPATCH_POLICY";
    inline static constexpr const char* POLICY_FIRE_MODEL = "FIRE_MODEL_TYPE";
    inline static constexpr const char* POLICY_INCIDENT_MODEL = "INCIDENT_MODEL_TYPE";
    inline static constexpr const char* POLICY_TRAVEL_TIME_MODEL = "TRAVEL_TIME_MODEL_TYPE";
    inline static constexpr const char* RESOLUTION_STATS_CSV_PATH = "RESOLUTION_STATS_CSV_PATH";
    inline static constexpr const char* MODEL_PATH = "MODEL_PATH";
    inline static constexpr const char* FEATURES_PATH = "FEATURES_PATH";
    inline static constexpr const char* NFD_RESPONSE_CSV_PATH = "NFD_RESPONSE_CSV_PATH";
    inline static constexpr const char* RANDOM_SEED = "RANDOM_SEED";
    inline static constexpr const char* FIREBEATS_MATRIX_PATH = "FIREBEATS_MATRIX_PATH";
    inline static constexpr const char* ZONE_MAP_PATH = "ZONE_MAP_PATH";
}