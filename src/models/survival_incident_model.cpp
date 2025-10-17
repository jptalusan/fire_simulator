#include "models/incident_model.h"
#include "io/loaders.h"
#include "utils/logger.h"
#include <algorithm>
#include <fstream>
#include <iostream>

bool SurvivalIncidentModel::load(const std::vector<Incident>& incidents) {
    return false;
}

bool SurvivalIncidentModel::load(const std::string& csvPath) {
    return false;
}

bool SurvivalIncidentModel::load() {
    return false;
}

std::optional<Incident> SurvivalIncidentModel::getNextIncident(std::time_t time) {
    return std::nullopt;
}