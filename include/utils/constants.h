#pragma once

#include <string>

namespace constants {
    // String constants
    inline constexpr const char* INCIDENT_ID = "INCIDENT_ID";
    inline constexpr const char* INCIDENT_INDEX = "INCIDENT_INDEX";
    inline constexpr const char* STATION_ID = "STATION_ID";
    inline constexpr const char* STATION_INDEX = "STATION_INDEX";
    inline constexpr const char* ENGINE_COUNT = "ENGINE_COUNT";
    inline constexpr const char* DISPATCH_TIME = "DISPATCH_TIME";
    inline constexpr const char* TRAVEL_TIME = "TRAVEL_TIME";
    inline constexpr const char* DISTANCE = "DISTANCE";
    inline constexpr const char* RESOLUTION_TIME = "RESOLUTION_TIME";

    // Numeric constants
    inline constexpr int DEFAULT_NUM_FIRE_TRUCKS = 4;
    inline constexpr int DEFAULT_NUM_AMBULANCES = 0;
    inline constexpr double EARTH_RADIUS_KM = 6371.0;
    inline constexpr double SECONDS_IN_MINUTE = 60.0;

    // Incident Levels
    inline constexpr const char* INCIDENT_LEVEL_LOW = "Low";
    inline constexpr const char* INCIDENT_LEVEL_MODERATE = "Moderate";
    inline constexpr const char* INCIDENT_LEVEL_HIGH = "High";
    inline constexpr const char* INCIDENT_LEVEL_CRITICAL = "Critical";
}