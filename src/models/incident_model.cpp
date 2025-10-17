#include "models/incident_model.h"
#include "io/loaders.h"
#include "utils/logger.h"
#include <algorithm>
#include <fstream>
#include <iostream>

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

std::optional<Incident> EmpiricalIncidentModel::getNextIncident([[maybe_unused]] State& state,std::time_t time) {

    // Find the first incident with report time >= the given time
    auto it = std::upper_bound(incidents_.begin(), incidents_.end(), time,
        [](std::time_t t, const Incident& incident) {
            return t < incident.reportTime;
        });
    
    if (it != incidents_.end()) {
        std::unordered_map<ApparatusType, int> requiredApparatusMap = fireModel_.calculateApparatusCount(*it);
        it->setRequiredApparatusMap(requiredApparatusMap); // Set the required apparatus map for the incident
        it->status = IncidentStatus::hasBeenReported; // Update status to reported
        return *it;
    }
    
    // No incident found at or after the given time
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