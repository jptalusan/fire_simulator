#ifndef FIRE_H
#define FIRE_H

#include <random>
#include "enums.h"
#include "simulator/state.h"
#include "utils/util.h"
#include "models/onnx_predictor.h"
#include <nlohmann/json.hpp>

struct ResolutionStats {
    double mean;
    double variance;
    int count; 
};

IncidentCategory stringToIncidentCategory(const std::string& str);

// Create a parent class for all fire-related models
class ServiceTimeAndApparatusModel {
public:
    virtual ~ServiceTimeAndApparatusModel() = default;
    virtual double computeResolutionTime(State& state, const Incident& incident) = 0;
    virtual std::unordered_map<ApparatusType, int> calculateApparatusCount(const Incident& incident) = 0; // Calculate the number of apparatus needed for an incident
};

class HardCodedFireModel : public ServiceTimeAndApparatusModel {
public:
    HardCodedFireModel(unsigned int seed);

    // Returns true with the given probability
    bool shouldResolveIncident(double probability);
    // Function to compute the resolution time for a fire incident
    double computeResolutionTime(State& state, const Incident& incident) override;
    std::unordered_map<ApparatusType, int> calculateApparatusCount(const Incident& incident) override;
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
};

class HistoricalFireModel : public ServiceTimeAndApparatusModel {
public:
    HistoricalFireModel(unsigned int seed, const std::string& csv_path, const std::string& resolution_stats_path = "");
    double computeResolutionTime(State& state, const Incident& incident) override;
    bool shouldResolveIncident(double probability);
    std::unordered_map<ApparatusType, int> calculateApparatusCount(const Incident& incident) override;

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
    std::unordered_map<IncidentCategory, std::unordered_map<ApparatusType, int>> apparatus_requirements_;
    std::unordered_map<IncidentCategory, ResolutionStats> resolution_stats_;
    void loadApparatusRequirements(const std::string& csv_path);
    void loadResolutionStats(const std::string& resolutionStats_path);
};

class MLFireModel : public ServiceTimeAndApparatusModel {
public:
    MLFireModel(unsigned int seed, const std::string& model_path, const std::string& config_path,const std::string& apparatus_csv_path);
    std::unordered_map<ApparatusType, int> calculateApparatusCount(const Incident& incident) override;
    double computeResolutionTime(State& state, const Incident& incident) override;
    void validateFeatureOrder() const;
    void printFeatureOrder(size_t max_features = 50) const;
private:
    // ONNX predictor for ML inference
    std::unique_ptr<ONNXPredictor> onnx_predictor_;
    
    // Model configuration from JSON
    nlohmann::json feature_config_;
    std::vector<std::string> numerical_features_;
    std::vector<std::string> categorical_features_;
    std::vector<std::string> feature_order_; // Exact order from JSON
    std::map<std::string, std::map<std::string, int>> categorical_mappings_;
    std::map<std::string, std::pair<double, double>> numerical_scaling_; // mean, scale pairs
    
    // Model metadata
    int expected_input_features_;
    std::string model_type_;

    void loadONNXModel(const std::string& model_path);
    void loadFeatureConfig(const std::string& config_path);
    std::unordered_map<IncidentCategory, std::unordered_map<ApparatusType, int>> apparatus_requirements_;
    void loadApparatusRequirements(const std::string& csv_path);

    // Feature extraction methods
    std::vector<float> extractFeatures(const State& state, const Incident& incident);
    std::vector<float> extractTemporalFeatures(const Incident& incident);
    std::vector<float> extractGeographicFeatures(const Incident& incident);
    std::vector<float> extractCategoricalFeatures(const Incident& incident);
    // std::vector<float> extractWorkloadFeatures(const State& state);
    
    // Preprocessing utilities
    float scaleNumericalFeature(const std::string& feature_name, float value);
    std::vector<float> encodeCategoricalFeature(const std::string& feature_name, const std::string& value);
};

#endif // FIRE_H
