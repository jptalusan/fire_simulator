#include <spdlog/spdlog.h>
#include "data/incident.h"

Incident::Incident(int index, int id, double latitude, double longitude,
                   IncidentType type, IncidentLevel level,
                   time_t time)
    : incidentIndex(index), incident_id(id), lat(latitude), lon(longitude),
      incident_type(type), incident_level(level), reportTime(time) {
        timeRespondedTo = std::time(nullptr);
        resolvedTime = std::time(nullptr);
        currentApparatusCount = 0;
        totalApparatusRequired = 0; // This can be set later based on the
        status = IncidentStatus::hasBeenReported; // Initially, the incident is not resolved
        zoneIndex = -1;
      }

void Incident::printInfo() const {
    spdlog::error("Incident Index: {}, ID: {}, Type: {}, Level: {}, Lat: {}, Lon: {}, Time: {}",
                incidentIndex, incident_id, to_string(incident_type), to_string(incident_level), lat, lon, reportTime);
}

Location Incident::getLocation() const {
    return Location(lat, lon);
}

