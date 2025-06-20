#include "models/fire.h"
#include "simulator/state.h"
#include "utils/constants.h"
#include <spdlog/spdlog.h>
#include <random>
#include <iostream>

// TODO: All placeholders, not working yet.
HardCodedFireModel::HardCodedFireModel(unsigned int seed)
    : rng_(seed), dist_(0.0, 1.0) {}

// Example implementation for HardCodedFireModel
double HardCodedFireModel::computeResolutionTime(State& state, const Incident& incident) {
    int currentApparatusCount = incident.currentApparatusCount;
    int totalApparatusRequired = incident.totalApparatusRequired;
    time_t reportTime = incident.reportTime;
    time_t currentTime = state.getSystemTime();
    IncidentLevel incidentLevel = incident.incident_level;

    double estimatedResolutionTime = 0.0;
    switch (incidentLevel) {
        case IncidentLevel::Low:        estimatedResolutionTime = 10 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Moderate:   estimatedResolutionTime = 30 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::High:       estimatedResolutionTime = 60 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Critical:   estimatedResolutionTime = 90 * constants::SECONDS_IN_MINUTE; break;
        default:
            spdlog::error("[HardCodedFireModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }

    double elapsedTime = difftime(currentTime, reportTime);

    // Don't even try to resolve before 50% of resolution time has elapsed
    if (elapsedTime < 0.5 * estimatedResolutionTime) {
        return false;
    }

    // Time factor (0.0 to 1.0) after minimum threshold
    double timeFactor = std::min(1.0, elapsedTime / estimatedResolutionTime);

    // Apparatus factor (0.0 to 1.0)
    double apparatusFactor = static_cast<double>(currentApparatusCount) / totalApparatusRequired;
    apparatusFactor = std::clamp(apparatusFactor, 0.0, 1.0);

    // Weighted probability
    double probability = 0.9 * timeFactor + 0.1 * apparatusFactor;

    // Sample and compare
    return shouldResolveIncident(probability);
}

// Example: compute a probability and sample true/false
bool HardCodedFireModel::shouldResolveIncident(double probability) {
    double sample = dist_(rng_);
    sample = std::clamp(sample, 0.1, 1.0);
    return sample < probability;
}