// incident.h
#ifndef INCIDENT_H
#define INCIDENT_H

#include <string>
#include <vector>
#include <ctime>
#include "data/location.h"
#include "simulator/event.h"

class Incident : public EventData {
public:
    int incident_id;
    double lat;
    double lon;
    std::string incident_type;
    IncidentLevel incident_level;
    time_t reportTime;
    time_t responseTime; // Time when the incident was responded to
    time_t resolvedTime; // Time when the incident was resolved
    bool hasBeenRespondedTo; // Flag to indicate if the incident has been responded to
    int engineCount; // Number of fire trucks dispatched to the incident
    int stationIndex; // Index of the station that responded to the incident
    double oneWayTravelTimeTo; // Travel time from the station to the incident location

    Incident(int id, double latitude, double longitude,
             const std::string& type, IncidentLevel level,
             time_t report_time);
    Incident()
        : incident_id(-1),
          lat(0.0),
          lon(0.0),
          incident_type(""),
          incident_level(IncidentLevel::Invalid),
          reportTime(0),
          responseTime(0),
          resolvedTime(0),
          hasBeenRespondedTo(false),
          engineCount(0),
          stationIndex(-1)
    {}

    void printInfo() const override;
    Location getLocation() const;
};

// Declaration of the loading function
std::vector<Incident> loadIncidentsFromCSV(const std::string& filename);

#endif // INCIDENT_H
