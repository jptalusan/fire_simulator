#ifndef INCIDENT_MODEL_H
#define INCIDENT_MODEL_H

#include <ctime>
#include <vector>
#include <string>
#include <optional>
#include "objects/incident.h"
#include "models/fire_model.h"

/**
 * @brief Base class for incident models
 * 
 * This abstract class defines the interface for incident models that can
 * provide incidents based on time queries.
 */
class IncidentModel {
public:
    IncidentModel(ServiceTimeAndApparatusModel& fireModel) : fireModel_(fireModel) {}
    virtual ~IncidentModel() = default;
    
    /**
     * @brief Get the next incident at or after the given time
     * @param time The time to query for incidents
     * @return Optional incident if found, nullopt if no incident exists at or after the given time
     */
    virtual std::optional<Incident> getNextIncident(std::time_t time) = 0;
    virtual bool load(const std::vector<Incident>& incidents) = 0;
    virtual bool load(const std::string& csvPath) = 0;
    virtual bool load() = 0;
protected:
    ServiceTimeAndApparatusModel& fireModel_;
};

/**
 * @brief Empirical incident model that loads incidents from data sources
 * 
 * This model loads incidents from CSV files or vectors and serves them
 * chronologically based on their report times.
 */
class EmpiricalIncidentModel : public IncidentModel {
public:
    EmpiricalIncidentModel(ServiceTimeAndApparatusModel& fireModel) : IncidentModel(fireModel) {}
    bool load() override;
    
    /**
     * @brief Load incidents from a CSV file
     * @param csvPath Path to the CSV file containing incident data
     * @return true if loading was successful, false otherwise
     */
    bool load(const std::string& csvPath) override;
    
    /**
     * @brief Load incidents from a vector
     * @param incidents Vector of incidents to load
     */
    bool load(const std::vector<Incident>& incidents) override;
    
    /**
     * @brief Get the next incident at or after the given time
     * @param time The time to query for incidents
     * @return Optional incident if found, nullopt if no incident exists at or after the given time
     */
    std::optional<Incident> getNextIncident(std::time_t time) override;
    
    /**
     * @brief Get the total number of loaded incidents
     * @return Number of incidents in the model
     */
    size_t getIncidentCount() const;
    
    /**
     * @brief Clear all loaded incidents
     */
    void clear();

private:
    std::vector<Incident> incidents_;
    
    /**
     * @brief Sort incidents by report time
     */
    void sortIncidents();
};

/**
 * @brief Empirical incident model that loads incidents from data sources
 * 
 * This model loads incidents from CSV files or vectors and serves them
 * chronologically based on their report times.
 */
class SurvivalIncidentModel : public IncidentModel {
public:
    SurvivalIncidentModel(ServiceTimeAndApparatusModel& fireModel) : IncidentModel(fireModel) {}
    bool load(const std::string& csvPath) override;
    bool load(const std::vector<Incident>& incidents) override;
    bool load() override;
    std::optional<Incident> getNextIncident(std::time_t time) override;
    size_t getIncidentCount() const;
    void clear();

private:
    std::vector<Incident> incidents_;
    void sortIncidents();
};

#endif // INCIDENT_MODEL_H
