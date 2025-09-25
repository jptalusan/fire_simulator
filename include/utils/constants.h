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
    inline static constexpr const char* POLICY_FIREBEATS = "FIREBEATS";
    inline static constexpr const char* POLICY_NEAREST = "NEAREST";

    // Numeric constants
    inline static constexpr int DEFAULT_NUM_FIRE_TRUCKS = 2;
    inline static constexpr int DEFAULT_NUM_AMBULANCES = 0;
    inline static constexpr double EARTH_RADIUS_KM = 6371.0;
    inline static constexpr double SECONDS_IN_MINUTE = 60.0;
    inline static constexpr double RESPOND_DELAY_SECONDS = 30.0;
    inline static constexpr double DISPATCH_BUFFER_SECONDS = 600.0;

    // Incident Levels
    inline static constexpr const char* INCIDENT_LEVEL_LOW = "Low";
    inline static constexpr const char* INCIDENT_LEVEL_MODERATE = "Moderate";
    inline static constexpr const char* INCIDENT_LEVEL_HIGH = "High";
    inline static constexpr const char* INCIDENT_LEVEL_CRITICAL = "Critical";
}