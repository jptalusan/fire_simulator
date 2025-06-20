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
    double prob = computeIncidentResolutionProbability(incident, state.getSystemTime());
    double resolutionTime = computeIndividualResolutionTime(incident, prob);
    return resolutionTime;
}

// Implementation of computeIndividualResolutionTime outside of computeResolutionTimes
double HardCodedFireModel::computeIndividualResolutionTime(const Incident& incident, double probability) {
    double resolutionTime = -1; // Placeholder for resolution time, should be calculated based on incident type and other factors

    double sample = dist_(rng_);
    if (sample > probability) {
        return resolutionTime;
    }

    IncidentLevel incidentLevel = incident.incident_level;
    switch (incidentLevel) {
        case IncidentLevel::Low:
            resolutionTime = 10 * constants::SECONDS_IN_MINUTE; // Example resolution time for low-level incidents
            break;
        case IncidentLevel::Moderate:
            resolutionTime = 30 * constants::SECONDS_IN_MINUTE; // Example resolution time for moderate-level incidents
            break;
        case IncidentLevel::High:
            resolutionTime = 60 * constants::SECONDS_IN_MINUTE; // Example resolution time for high-level incidents
            break;
        case IncidentLevel::Critical:
            resolutionTime = 90 * constants::SECONDS_IN_MINUTE; // Example resolution time for critical-level incidents
            break;
        default:
            spdlog::error("[EnvironmentModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }
    
    return resolutionTime;
}

// Example: compute a probability and sample true/false
bool HardCodedFireModel::shouldResolveIncident(double probability) {
    double sample = dist_(rng_);
    return sample < probability;
}

double HardCodedFireModel::computeIncidentResolutionProbability(const Incident& incident, time_t currentTime) {
    // Base probability by incident level
    double base_prob = 0.0;
    switch (incident.incident_level) {
        case IncidentLevel::Low:      base_prob = 0.7; break;
        case IncidentLevel::Moderate: base_prob = 0.5; break;
        case IncidentLevel::High:     base_prob = 0.3; break;
        case IncidentLevel::Critical: base_prob = 0.1; break;
        default:                      base_prob = 0.1; break;
    }

    // Apparatus ratio: more apparatus means higher probability (cap at 1.0)
    double apparatus_ratio = 0.0;
    if (incident.totalApparatusRequired > 0) {
        apparatus_ratio = static_cast<double>(incident.currentApparatusCount) / incident.totalApparatusRequired;
        if (apparatus_ratio > 1.0) apparatus_ratio = 1.0;
    }

    // Time since responded to (in minutes)
    double minutes_since_responded = 0.0;
    if (incident.timeRespondedTo > 0) {
        minutes_since_responded = static_cast<double>(currentTime - incident.timeRespondedTo) / constants::SECONDS_IN_MINUTE;
        if (minutes_since_responded < 0) minutes_since_responded = 0.0;
    }

    // Combine: weighted sum (tune weights as needed)
    double probability = base_prob;
    probability += 0.2 * apparatus_ratio;           // Apparatus helps
    probability += 0.01 * minutes_since_responded;  // More time helps

    // Clamp to [0, 1]
    if (probability > 1.0) probability = 1.0;
    if (probability < 0.0) probability = 0.0;

    spdlog::debug("[HardCodedFireModel] Prob: base={}, apparatus_ratio={}, minutes_since_responded={}, final={}",
                  base_prob, apparatus_ratio, minutes_since_responded, probability);

    return probability;
}