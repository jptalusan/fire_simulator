#include "models/fire.h"
#include "simulator/state.h"
#include "utils/constants.h"
#include "utils/error.h"
#include <random>
#include "utils/logger.h"
#include "enums.h"

// TODO: All placeholders, not working yet.
HardCodedFireModel::HardCodedFireModel(unsigned int seed)
    : rng_(seed), dist_(0.0, 1.0) {}

// Example implementation for HardCodedFireModel
double HardCodedFireModel::computeResolutionTime(State& state, const Incident& incident) {
    IncidentLevel incidentLevel = incident.incident_level;
    state.getSystemTime();

    double estimatedResolutionTime = 0.0;
    switch (incidentLevel) {
        case IncidentLevel::Low:        estimatedResolutionTime = 10 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Moderate:   estimatedResolutionTime = 30 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::High:       estimatedResolutionTime = 60 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Critical:   estimatedResolutionTime = 90 * constants::SECONDS_IN_MINUTE; break;
        default:
            LOG_ERROR("[HardCodedFireModel] Unknown incident level: {}", to_string(incidentLevel));
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
            LOG_ERROR("[EnvironmentModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError(); // Throw an error for unknown incident levels
    }
    return apparatusCount; // Return the calculated apparatus count map
}


//TODO: Delete this function and put in loader.h
IncidentCategory stringToIncidentCategory(const std::string& str) {
    if (str == "One") return IncidentCategory::One;
    if (str == "OneB") return IncidentCategory::OneB;
    if (str == "OneBM") return IncidentCategory::OneBM;
    if (str == "OneC") return IncidentCategory::OneC;
    if (str == "OneD") return IncidentCategory::OneD;
    if (str == "OneE") return IncidentCategory::OneE;
    if (str == "OneEM") return IncidentCategory::OneEM;
    if (str == "OneF") return IncidentCategory::OneF;
    if (str == "OneG") return IncidentCategory::OneG;
    if (str == "OneH") return IncidentCategory::OneH;
    if (str == "OneJ") return IncidentCategory::OneJ;
    if (str == "Two") return IncidentCategory::Two;
    if (str == "TwoM") return IncidentCategory::TwoM;
    if (str == "TwoMF") return IncidentCategory::TwoMF;
    if (str == "TwoA") return IncidentCategory::TwoA;
    if (str == "TwoB") return IncidentCategory::TwoB;
    if (str == "TwoC") return IncidentCategory::TwoC;
    if (str == "Three") return IncidentCategory::Three;
    if (str == "ThreeF") return IncidentCategory::ThreeF;
    if (str == "ThreeM") return IncidentCategory::ThreeM;
    if (str == "ThreeA") return IncidentCategory::ThreeA;
    if (str == "ThreeB") return IncidentCategory::ThreeB;
    if (str == "ThreeC") return IncidentCategory::ThreeC;
    if (str == "ThreeCM") return IncidentCategory::ThreeCM;
    if (str == "ThreeD") return IncidentCategory::ThreeD;
    if (str == "Four") return IncidentCategory::Four;
    if (str == "FourM") return IncidentCategory::FourM;
    if (str == "FourA") return IncidentCategory::FourA;
    if (str == "FourB") return IncidentCategory::FourB;
    if (str == "FourC") return IncidentCategory::FourC;
    if (str == "Five") return IncidentCategory::Five;
    if (str == "FiveA") return IncidentCategory::FiveA;
    if (str == "Six") return IncidentCategory::Six;
    if (str == "Seven") return IncidentCategory::Seven;
    if (str == "SevenB") return IncidentCategory::SevenB;
    if (str == "SevenBM") return IncidentCategory::SevenBM;
    if (str == "Eight") return IncidentCategory::Eight;
    if (str == "EightA") return IncidentCategory::EightA;
    if (str == "EightB") return IncidentCategory::EightB;
    if (str == "EightC") return IncidentCategory::EightC;
    if (str == "EightD") return IncidentCategory::EightD;
    if (str == "EightE") return IncidentCategory::EightE;
    if (str == "EightF") return IncidentCategory::EightF;
    if (str == "EightG") return IncidentCategory::EightG;
    if (str == "Nine") return IncidentCategory::Nine;
    if (str == "Ten") return IncidentCategory::Ten;
    if (str == "Eleven") return IncidentCategory::Eleven;
    if (str == "ElevenA") return IncidentCategory::ElevenA;
    if (str == "ElevenB") return IncidentCategory::ElevenB;
    if (str == "Thirteen") return IncidentCategory::Thirteen;
    if (str == "Fourteen") return IncidentCategory::Fourteen;
    if (str == "Fifteen") return IncidentCategory::Fifteen;
    if (str == "Sixteen") return IncidentCategory::Sixteen;
    if (str == "Eighteen") return IncidentCategory::Eighteen;
    return IncidentCategory::Invalid;
}

DepartmentFireModel::DepartmentFireModel(unsigned int seed, const std::string& csv_path, const std::string& resolution_stats_path)
    : rng_(seed), dist_(0.0, 1.0) {
    loadApparatusRequirements(csv_path);
    loadResolutionStats(resolution_stats_path);
}

void DepartmentFireModel::loadApparatusRequirements(const std::string& csv_path) {
    std::ifstream file(csv_path);
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
        if (tokens.size() > 14 && !tokens[14].empty()) reqs[ApparatusType::Chief] = std::stoi(tokens[14]);
        // if (reqs.empty()) {
        //     LOG_ERROR("No apparatus requirements found for category: {}", to_string(cat));

        // }

        apparatus_requirements_[cat] = reqs;
        
    }
}






// Add a loader function (call from constructor)
void DepartmentFireModel::loadResolutionStats(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        LOG_ERROR("[DepartmentFireModel] Failed to open resolution stats file: {}", csv_path);
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
        std::getline(ss, token, ','); // count, not used
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


// Add new statistical resolution time function
double DepartmentFireModel::computeResolutionTime(State& state, const Incident& incident) {
    state.getSystemTime();
    auto it = resolution_stats_.find(incident.category);
    if (it != resolution_stats_.end()) {
        std::normal_distribution<double> normal_dist(it->second.mean, std::sqrt(it->second.variance));
        double sampled_time = normal_dist(rng_);
        // Clamp to minimum reasonable time
        return std::max(sampled_time, 1.0);
    }
    // Fallback to incident level if category not found
    IncidentLevel incidentLevel = incident.incident_level;
    double estimatedResolutionTime = 0.0;
    switch (incidentLevel) {
        case IncidentLevel::Low:        estimatedResolutionTime = 15 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Moderate:   estimatedResolutionTime = 35 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::High:       estimatedResolutionTime = 70 * constants::SECONDS_IN_MINUTE; break;
        case IncidentLevel::Critical:   estimatedResolutionTime = 120 * constants::SECONDS_IN_MINUTE; break;
        default:
            LOG_ERROR("[DepartmentFireModel] Unknown incident level: {}", to_string(incidentLevel));
            throw UnknownValueError();
    }
    return estimatedResolutionTime;
}

bool DepartmentFireModel::shouldResolveIncident(double probability) {
    double sample = dist_(rng_);
    sample = std::clamp(sample, 0.1, 1.0);
    return sample < probability;
}

std::unordered_map<ApparatusType, int> DepartmentFireModel::calculateApparatusCount(const Incident& incident) {
    auto it = apparatus_requirements_.find(incident.category);
    if (it != apparatus_requirements_.end()) {
        return it->second;
    }
    throw UnknownValueError();
}
    


