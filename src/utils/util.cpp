#include "utils/util.h"
#include <ctime>
#include <cmath>

int extractHour(const std::time_t& timestamp) {
    std::tm* timeinfo = std::localtime(&timestamp);
    return timeinfo->tm_hour;
}

int extractDayOfWeek(const std::time_t& timestamp) {
    std::tm* timeinfo = std::localtime(&timestamp);
    return timeinfo->tm_wday; // 0=Sunday, 1=Monday, ..., 6=Saturday
}

int extractMonth(const std::time_t& timestamp) {
    std::tm* timeinfo = std::localtime(&timestamp);
    return timeinfo->tm_mon + 1; // tm_mon is 0-based, we want 1-12
}

int extractQuarter(const std::time_t& timestamp) {
    int month = extractMonth(timestamp);
    return ((month - 1) / 3) + 1; // Convert to quarter (1-4)
}

int extractDayOfYear(const std::time_t& timestamp) {
    std::tm* timeinfo = std::localtime(&timestamp);
    return timeinfo->tm_yday + 1; // tm_yday is 0-based, we want 1-366
}

int extractSeason(const std::time_t& timestamp) {
    int month = extractMonth(timestamp);
    if (month == 12 || month == 1 || month == 2) {
        return 0; // Winter
    } else if (month >= 3 && month <= 5) {
        return 1; // Spring
    } else if (month >= 6 && month <= 8) {
        return 2; // Summer
    } else {
        return 3; // Fall
    }
}

int extractShift(int hour) {
    if (hour >= 6 && hour < 14) {
        return 0; // Day shift
    } else if (hour >= 14 && hour < 22) {
        return 1; // Evening shift
    } else {
        return 2; // Night shift
    }
}

bool isHoliday(const std::time_t& timestamp) {
    std::tm* timeinfo = std::localtime(&timestamp);
    int month = timeinfo->tm_mon + 1;
    int day = timeinfo->tm_mday;
    
    // Simple holiday detection for major US holidays
    // New Year's Day
    if (month == 1 && day == 1) return true;
    
    // Independence Day
    if (month == 7 && day == 4) return true;
    
    // Christmas
    if (month == 12 && day == 25) return true;
    
    // Add more holidays as needed
    // This is a simplified version - in practice you'd want a more comprehensive holiday library
    
    return false;
}

double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 3959.0; // Earth's radius in miles
    
    // Convert degrees to radians
    lat1 = lat1 * M_PI / 180.0;
    lon1 = lon1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;
    lon2 = lon2 * M_PI / 180.0;
    
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;
    
    double a = std::sin(dlat/2) * std::sin(dlat/2) + 
               std::cos(lat1) * std::cos(lat2) * 
               std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    
    return R * c;
}

double calculateDistanceFromCenter(double lat, double lon, double center_lat, double center_lon) {
    return haversineDistance(lat, lon, center_lat, center_lon);
}
