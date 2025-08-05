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
    const time_t& stateTime = state.getSystemTime();

    double estimatedResolutionTime = 0.0;
    switch (incidentLevel) {
        case IncidentLevel::Low:        estimatedResolutionTime = 60 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Moderate:   estimatedResolutionTime = 90 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::High:       estimatedResolutionTime = 150 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Critical:   estimatedResolutionTime = 210 * constants::SECONDS_IN_MINUTE; break;
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

// HACK: Right now only engine type is required.
std::unordered_map<ApparatusType, int> HardCodedFireModel::calculateApparatusCount(const Incident& incident) {
    std::unordered_map<ApparatusType, int> apparatusCount;
    IncidentLevel incidentLevel = incident.incident_level;
    switch (incidentLevel) {
        case IncidentLevel::Low:
            apparatusCount[ApparatusType::Engine] = 1; // Example apparatus count for low-level incidents
            break;
        case IncidentLevel::Moderate:
            apparatusCount[ApparatusType::Engine] = 2; // Example apparatus count for moderate-level incidents
            break;
        case IncidentLevel::High:
            apparatusCount[ApparatusType::Engine] = 3; // Example apparatus count for high-level incidents
            break;
        case IncidentLevel::Critical:
            apparatusCount[ApparatusType::Engine] = 4; // Example apparatus count for critical-level incidents
            break;
        default:
            spdlog::error("[EnvironmentModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }
    return apparatusCount; // Return the calculated apparatus count map
}
