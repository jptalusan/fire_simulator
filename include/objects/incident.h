// incident.h
#ifndef INCIDENT_H
#define INCIDENT_H

#include <vector>
#include <unordered_map>
#include <string>
#include <ctime>
#include "enums.h"
#include "objects/common.h"

// TODO: Modify stationIndex to potentially be a vector of station indices if multiple stations can respond to an incident.
class Incident {
public:
    double lat;
    double lon;
    time_t reportTime;
    time_t originalReportTime; // Time when the incident was originally reported
    time_t timeRespondedTo; // Time when the incident was responded to
    time_t resolvedTime; // Time when the incident was resolved
    int incidentIndex;
    int incident_id;
    int zoneIndex;

    IncidentType incident_type;
    std::string incident_type_str; // Original incident type string from CSV (for ML model)
    IncidentLevel incident_level;
    IncidentStatus status;
    IncidentCategory category; // Category of the incident
    bool emsAssigned = false;          // First Medic claimed EMS (handles scene time + transport)
    int emsStayBehindVehicleId = -1;   // Vehicle ID of the Medic staying behind at scene (-1 = none assigned)

    std::vector<std::tuple<int, int, double>> apparatusReceived; // Maps station index to (number of apparatus, travel time)
    std::unordered_map<ApparatusType, int> requiredApparatusMap; // How many apparatus is needed for this incident
    std::unordered_map<ApparatusType, int> currentApparatusMap; // How many apparatus are currently at the incident (or have been assigned to the incident)

    Incident(int index, int id, double latitude, double longitude,
             IncidentType type, IncidentLevel level,
             time_t report_time, IncidentCategory category);

    // Default constructor
    Incident()
        : lat(0.0),
          lon(0.0),
          reportTime(std::time(nullptr)),
          originalReportTime(std::time(nullptr)),
          timeRespondedTo(std::time(nullptr)),
          resolvedTime(std::time(nullptr)),
          incidentIndex(-1),
          incident_id(-1),
          zoneIndex(-1),
          incident_type(IncidentType::Invalid),
          incident_level(IncidentLevel::Invalid),
          status(IncidentStatus::hasBeenReported),
          category(IncidentCategory::Invalid) {} // Default values

    void printInfo() const;
    Location getLocation() const;
    void setRequiredApparatusMap(const std::unordered_map<ApparatusType, int>& requiredApparatusMap);
    int getCurrentApparatusCount() const;
    int getTotalApparatusRequired() const;
};

#endif // INCIDENT_H
