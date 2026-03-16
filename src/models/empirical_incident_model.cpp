#include "models/incident_model.h"
#include "io/loaders.h"
#include "utils/logger.h"
#include <algorithm>
#include <fstream>
#include "utils/constants.h"

bool EmpiricalIncidentModel::load(const std::string& csvPath) {
    try {
        // Check if file exists
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open incident CSV file: {}", csvPath);
            return false;
        }
        file.close();
        
        // Use existing loader function from the loader namespace
        incidents_ = loader::loadIncidentsFromCSV();
        
        // Sort incidents by report time
        sortIncidents();
        
        LOG_INFO("Loaded {} incidents from CSV file: {}", incidents_.size(), csvPath);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading incidents from CSV {}: {}", csvPath, e.what());
        return false;
    }
}

bool EmpiricalIncidentModel::load(const std::vector<Incident>& incidents) {
    incidents_ = incidents;
    sortIncidents();
    LOG_INFO("Loaded {} incidents from vector", incidents_.size());
    return true;
}

bool EmpiricalIncidentModel::load() {
    return false;
}

// TDOO: Critical. having an outstanding incident is important but it should be handled better
/*
what about other existing incidents that have just been "reported". right now they get overwritten reusling in only one outstanding incident at a time
*/
std::optional<Incident> EmpiricalIncidentModel::getNextIncident(std::time_t time) {
    // Loop to skip EMS-only incidents when EMS is disabled
    while (incidents_.size() > static_cast<size_t>(currentIncidentIdx_)) {
        // Get reference to the incident in the vector (not a copy)
        Incident& incident = incidents_.at(currentIncidentIdx_);
        auto it = std::find(outstandingIncidentIndices_.begin(), outstandingIncidentIndices_.end(), incident.incidentIndex);

        if (it != outstandingIncidentIndices_.end()) {
            incident.reportTime = time + constants::STEP_FORWARD_TIME;
            return incident;
        } else {
            if (incident.reportTime < time) {
                incident.reportTime = time;
            }
            std::unordered_map<ApparatusType, int> requiredApparatusMap = fireModel_.calculateApparatusCount(incident);

            // Strip all medic requirements when EMS is disabled
            if (disableEms_) {
                requiredApparatusMap.erase(ApparatusType::Medic);
                if (requiredApparatusMap.empty()) {
                    LOG_INFO("Skipping EMS-only incident {} (DISABLE_EMS=true)", incident.incident_id);
                    currentIncidentIdx_++;
                    continue;
                }
            }

            // print required apparatus map for debugging
            LOG_DEBUG("Incident {} requires apparatus:", incident.incidentIndex);
            for (const auto& [type, count] : requiredApparatusMap) {
                LOG_DEBUG("  {}: {}", to_string(type), count);
            }
            incident.setRequiredApparatusMap(requiredApparatusMap); // Set the required apparatus map for the incident
            incident.status = IncidentStatus::hasBeenReported; // Update status to reported
            return incident;
        }
    }

    LOG_WARN("No more incidents available in EmpiricalIncidentModel");
    return std::nullopt;
}

size_t EmpiricalIncidentModel::getIncidentCount() const {
    return incidents_.size();
}

void EmpiricalIncidentModel::clear() {
    incidents_.clear();
    LOG_DEBUG("Cleared all incidents from empirical model");
}

void EmpiricalIncidentModel::sortIncidents() {
    std::sort(incidents_.begin(), incidents_.end(),
        [](const Incident& a, const Incident& b) {
            return a.reportTime < b.reportTime;
        });
    LOG_DEBUG("Sorted {} incidents by report time", incidents_.size());
}