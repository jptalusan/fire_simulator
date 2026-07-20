#include "models/fire_model.h"
#include <random>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "utils/logger.h"
#include "enums.h"
#include "utils/constants.h"
#include "utils/error.h"

HistoricalFireModel::HistoricalFireModel(unsigned int seed, const std::string& csv_path, const std::string& resolution_stats_path)
    : rng_(seed), dist_(0.0, 1.0) {
    loadApparatusRequirements(csv_path);
    loadResolutionStats(resolution_stats_path);
}

void HistoricalFireModel::loadApparatusRequirements(const std::string& csv_path) {
    std::ifstream file(csv_path);
    
    LOG_INFO("[HistoricalFireModel] Loading apparatus requirements from: {}", csv_path);
    std::string line;
    bool first = true;
    while (std::getline(file, line)) {
        if (first) { first = false; continue; } // skip header
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        tokens[14] = tokens[14].substr(0, tokens[14].find_last_not_of(" \n\r\t")+1); // Trim whitespace/newline from last token
        if (tokens.size() < 3) continue; // skip incomplete lines

        IncidentCategory cat = stringToIncidentCategory(tokens[0]);
        std::unordered_map<ApparatusType, int> reqs;
        // Map columns to apparatus types (adjust indices as needed)
        if (tokens.size() > 3 && !tokens[3].empty()) reqs[ApparatusType::Engine] = std::stoi(tokens[3]);
        if (tokens.size() > 4 && !tokens[4].empty()) reqs[ApparatusType::Truck] = std::stoi(tokens[4]);
        if (tokens.size() > 5 && !tokens[5].empty()) reqs[ApparatusType::Rescue] = std::stoi(tokens[5]);
        if (tokens.size() > 6 && !tokens[6].empty()) reqs[ApparatusType::Hazard] = std::stoi(tokens[6]);
        if (tokens.size() > 7 && !tokens[7].empty()) reqs[ApparatusType::Squad] = std::stoi(tokens[7]);
        if (tokens.size() > 8 && !tokens[8].empty()) reqs[ApparatusType::Fast] = std::stoi(tokens[8]);
        if (tokens.size() > 9 && !tokens[9].empty()) reqs[ApparatusType::Medic] = std::stoi(tokens[9]);
        if (tokens.size() > 10 && !tokens[10].empty()) reqs[ApparatusType::Brush] = std::stoi(tokens[10]);
        if (tokens.size() > 11 && !tokens[11].empty()) reqs[ApparatusType::Boat] = std::stoi(tokens[11]);
        if (tokens.size() > 12 && !tokens[12].empty()) reqs[ApparatusType::UTV] = std::stoi(tokens[12]);
        if (tokens.size() > 13 && !tokens[13].empty()) reqs[ApparatusType::Reach] = std::stoi(tokens[13]);
        if (tokens.size() > 14 && !tokens[14].empty()) reqs[ApparatusType::SuppressionChief] = std::stoi(tokens[14]);
        if (tokens.size() > 15 && !tokens[15].empty()) reqs[ApparatusType::EMSChief] = std::stoi(tokens[15]);

        if (reqs.size() == 0) {
            LOG_WARN("[HistoricalFireModel] No apparatus requirements found for category: {}", tokens[0]);
            throw IncidentRequirementsError("No apparatus requirements found for category: " + tokens[0]);
        }
        apparatus_requirements_[cat] = reqs;
        
    }
}

// Add a loader function (call from constructor)
void HistoricalFireModel::loadResolutionStats(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        LOG_ERROR("[HistoricalFireModel] Failed to open resolution stats file: {}", csv_path);
        return;
    }
    std::string line;
    bool first = true;
    while (std::getline(file, line)) {
        //skip header
        if (first) { first = false; continue; }
        std::stringstream ss(line);
        std::string token;
        IncidentCategory category;
        ResolutionStats stats;
        // header=Enum,count,mean,std,min,25%,50%,75%,max
        std::getline(ss, token, ',');
        category = stringToIncidentCategory(token);
        std::getline(ss, token, ','); // count
        stats.count = std::stoi(token);

        std::getline(ss, token, ',');
        stats.mean = std::stod(token);
        std::getline(ss, token, ',');
        //check if token is empty or not a number
        if (token.empty() || !std::isdigit(token[0])) {
            LOG_ERROR("[DepartmentFireModel] Invalid stddev value: {}", token);
            stats.variance = 0.0; // default to 0 if invalid
        } else {
            stats.variance = std::stod(token) * std::stod(token); // variance is stddev squared
        }

        std::getline(ss, token, ','); // min, not used
        std::getline(ss, token, ','); // 25%, not used
        std::getline(ss, token, ','); // 50%, not used
        std::getline(ss, token, ','); // 75%, not used
        std::getline(ss, token, ','); // max, not used
        if (category == IncidentCategory::Invalid) {
            LOG_ERROR("[DepartmentFireModel] Invalid category in resolution stats: {}", token);
            continue; // skip invalid categories
        }

        resolution_stats_[category] = stats;
    }
}

double HistoricalFireModel::computeResolutionTime(State& state, const Incident& incident) {
    state.getSystemTime();
    auto it = resolution_stats_.find(incident.category);
    if (it != resolution_stats_.end()) {
        std::normal_distribution<double> normal_dist(it->second.mean, std::sqrt(it->second.variance));
        double sampled_time = normal_dist(rng_);
        return std::max(sampled_time, 1.0);
    }
    
    // Fallback: compute weighted mean and variance
    if (!resolution_stats_.empty()) {
        double weighted_mean_sum = 0.0;
        double weighted_variance_sum = 0.0;
        int total_count = 0;
        
        // Calculate weighted sums
        for (const auto& pair : resolution_stats_) {
            weighted_mean_sum += pair.second.mean * pair.second.count;
            weighted_variance_sum += pair.second.variance * pair.second.count;
            total_count += pair.second.count;
        }
        
        double weighted_mean = weighted_mean_sum / total_count;
        double weighted_variance = weighted_variance_sum / total_count;
        
        std::normal_distribution<double> fallback_dist(weighted_mean, std::sqrt(weighted_variance));
        double sampled_time = fallback_dist(rng_);

        return std::max(sampled_time, 1.0);
    }
    
    // Last resort fallback
    LOG_ERROR("[DepartmentFireModel] No resolution stats loaded, using hardcoded fallback");
    return 30 * constants::SECONDS_IN_MINUTE;
}

bool HistoricalFireModel::shouldResolveIncident(double probability) {
    double sample = dist_(rng_);
    sample = std::clamp(sample, 0.1, 1.0);
    return sample < probability;
}

std::unordered_map<ApparatusType, int> HistoricalFireModel::calculateApparatusCount(const Incident& incident) {
    auto it = apparatus_requirements_.find(incident.category);
    if (it != apparatus_requirements_.end()) {
        return it->second;
    }
    throw UnknownValueError();
}