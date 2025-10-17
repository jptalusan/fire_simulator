#include "models/fire_model.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/logger.h"
#include "enums.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/logger.h"

MLFireModel::MLFireModel([[maybe_unused]] unsigned int seed, const std::string& model_path, const std::string& config_path, const std::string& apparatus_csv_path){
    // Initialize ONNX predictor
    onnx_predictor_ = std::make_unique<ONNXPredictor>();
    
    // Load components in order
    loadFeatureConfig(config_path);  // Load first to get feature info
    loadONNXModel(model_path);       // Then load model
    loadApparatusRequirements(apparatus_csv_path);  // Load apparatus requirements
    
    LOG_INFO("[MLFireModel] Initialized with {} expected input features", expected_input_features_);
}

void MLFireModel::loadApparatusRequirements(const std::string& csv_path) {
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

        apparatus_requirements_[cat] = reqs;
        
    }
}

std::unordered_map<ApparatusType, int> MLFireModel::calculateApparatusCount(const Incident& incident) {
    auto it = apparatus_requirements_.find(incident.category);
    if (it != apparatus_requirements_.end()) {
        return it->second;
    }
    throw UnknownValueError();
}

double MLFireModel::computeResolutionTime(State& state, const Incident& incident) {
    try {
        // Extract all features for the incident
        std::vector<float> features = extractFeatures(state, incident);
        
        // Validate feature count
        if (features.size() != static_cast<size_t>(expected_input_features_)) {
            LOG_ERROR("[MLFireModel] Feature size mismatch. Expected: {}, Got: {}", 
                     expected_input_features_, features.size());
            return 45 * constants::SECONDS_IN_MINUTE; // Fallback
        }
        
        // Run ONNX inference
        float predicted_time = onnx_predictor_->predict(features);
        
        if (predicted_time < 0) {
            LOG_ERROR("[MLFireModel] ONNX prediction failed, using fallback");
            return 45 * constants::SECONDS_IN_MINUTE; // Fallback
        }
        
        // Ensure minimum response time (clamp to at least 1 minute)
        double response_time = std::max(static_cast<double>(predicted_time), 0.0);
        
        LOG_DEBUG("[MLFireModel] Predicted response time: {:.2f} seconds for incident category: {}", 
                 response_time, to_string(incident.category));
        
        return response_time;
        
    } catch (const std::exception& e) {
        LOG_ERROR("[MLFireModel] Error during prediction: {}", e.what());
        return 45 * constants::SECONDS_IN_MINUTE; // Safe fallback
    }
}

void MLFireModel::loadONNXModel(const std::string& model_path) {
    LOG_INFO("[MLFireModel] Loading ONNX model from: {}", model_path);
    
    if (!onnx_predictor_->loadModel(model_path)) {
        LOG_ERROR("[MLFireModel] Failed to load ONNX model: {}", model_path);
        throw std::runtime_error("Failed to load ONNX model");
    }
    
    LOG_INFO("[MLFireModel] Successfully loaded ONNX model");
}

void MLFireModel::loadFeatureConfig(const std::string& config_path) {
    LOG_INFO("[MLFireModel] Loading feature configuration from: {}", config_path);
    
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        LOG_ERROR("[MLFireModel] Cannot open feature config file: {}", config_path);
        throw std::runtime_error("Cannot open feature config file");
    }
    
    try {
        config_file >> feature_config_;
        
        // Extract model info
        if (feature_config_.contains("model_info")) {
            auto model_info = feature_config_["model_info"];
            expected_input_features_ = model_info.value("input_features", 0);
            model_type_ = model_info.value("model_type", "unknown");
        }
        
        // Extract numerical features and scaling parameters
        if (feature_config_.contains("numerical_features")) {
            auto numerical_config = feature_config_["numerical_features"];
            numerical_features_ = numerical_config.value("names", std::vector<std::string>());
            
            if (numerical_config.contains("scaler_params")) {
                auto scaler_params = numerical_config["scaler_params"];
                auto means = scaler_params.value("mean", std::vector<double>());
                auto scales = scaler_params.value("scale", std::vector<double>());
                
                for (size_t i = 0; i < numerical_features_.size() && i < means.size() && i < scales.size(); ++i) {
                    numerical_scaling_[numerical_features_[i]] = {means[i], scales[i]};
                }
            }
        }
        
        // Extract categorical features and encodings
        if (feature_config_.contains("categorical_features")) {
            auto categorical_config = feature_config_["categorical_features"];
            categorical_features_ = categorical_config.value("original_names", std::vector<std::string>());
            
            if (categorical_config.contains("categories")) {
                auto categories = categorical_config["categories"];
                for (auto& [feature_name, category_info] : categories.items()) {
                    std::map<std::string, int> feature_mapping;
                    
                if (category_info.contains("encoded_categories")) {
                    auto encoded_categories = category_info["encoded_categories"];
                    for (size_t i = 0; i < encoded_categories.size(); ++i) {
                        std::string category_value;
                        if (encoded_categories[i].is_string()) {
                            category_value = encoded_categories[i].get<std::string>();
                        } else {
                            // Convert numeric values to string with .0 format to match runtime usage
                            double val = encoded_categories[i].get<double>();
                            category_value = std::to_string(static_cast<int>(val)) + ".0";
                        }
                        
                        // Handle NaN special case
                        if (encoded_categories[i].is_null() || 
                            (encoded_categories[i].is_string() && encoded_categories[i] == "NaN")) {
                            category_value = "nan";
                        }
                        
                        feature_mapping[category_value] = static_cast<int>(i + 1); // +1 because first is dropped
                    }
                }
                
                // Add dropped category as 0 (all zeros in one-hot)
                if (category_info.contains("dropped_category")) {
                    std::string dropped_value;
                    auto dropped_category = category_info["dropped_category"];
                    
                    if (dropped_category.is_string()) {
                        dropped_value = dropped_category.get<std::string>();
                    } else if (dropped_category.is_number()) {
                        // Convert numeric values to string with .0 format to match runtime usage
                        double val = dropped_category.get<double>();
                        dropped_value = std::to_string(static_cast<int>(val)) + ".0";
                    } else if (dropped_category.is_null()) {
                        dropped_value = "nan";
                    }
                    
                    feature_mapping[dropped_value] = 0;
                }
                    
                    categorical_mappings_[feature_name] = feature_mapping;
                }
            }
        }
        
        // Extract the exact feature order from JSON - this is crucial!
        if (feature_config_.contains("feature_order")) {
            feature_order_ = feature_config_["feature_order"].get<std::vector<std::string>>();
            LOG_INFO("[MLFireModel] Loaded feature order with {} features", feature_order_.size());
        } else {
            LOG_WARN("[MLFireModel] No feature_order found in config, using default ordering");
        }
        
        LOG_INFO("[MLFireModel] Loaded feature config - {} numerical, {} categorical features", 
                numerical_features_.size(), categorical_features_.size());
        LOG_INFO("[MLFireModel] Expected input features: {}", expected_input_features_);
        
        // Log first few features in the order for debugging
        if (!feature_order_.empty()) {
            LOG_INFO("[MLFireModel] First 20 features in order:");
            for (size_t i = 0; i < std::min(static_cast<size_t>(20), feature_order_.size()); ++i) {
                LOG_INFO("  [{}] {}", i, feature_order_[i]);
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("[MLFireModel] Error parsing feature config: {}", e.what());
        throw std::runtime_error("Error parsing feature config");
    }
}

std::vector<float> MLFireModel:: extractFeatures([[maybe_unused]]const State& state, const Incident& incident) {
    std::vector<float> all_features;
    
    // Create a map of all possible features
    std::map<std::string, float> feature_map;
    
    // Extract all individual features and store them in the map
    
    // 1. Extract temporal features
    int hour = extractHour(incident.reportTime);
    int day_of_week = extractDayOfWeek(incident.reportTime);
    int month = extractMonth(incident.reportTime);
    int quarter = extractQuarter(incident.reportTime);
    int day_of_year = extractDayOfYear(incident.reportTime);
    bool is_weekend = (day_of_week == 5 || day_of_week == 6);
    bool is_night = (hour >= 22 || hour <= 6);
    bool is_rush_hour = ((hour >= 7 && hour <= 9) || (hour >= 16 && hour <= 18));
    bool is_business_hours = (hour >= 9 && hour <= 17 && day_of_week < 5);
    
    // Store numerical features
    feature_map["hour"] = scaleNumericalFeature("hour", static_cast<float>(hour));
    feature_map["day_of_week"] = scaleNumericalFeature("day_of_week", static_cast<float>(day_of_week));
    feature_map["month"] = scaleNumericalFeature("month", static_cast<float>(month));
    feature_map["quarter"] = scaleNumericalFeature("quarter", static_cast<float>(quarter));
    feature_map["day_of_year"] = scaleNumericalFeature("day_of_year", static_cast<float>(day_of_year));
    feature_map["is_weekend"] = scaleNumericalFeature("is_weekend", is_weekend ? 1.0f : 0.0f);
    feature_map["is_night"] = scaleNumericalFeature("is_night", is_night ? 1.0f : 0.0f);
    feature_map["is_rush_hour"] = scaleNumericalFeature("is_rush_hour", is_rush_hour ? 1.0f : 0.0f);
    feature_map["is_business_hours"] = scaleNumericalFeature("is_business_hours", is_business_hours ? 1.0f : 0.0f);
    
    // 2. Extract geographic features
    feature_map["lat"] = scaleNumericalFeature("lat", static_cast<float>(incident.lat));
    feature_map["lon"] = scaleNumericalFeature("lon", static_cast<float>(incident.lon));
    
    // ISSUE Don't hard code this.
    double nashville_center_lat = 36.1627;
    double nashville_center_lon = -86.7816;
    double distance = calculateDistanceFromCenter(incident.lat, incident.lon, nashville_center_lat, nashville_center_lon);
    feature_map["distance_from_center"] = scaleNumericalFeature("distance_from_center", static_cast<float>(distance));
    
    // 3. Extract categorical features and create one-hot encodings
    
    // Shift
    std::string shift;
    if (hour >= 6 && hour < 14) {
        shift = "Day";
    } else if (hour >= 14 && hour < 22) {
        shift = "Evening";  
    } else {
        shift = "Night";
    }
    auto shift_encoded = encodeCategoricalFeature("shift", shift);
    
    // Season
    std::string season;
    if (month == 12 || month == 1 || month == 2) {
        season = "Winter";
    } else if (month >= 3 && month <= 5) {
        season = "Spring";
    } else if (month >= 6 && month <= 8) {
        season = "Summer";
    } else {
        season = "Fall";
    }
    auto season_encoded = encodeCategoricalFeature("season", season);
    
    // Category
    std::string category_str = to_string(incident.category);
    auto category_encoded = encodeCategoricalFeature("category", category_str);
    
    // Zone ID
    std::string zone_id;
    if (incident.zoneIndex >= 0) {
        zone_id = std::to_string(incident.zoneIndex) + ".0"; // Use integer conversion, not float
    } else {
        zone_id = "nan";
    }
    auto zone_encoded = encodeCategoricalFeature("ZONE_ID", zone_id);
    
    // Incident type
    std::string incident_type = to_string(incident.incident_type);
    auto type_encoded = encodeCategoricalFeature("incident_type", incident_type);
    
    // Store categorical features in the map
    // For categorical features, we need to store each one-hot encoded feature separately
    
    // Shift features
    if (shift_encoded.size() >= 1) feature_map["shift_Evening"] = shift_encoded[0];
    if (shift_encoded.size() >= 2) feature_map["shift_Night"] = shift_encoded[1];
    
    // Season features  
    if (season_encoded.size() >= 1) feature_map["season_Spring"] = season_encoded[0];
    if (season_encoded.size() >= 2) feature_map["season_Summer"] = season_encoded[1];
    if (season_encoded.size() >= 3) feature_map["season_Winter"] = season_encoded[2];
    
    // Category features - map each encoded feature to its name
    auto category_mapping = categorical_mappings_.find("category");
    if (category_mapping != categorical_mappings_.end()) {
        size_t idx = 0;
        for (const auto& [cat_name, cat_idx] : category_mapping->second) {
            if (cat_idx > 0) { // Skip dropped category (index 0)
                std::string feature_name = "category_" + cat_name;
                if (idx < category_encoded.size()) {
                    feature_map[feature_name] = category_encoded[idx];
                    idx++;
                }
            }
        }
    }
    
    // Zone ID features
    auto zone_mapping = categorical_mappings_.find("ZONE_ID");
    if (zone_mapping != categorical_mappings_.end()) {
        size_t idx = 0;
        for (const auto& [zone_name, zone_idx] : zone_mapping->second) {
            if (zone_idx > 0) { // Skip dropped category
                std::string feature_name = "ZONE_ID_" + zone_name;
                if (idx < zone_encoded.size()) {
                    feature_map[feature_name] = zone_encoded[idx];
                    idx++;
                }
            }
        }
    }
    
    // Incident type features
    auto type_mapping = categorical_mappings_.find("incident_type");
    if (type_mapping != categorical_mappings_.end()) {
        size_t idx = 0;
        for (const auto& [type_name, type_idx] : type_mapping->second) {
            if (type_idx > 0) { // Skip dropped category
                std::string feature_name = "incident_type_" + type_name;
                if (idx < type_encoded.size()) {
                    feature_map[feature_name] = type_encoded[idx];
                    idx++;
                }
            }
        }
    }
    
    // 4. Use the exact feature order from JSON to build the final feature vector
    if (!feature_order_.empty()) {
        all_features.reserve(feature_order_.size());
        for (const std::string& feature_name : feature_order_) {
            auto it = feature_map.find(feature_name);
            if (it != feature_map.end()) {
                all_features.push_back(it->second);
            } else {
                LOG_WARN("[MLFireModel] Feature '{}' not found in feature map, using 0.0", feature_name);
                all_features.push_back(0.0f); // Default value for missing features
            }
        }
    } else {
        LOG_ERROR("[MLFireModel] No feature order available, falling back to manual ordering");
        // Fallback to the previous manual method if feature_order is not available
        auto temporal_features = extractTemporalFeatures(incident);
        auto geographic_features = extractGeographicFeatures(incident);
        auto categorical_features = extractCategoricalFeatures(incident);
        
        all_features.insert(all_features.end(), temporal_features.begin(), temporal_features.end());
        all_features.insert(all_features.end(), geographic_features.begin(), geographic_features.end());
        all_features.insert(all_features.end(), categorical_features.begin(), categorical_features.end());
    }
    
    LOG_DEBUG("[MLFireModel] Extracted {} total features (expected: {})", 
              all_features.size(), expected_input_features_);
    
    // Validate feature count
    if (static_cast<int>(all_features.size()) != expected_input_features_) {
        LOG_ERROR("[MLFireModel] Feature count mismatch! Got {}, expected {}", 
                  all_features.size(), expected_input_features_);
    }
    
    return all_features;
}

void MLFireModel::validateFeatureOrder() const {
    if (feature_order_.empty()) {
        LOG_ERROR("[MLFireModel] No feature order loaded from JSON!");
        return;
    }
    
    if (static_cast<int>(feature_order_.size()) != expected_input_features_) {
        LOG_ERROR("[MLFireModel] Feature order size ({}) doesn't match expected features ({})", 
                  feature_order_.size(), expected_input_features_);
        return;
    }
    
    // Verify that all numerical features come first
    size_t numerical_count = numerical_features_.size();
    bool found_categorical = false;
    
    for (size_t i = 0; i < feature_order_.size(); ++i) {
        const std::string& feature_name = feature_order_[i];
        
        // Check if this is a numerical feature
        bool is_numerical = std::find(numerical_features_.begin(), numerical_features_.end(), feature_name) != numerical_features_.end();
        
        if (is_numerical && found_categorical) {
            LOG_ERROR("[MLFireModel] Numerical feature '{}' found after categorical features at position {}", feature_name, i);
        }
        
        if (!is_numerical && i < numerical_count) {
            found_categorical = true;
        }
    }
    
    LOG_INFO("[MLFireModel] Feature order validation completed");
}

void MLFireModel::printFeatureOrder(size_t max_features) const {
    if (feature_order_.empty()) {
        LOG_INFO("[MLFireModel] No feature order available");
        return;
    }
    
    LOG_INFO("[MLFireModel] Feature Order (showing first {} of {} features):", 
             std::min(max_features, feature_order_.size()), feature_order_.size());
    
    for (size_t i = 0; i < std::min(max_features, feature_order_.size()); ++i) {
        // Determine if this is numerical or categorical
        std::string type = "categorical";
        if (std::find(numerical_features_.begin(), numerical_features_.end(), feature_order_[i]) != numerical_features_.end()) {
            type = "numerical";
        }
        
        LOG_INFO("  [{:3}] {} ({})", i, feature_order_[i], type);
    }
}

float MLFireModel::scaleNumericalFeature(const std::string& feature_name, float value) {
    auto it = numerical_scaling_.find(feature_name);
    if (it != numerical_scaling_.end()) {
        double mean = it->second.first;
        double scale = it->second.second;
        return static_cast<float>((value - mean) / scale);
    }
    
    LOG_WARN("[MLFireModel] No scaling parameters found for feature: {}", feature_name);
    return value; // Return unscaled if no parameters found
}

std::vector<float> MLFireModel::encodeCategoricalFeature(const std::string& feature_name, const std::string& value) {
    std::vector<float> encoded_features;
    
    auto feature_mapping = categorical_mappings_.find(feature_name);
    if (feature_mapping != categorical_mappings_.end()) {
        auto value_mapping = feature_mapping->second.find(value);
        
        if (value_mapping != feature_mapping->second.end()) {
            int category_index = value_mapping->second;
            
            // Create one-hot encoding (excluding the dropped category which is all zeros)
            int num_categories = feature_mapping->second.size() - 1; // -1 for dropped category
            
            for (int i = 1; i <= num_categories; ++i) { // Start from 1 to skip dropped
                encoded_features.push_back(category_index == i ? 1.0f : 0.0f);
            }
        } else {
            LOG_WARN("[MLFireModel] Unknown category '{}' for feature '{}', using all zeros", value, feature_name);
            // Handle unknown categories (all zeros for drop='first' encoding)
            int num_categories = feature_mapping->second.size() - 1;
            encoded_features.assign(num_categories, 0.0f);
        }
    } else {
        LOG_WARN("[MLFireModel] No encoding found for categorical feature: {}", feature_name);
        encoded_features.push_back(0.0f); // Default single feature
    }
    
    return encoded_features;
}

std::vector<float> MLFireModel::extractTemporalFeatures(const Incident& incident) {
    std::vector<float> features;
    
    // Extract temporal features and scale them
    int hour = extractHour(incident.reportTime);
    int day_of_week = extractDayOfWeek(incident.reportTime);
    int month = extractMonth(incident.reportTime);
    int quarter = extractQuarter(incident.reportTime);
    int day_of_year = extractDayOfYear(incident.reportTime);
    
    // Binary indicators
    bool is_weekend = (day_of_week == 5 || day_of_week == 6); // Saturday or Sunday
    bool is_night = (hour >= 22 || hour <= 6);
    bool is_rush_hour = ((hour >= 7 && hour <= 9) || (hour >= 16 && hour <= 18));
    bool is_business_hours = (hour >= 9 && hour <= 17 && day_of_week < 5);
    // bool is_holiday = isHoliday(incident.reportTime);
    
    // Add scaled numerical temporal features
    features.push_back(scaleNumericalFeature("hour", static_cast<float>(hour)));
    features.push_back(scaleNumericalFeature("day_of_week", static_cast<float>(day_of_week)));
    features.push_back(scaleNumericalFeature("month", static_cast<float>(month)));
    features.push_back(scaleNumericalFeature("quarter", static_cast<float>(quarter)));
    features.push_back(scaleNumericalFeature("day_of_year", static_cast<float>(day_of_year)));
    features.push_back(scaleNumericalFeature("is_weekend", is_weekend ? 1.0f : 0.0f));
    features.push_back(scaleNumericalFeature("is_night", is_night ? 1.0f : 0.0f));
    features.push_back(scaleNumericalFeature("is_rush_hour", is_rush_hour ? 1.0f : 0.0f));
    features.push_back(scaleNumericalFeature("is_business_hours", is_business_hours ? 1.0f : 0.0f));
    // features.push_back(scaleNumericalFeature("is_holiday", is_holiday ? 1.0f : 0.0f));

    return features;
}

std::vector<float> MLFireModel::extractGeographicFeatures(const Incident& incident) {
    std::vector<float> features;
    
    // Basic geographic features
    features.push_back(scaleNumericalFeature("lat", static_cast<float>(incident.lat)));
    features.push_back(scaleNumericalFeature("lon", static_cast<float>(incident.lon)));
    
    // Calculate distance from Nashville city center (36.1627, -86.7816) using haversine
    double nashville_center_lat = 36.1627;
    double nashville_center_lon = -86.7816;
    
    double distance = calculateDistanceFromCenter(incident.lat, incident.lon, 
                                                 nashville_center_lat, nashville_center_lon);
    
    features.push_back(scaleNumericalFeature("distance_from_center", static_cast<float>(distance)));
    
    return features;
}

std::vector<float> MLFireModel::extractCategoricalFeatures(const Incident& incident) {
    std::vector<float> features;
    
    // Extract categorical features and encode them
    
    // Shift (determined by hour)
    int hour = extractHour(incident.reportTime);
    std::string shift;
    if (hour >= 6 && hour < 14) {
        shift = "Day";
    } else if (hour >= 14 && hour < 22) {
        shift = "Evening";  
    } else {
        shift = "Night";
    }
    
    auto shift_encoded = encodeCategoricalFeature("shift", shift);
    features.insert(features.end(), shift_encoded.begin(), shift_encoded.end());
    
    // Season (determined by month)
    int month = extractMonth(incident.reportTime);
    std::string season;
    if (month == 12 || month == 1 || month == 2) {
        season = "Winter";
    } else if (month >= 3 && month <= 5) {
        season = "Spring";
    } else if (month >= 6 && month <= 8) {
        season = "Summer";
    } else {
        season = "Fall";
    }
    
    auto season_encoded = encodeCategoricalFeature("season", season);
    features.insert(features.end(), season_encoded.begin(), season_encoded.end());
    
    // Incident category
    std::string category_str = to_string(incident.category);
    auto category_encoded = encodeCategoricalFeature("category", category_str);
    features.insert(features.end(), category_encoded.begin(), category_encoded.end());
    
    // Zone ID (handle NaN values properly)
    std::string zone_id;
    if (incident.zoneIndex >= 0) {
        zone_id = std::to_string(incident.zoneIndex) + ".0"; // Use integer conversion, not float
    } else {
        zone_id = "nan"; // Handle missing/invalid zone IDs
    }
    auto zone_encoded = encodeCategoricalFeature("ZONE_ID", zone_id);
    features.insert(features.end(), zone_encoded.begin(), zone_encoded.end());
    
    // Incident type
    std::string incident_type = to_string(incident.incident_type);
    auto type_encoded = encodeCategoricalFeature("incident_type", incident_type);
    features.insert(features.end(), type_encoded.begin(), type_encoded.end());
    
    return features;
}


