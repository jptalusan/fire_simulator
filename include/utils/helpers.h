#pragma once
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <string>
#include "simulator/event.h"
#include "data/incident.h"

inline std::string formatTime(std::time_t t) {
    std::ostringstream oss;
    std::tm tm = *std::localtime(&t);
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}