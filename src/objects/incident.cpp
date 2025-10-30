#include <numeric>
#include "objects/incident.h"
#include "utils/logger.h"

Incident::Incident(int index, int id, double latitude, double longitude,
                   IncidentType type, IncidentLevel level,
                   time_t time, IncidentCategory category)
    : lat(latitude), 
      lon(longitude), 
      reportTime(time),
      originalReportTime(time),
      timeRespondedTo(std::time(nullptr)),
      resolvedTime(std::time(nullptr)),
      incidentIndex(index), 
      incident_id(id),
      zoneIndex(-1),
      incident_type(type), 
      incident_level(level), 
      status(IncidentStatus::hasBeenReported), 
      category(category) {}

Location Incident::getLocation() const {
    return Location(lat, lon);
}

void Incident::setRequiredApparatusMap(const std::unordered_map<ApparatusType, int>& requiredApparatusMap) {
    this->requiredApparatusMap = requiredApparatusMap;
}

int Incident::getCurrentApparatusCount() const {
    return std::accumulate(currentApparatusMap.begin(), currentApparatusMap.end(), 0,
                           [](int sum, const std::pair<ApparatusType, int>& p) {
                               return sum + p.second;
                           });
}

int Incident::getTotalApparatusRequired() const {
    return std::accumulate(requiredApparatusMap.begin(), requiredApparatusMap.end(), 0,
                           [](int sum, const std::pair<ApparatusType, int>& p) {
                               return sum + p.second;
                           });
}

// print the apparatus type needed and how many is needed.
void Incident::printInfo() const {
    std::cout << "Incident Index: " << incidentIndex << ", ID: " << incident_id << "\n";
    std::cout << "Type: " << to_string(incident_type) << ", Level: " << to_string(incident_level) << "\n";
    std::cout << "Location: (" << locationToString(getLocation()) << ")\n";
    std::cout << "Report Time: " << std::asctime(std::localtime(&reportTime));
    std::cout << "Status: " << to_string(status) << "\n";
    std::cout << "Category: " << static_cast<int>(category) << "\n";

    std::cout << "Required Apparatus:\n";
    for (const auto& [type, count] : requiredApparatusMap) {
        std::cout << "  - " << to_string(type) << ": " << count << "\n";
    }

    std::cout << "Current Apparatus on Scene:\n";
    for (const auto& [type, count] : currentApparatusMap) {
        std::cout << "  - " << to_string(type) << ": " << count << "\n";
    }

    std::cout << "Total Apparatus Required: " << getTotalApparatusRequired() << "\n";
    std::cout << "Total Apparatus Currently on Scene: " << getCurrentApparatusCount() << "\n";
}