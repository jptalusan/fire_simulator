// incident.h
#ifndef INCIDENT_H
#define INCIDENT_H

#include <string>
#include <vector>
#include <ctime>
#include "simulator/event_data.h"
#include "data/location.h"

class Incident : public EventData {
public:
    int incident_id;
    double lat;
    double lon;
    std::string incident_type;
    std::string incident_level;
    time_t unix_time;
    time_t response_time; // Time when the incident was responded to
    time_t resolved_time; // Time when the incident was resolved
    bool is_responded; // Flag to indicate if the incident has been responded to

    Incident(int id, double latitude, double longitude,
             const std::string& type, const std::string& level,
             time_t unix_time);
    Incident() = default;

    void printInfo() const override;
    Location getLocation() const;
};

// Declaration of the loading function
std::vector<Incident> loadIncidentsFromCSV(const std::string& filename);

#endif // INCIDENT_H
