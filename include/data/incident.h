// incident.h
#ifndef INCIDENT_H
#define INCIDENT_H

#include <string>
#include <vector>
#include <ctime>
#include "data/location.h"
#include "simulator/event.h"

// TODO: Modify stationIndex to potentially be a vector of station indices if multiple stations can respond to an incident.
class Incident : public EventData {
public:
    int incidentIndex;
    int incident_id;
    double lat;
    double lon;
    std::string incident_type;
    IncidentLevel incident_level;
    time_t reportTime;
    time_t timeRespondedTo; // Time when the incident was responded to
    time_t resolvedTime; // Time when the incident was resolved
    int totalApparatusRequired;
    int currentApparatusCount;
    IncidentStatus status;
    std::vector<std::tuple<int, int, double>> apparatusReceived; // Maps station index to (number of apparatus, travel time)

    Incident(int index, int id, double latitude, double longitude,
             const std::string& type, IncidentLevel level,
             time_t report_time);
    Incident()
        : incidentIndex(-1),
          incident_id(-1),
          lat(0.0),
          lon(0.0),
          incident_type(""),
          incident_level(IncidentLevel::Invalid),
          reportTime(std::time(nullptr)),
          timeRespondedTo(std::time(nullptr)),
          resolvedTime(std::time(nullptr)),
          totalApparatusRequired(0),
          currentApparatusCount(0),
          status(IncidentStatus::hasBeenReported)
    {}

    void printInfo() const override;
    Location getLocation() const;
};

// Declaration of the loading function
std::vector<Incident> loadIncidentsFromCSV(const std::string& filename);

#endif // INCIDENT_H
