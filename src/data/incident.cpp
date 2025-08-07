#include <numeric>
#include "data/incident.h"
#include "utils/logger.h"

Incident::Incident(int index, int id, double latitude, double longitude,
                   IncidentType type, IncidentLevel level,
                   time_t time)
    : lat(latitude), 
      lon(longitude), 
      reportTime(time),
      timeRespondedTo(std::time(nullptr)),
      resolvedTime(std::time(nullptr)),
      incidentIndex(index), 
      incident_id(id),
      zoneIndex(-1),
      incident_type(type), 
      incident_level(level), 
      status(IncidentStatus::hasBeenReported) {}

void Incident::printInfo() const {
    LOG_ERROR("Incident Index: {}, ID: {}, Type: {}, Level: {}, Lat: {}, Lon: {}, Time: {}",
                incidentIndex, incident_id, to_string(incident_type), to_string(incident_level), lat, lon, reportTime);
}

Location Incident::getLocation() const {
    return Location(lat, lon);
}

void Incident::setRequiredApparatusMap(const std::unordered_map<ApparatusType, int>& requiredApparatusMap) {
    this->requiredApparatusMap = requiredApparatusMap;
}

void Incident::updateCurrentApparatusMap(const ApparatusType& type, int count) {
    if (currentApparatusMap.find(type) != currentApparatusMap.end()) {
        currentApparatusMap[type] += count;
    } else {
        currentApparatusMap[type] = count;
    }
    LOG_DEBUG("Updated current apparatus count for type {}: {}", to_string(type), currentApparatusMap[type]);
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