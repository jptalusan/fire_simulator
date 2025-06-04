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

    Incident(int id, double latitude, double longitude,
             const std::string& type, const std::string& level,
             time_t unix_time);

    void printInfo() const override;
    Location getLocation() const;
};

// Declaration of the loading function
std::vector<Incident> loadIncidentsFromCSV(const std::string& filename);

#endif // INCIDENT_H
