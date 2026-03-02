#include "models/ems_service_model.h"
#include "objects/incident.h"
#include "simulator/state.h"
#include "models/travel_time_model.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

HistoricalEMSServiceModel::HistoricalEMSServiceModel(unsigned int seed)
    : rng_(seed), uniformDist_(0.0, 1.0) {
}

bool HistoricalEMSServiceModel::loadSceneTimeStats(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open EMS scene time stats file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header
    std::getline(file, line);

    int loadedCount = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string category, meanStr, varianceStr, stdStr, minStr, countStr;

        std::getline(ss, category, ',');
        std::getline(ss, meanStr, ',');
        std::getline(ss, varianceStr, ',');
        std::getline(ss, stdStr, ',');
        std::getline(ss, minStr, ',');
        std::getline(ss, countStr, ',');

        EMSSceneTimeStats stats;
        try {
            stats.mean = std::stod(meanStr);
            stats.variance = std::stod(varianceStr);
            stats.std = std::stod(stdStr);
            stats.min = std::stod(minStr);
            stats.count = std::stoi(countStr);
            sceneTimeStats_[category] = stats;
            loadedCount++;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing scene time stats for category {}: {}", category, e.what());
        }
    }

    LOG_INFO("Loaded EMS scene time stats for {} categories from {}", loadedCount, csvPath);
    return loadedCount > 0;
}

bool HistoricalEMSServiceModel::loadTransportStats(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open EMS transport stats file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header
    std::getline(file, line);

    int loadedCount = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string category, probStr, countStr;

        std::getline(ss, category, ',');
        std::getline(ss, probStr, ',');
        std::getline(ss, countStr, ',');

        EMSTransportStats stats;
        try {
            stats.transportProbability = std::stod(probStr);
            stats.count = std::stoi(countStr);
            transportStats_[category] = stats;
            loadedCount++;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing transport stats for category {}: {}", category, e.what());
        }
    }

    LOG_INFO("Loaded EMS transport stats for {} categories from {}", loadedCount, csvPath);
    return loadedCount > 0;
}

bool HistoricalEMSServiceModel::loadHospitalTimeStats(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open hospital time stats file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header
    std::getline(file, line);

    // Read first data line (overall stats)
    if (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string category, meanStr, varianceStr, stdStr, countStr;

        std::getline(ss, category, ',');
        std::getline(ss, meanStr, ',');
        std::getline(ss, varianceStr, ',');
        std::getline(ss, stdStr, ',');
        std::getline(ss, countStr, ',');

        try {
            hospitalTimeStats_.mean = std::stod(meanStr);
            hospitalTimeStats_.variance = std::stod(varianceStr);
            hospitalTimeStats_.std = std::stod(stdStr);
            hospitalTimeStats_.count = std::stoi(countStr);
            LOG_INFO("Loaded hospital time stats: mean={:.1f}s, std={:.1f}s from {}",
                     hospitalTimeStats_.mean, hospitalTimeStats_.std, csvPath);
            return true;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing hospital time stats: {}", e.what());
        }
    }

    return false;
}

std::string HistoricalEMSServiceModel::getCategoryKey(const Incident& incident) const {
    // Use the original incident type string from CSV (matches EMS stats keys)
    if (!incident.incident_type_str.empty()) {
        return incident.incident_type_str;
    }
    return to_string(incident.incident_type);
}

double HistoricalEMSServiceModel::sampleNormal(double mean, double std, double minVal) {
    std::normal_distribution<double> normalDist(mean, std);
    double sampled = normalDist(rng_);
    // Clamp to per-category minimum (defaults to 60s if not specified)
    return std::max(sampled, minVal);
}

double HistoricalEMSServiceModel::computeEMSSceneTime(const Incident& incident) {
    std::string category = getCategoryKey(incident);

    // Check if we have stats for this category
    auto it = sceneTimeStats_.find(category);
    if (it != sceneTimeStats_.end() && it->second.count > 0) {
        return sampleNormal(it->second.mean, it->second.std, it->second.min);
    }

    // Try "Medical" as fallback (most common EMS incident type)
    it = sceneTimeStats_.find("Medical");
    if (it != sceneTimeStats_.end() && it->second.count > 0) {
        return sampleNormal(it->second.mean, it->second.std, it->second.min);
    }

    // Use overall weighted average if no category match
    if (!sceneTimeStats_.empty()) {
        double totalWeight = 0.0;
        double weightedSum = 0.0;
        for (const auto& [cat, stats] : sceneTimeStats_) {
            totalWeight += stats.count;
            weightedSum += stats.mean * stats.count;
        }
        if (totalWeight > 0) {
            double overallMean = weightedSum / totalWeight;
            return sampleNormal(overallMean, 380.0);  // Use default std
        }
    }

    // Fallback to hardcoded default (should rarely happen if files are loaded)
    LOG_DEBUG("Using fallback EMS scene time for category: {}", category);
    return sampleNormal(624.8, 380.2);
}

bool HistoricalEMSServiceModel::requiresHospitalTransport(const Incident& incident) {
    std::string category = getCategoryKey(incident);

    // Check if we have stats for this category
    auto it = transportStats_.find(category);
    if (it != transportStats_.end() && it->second.count > 0) {
        return uniformDist_(rng_) < it->second.transportProbability;
    }

    // Try "Medical" as fallback (most common EMS incident type)
    it = transportStats_.find("Medical");
    if (it != transportStats_.end() && it->second.count > 0) {
        return uniformDist_(rng_) < it->second.transportProbability;
    }

    // Use overall weighted average if no category match
    if (!transportStats_.empty()) {
        double totalWeight = 0.0;
        double weightedSum = 0.0;
        for (const auto& [cat, stats] : transportStats_) {
            totalWeight += stats.count;
            weightedSum += stats.transportProbability * stats.count;
        }
        if (totalWeight > 0) {
            double overallProb = weightedSum / totalWeight;
            return uniformDist_(rng_) < overallProb;
        }
    }

    // Fallback to hardcoded default
    LOG_DEBUG("Using fallback transport probability for category: {}", category);
    return uniformDist_(rng_) < 0.4562;
}

double HistoricalEMSServiceModel::computeHospitalTime() {
    if (hospitalTimeStats_.count > 0) {
        return sampleNormal(hospitalTimeStats_.mean, hospitalTimeStats_.std);
    }

    // Fallback to hardcoded default
    LOG_DEBUG("Using fallback hospital time stats");
    return sampleNormal(1270.7, 601.5);
}

double HistoricalEMSServiceModel::computeHospitalTime(const std::string& hospitalName) {
    auto it = perHospitalTimeStats_.find(hospitalName);
    if (it != perHospitalTimeStats_.end() && it->second.count > 0) {
        LOG_DEBUG("Using per-hospital turnaround for {}: mean={:.1f}s, std={:.1f}s",
                  hospitalName, it->second.mean, it->second.std);
        return sampleNormal(it->second.mean, it->second.std);
    }
    // Fallback to overall hospital time
    return computeHospitalTime();
}

double HistoricalEMSServiceModel::computeCoupledSceneTime(const Incident& incident, time_t /*medicArrivalTime*/) {
    if (!couplingParams_.loaded) {
        // Fallback to independent per-category scene time
        return computeEMSSceneTime(incident);
    }

    // Fire resolution = total fire on-scene duration
    double fireResolutionSec = difftime(incident.resolvedTime, incident.timeRespondedTo);

    // Coupled model: max(baseline, min(fire_resolution, ceiling)) + noise
    double coupled = std::max(couplingParams_.baseline_sec,
                              std::min(fireResolutionSec, couplingParams_.ceiling_sec));

    std::normal_distribution<double> noiseDist(couplingParams_.residual_mean_sec,
                                                couplingParams_.residual_std_sec);
    double sceneTime = coupled + noiseDist(rng_);

    // Minimum 60 seconds
    sceneTime = std::max(sceneTime, 60.0);

    LOG_DEBUG("Coupled scene time: fire_res={:.0f}s, coupled={:.0f}s, scene_time={:.0f}s",
              fireResolutionSec, coupled, sceneTime);
    return sceneTime;
}

int HistoricalEMSServiceModel::determineTransportCount(int medicCount, const Incident& incident) {
    if (medicCount <= 0) return 0;

    if (medicCount == 1) {
        // Single medic: use per-category transport probability
        return requiresHospitalTransport(incident) ? 1 : 0;
    }

    // Multi-medic: use multi-medic transport distribution if loaded
    if (!multiMedicDist_.empty()) {
        // Find probabilities for this medic count
        std::vector<std::pair<int, double>> probs; // (transport_count, probability)
        for (const auto& entry : multiMedicDist_) {
            if (entry.medic_count == medicCount) {
                probs.push_back({entry.transport_count, entry.probability});
            }
        }

        // If no exact match, use the highest available medic count
        if (probs.empty()) {
            int maxMedicCount = 0;
            for (const auto& entry : multiMedicDist_) {
                maxMedicCount = std::max(maxMedicCount, entry.medic_count);
            }
            for (const auto& entry : multiMedicDist_) {
                if (entry.medic_count == maxMedicCount) {
                    probs.push_back({entry.transport_count, entry.probability});
                }
            }
        }

        if (!probs.empty()) {
            // Sample from the distribution
            double randomVal = uniformDist_(rng_);
            double cumProb = 0.0;
            int sampledTransportCount = 0;

            for (const auto& [tc, p] : probs) {
                cumProb += p;
                if (randomVal < cumProb) {
                    sampledTransportCount = tc;
                    break;
                }
            }

            // Cap at 1 (simplified model: at most 1 medic transports)
            int transportCount = std::min(sampledTransportCount, 1);
            LOG_DEBUG("Multi-medic transport decision: {} medics, sampled={}, capped={}",
                      medicCount, sampledTransportCount, transportCount);
            return transportCount;
        }
    }

    // Fallback: use per-category transport probability for a single transport decision
    return requiresHospitalTransport(incident) ? 1 : 0;
}

bool HistoricalEMSServiceModel::loadZoneHospitalProbs(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open zone-hospital probabilities file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header: FireBeat,Destination,count,zone_total,probability
    std::getline(file, line);

    int loadedCount = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string zone, hospital, countStr, totalStr, probStr;

        std::getline(ss, zone, ',');
        std::getline(ss, hospital, ',');
        std::getline(ss, countStr, ',');
        std::getline(ss, totalStr, ',');
        std::getline(ss, probStr, ',');

        try {
            ZoneHospitalProb prob;
            prob.hospitalName = hospital;
            prob.probability = std::stod(probStr);
            prob.count = std::stoi(countStr);
            zoneHospitalProbs_[zone].push_back(prob);
            loadedCount++;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing zone-hospital prob for zone {}: {}", zone, e.what());
        }
    }

    LOG_INFO("Loaded {} zone-hospital probability mappings from {}", loadedCount, csvPath);
    return loadedCount > 0;
}

bool HistoricalEMSServiceModel::loadSceneTimeCouplingParams(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open scene time coupling params file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header: parameter,value
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string param, valueStr;

        std::getline(ss, param, ',');
        std::getline(ss, valueStr, ',');

        try {
            double value = std::stod(valueStr);
            if (param == "baseline_sec") couplingParams_.baseline_sec = value;
            else if (param == "ceiling_sec") couplingParams_.ceiling_sec = value;
            else if (param == "residual_mean_sec") couplingParams_.residual_mean_sec = value;
            else if (param == "residual_std_sec") couplingParams_.residual_std_sec = value;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing coupling param {}: {}", param, e.what());
        }
    }

    couplingParams_.loaded = true;
    LOG_INFO("Loaded scene time coupling params: baseline={:.0f}s, ceiling={:.0f}s, residual_mean={:.0f}s, residual_std={:.0f}s from {}",
             couplingParams_.baseline_sec, couplingParams_.ceiling_sec,
             couplingParams_.residual_mean_sec, couplingParams_.residual_std_sec, csvPath);
    return true;
}

bool HistoricalEMSServiceModel::loadHospitalTimeByDest(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open hospital time by dest file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header: Hospital,mean,variance,std,count
    std::getline(file, line);

    int loadedCount = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string hospital, meanStr, varianceStr, stdStr, countStr;

        std::getline(ss, hospital, ',');
        std::getline(ss, meanStr, ',');
        std::getline(ss, varianceStr, ',');
        std::getline(ss, stdStr, ',');
        std::getline(ss, countStr, ',');

        HospitalTimeStats stats;
        try {
            stats.mean = std::stod(meanStr);
            stats.variance = std::stod(varianceStr);
            stats.std = std::stod(stdStr);
            stats.count = std::stoi(countStr);
            perHospitalTimeStats_[hospital] = stats;
            loadedCount++;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing hospital time stats for {}: {}", hospital, e.what());
        }
    }

    LOG_INFO("Loaded per-hospital turnaround stats for {} hospitals from {}", loadedCount, csvPath);
    return loadedCount > 0;
}

bool HistoricalEMSServiceModel::loadMultiMedicTransportDist(const std::string& csvPath) {
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        LOG_WARN("Could not open multi-medic transport dist file: {}", csvPath);
        return false;
    }

    std::string line;
    // Skip header: medic_count,transport_count,probability,sample_count
    std::getline(file, line);

    int loadedCount = 0;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string mcStr, tcStr, probStr, sampleStr;

        std::getline(ss, mcStr, ',');
        std::getline(ss, tcStr, ',');
        std::getline(ss, probStr, ',');
        std::getline(ss, sampleStr, ',');

        try {
            MultiMedicTransportProb entry;
            entry.medic_count = std::stoi(mcStr);
            entry.transport_count = std::stoi(tcStr);
            entry.probability = std::stod(probStr);
            multiMedicDist_.push_back(entry);
            loadedCount++;
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing multi-medic transport dist: {}", e.what());
        }
    }

    LOG_INFO("Loaded {} multi-medic transport distribution entries from {}", loadedCount, csvPath);
    return loadedCount > 0;
}

void HistoricalEMSServiceModel::buildHospitalNameMapping(const State& state) {
    hospitalNameToIndex_.clear();
    const auto& hospitals = state.getHospitals();
    for (size_t i = 0; i < hospitals.size(); ++i) {
        hospitalNameToIndex_[hospitals[i].getName()] = static_cast<int>(i);
    }
    LOG_DEBUG("Built hospital name mapping for {} hospitals", hospitalNameToIndex_.size());
}

int HistoricalEMSServiceModel::selectHospital(const Incident& incident, const State& state,
                                     TravelTimeModel& travelTimeModel) {
    if (!state.hasHospitals()) {
        LOG_WARN("No hospitals available for selection");
        return -1;
    }

    // Build name mapping if not done
    if (hospitalNameToIndex_.empty()) {
        buildHospitalNameMapping(state);
    }

    // Get the zone (FireBeat) for this incident
    // zoneIndex is an int, but zone data uses string keys like "1", "10", "10A"
    std::string zoneKey = std::to_string(incident.zoneIndex);

    // Check if we have zone-specific hospital probabilities
    auto zoneIt = zoneHospitalProbs_.find(zoneKey);
    if (zoneIt != zoneHospitalProbs_.end() && !zoneIt->second.empty()) {
        // Use weighted random selection based on historical probabilities
        double randomVal = uniformDist_(rng_);
        double cumProb = 0.0;

        for (const auto& prob : zoneIt->second) {
            cumProb += prob.probability;
            if (randomVal < cumProb) {
                // Found the hospital - look up its index
                auto nameIt = hospitalNameToIndex_.find(prob.hospitalName);
                if (nameIt != hospitalNameToIndex_.end()) {
                    LOG_DEBUG("Selected hospital {} for zone {} (historical prob: {:.2f})",
                              prob.hospitalName, zoneKey, prob.probability);
                    return nameIt->second;
                }
            }
        }
    }

    // Fallback: Find nearest hospital by OSRM travel time
    LOG_DEBUG("Using OSRM travel time to find nearest hospital for zone {}", zoneKey);

    const auto& hospitals = state.getHospitals();
    int nearestIndex = -1;
    double minTravelTime = std::numeric_limits<double>::max();

    for (size_t i = 0; i < hospitals.size(); ++i) {
        try {
            auto [travelTime, route] = travelTimeModel.getTravelTimeAndRoute(
                incident.getLocation(), hospitals[i].getLocation()
            );
            if (travelTime < minTravelTime) {
                minTravelTime = travelTime;
                nearestIndex = static_cast<int>(i);
            }
        } catch (const std::exception& e) {
            LOG_WARN("Error getting travel time to hospital {}: {}", hospitals[i].getName(), e.what());
        }
    }

    if (nearestIndex >= 0) {
        LOG_DEBUG("Selected nearest hospital {} (travel time: {:.0f}s)",
                  hospitals[nearestIndex].getName(), minTravelTime);
    } else {
        // Ultimate fallback: first hospital
        nearestIndex = 0;
        LOG_WARN("Could not determine nearest hospital, using first hospital");
    }

    return nearestIndex;
}
