#include "models/fire.h"
#include "simulator/state.h"
#include "utils/constants.h"
#include "utils/error.h"
#include <spdlog/spdlog.h>
#include <random>

// TODO: All placeholders, not working yet.
HardCodedFireModel::HardCodedFireModel(unsigned int seed)
    : rng_(seed), dist_(0.0, 1.0) {}

// Example implementation for HardCodedFireModel
double HardCodedFireModel::computeResolutionTime(State& state, const Incident& incident) {
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

    return estimatedResolutionTime;
}

// Example: compute a probability and sample true/false
bool HardCodedFireModel::shouldResolveIncident(double probability) {
    double sample = dist_(rng_);
    sample = std::clamp(sample, 0.1, 1.0);
    return sample < probability;
}
