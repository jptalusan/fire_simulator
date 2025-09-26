#ifndef FIRE_H
#define FIRE_H

#include <random>
#include "simulator/state.h"
#include "utils/util.h"
#include "models/onnx_predictor.h"
#include <nlohmann/json.hpp>

// Create a parent class for all fire-related models
class FireModel {
public:
    virtual ~FireModel() = default;
    virtual double computeResolutionTime(State& state, const Incident& incident) = 0;
    virtual std::unordered_map<ApparatusType, int> calculateApparatusCount(const Incident& incident) = 0; // Calculate the number of apparatus needed for an incident
};

/*
The idea would be resolutionevents are triggered in-situ at every event.
It will check the current apparatus count and the total needed for the incident.
If the current apparatus count is less than the total needed, it will compute the resolution time based on the current apparatus count, total needed, and the incident level.
If the current apparatus count is greater than or equal to the total needed, it will set the incident as resolved and remove it from the active incidents list.
This model will be hardcoded for now, but can be extended later to use more complex logic or data-driven approaches.
This model will be used to compute the resolution time for fire incidents based on the current apparatus count, total needed, and incident level.
It will also handle the resolution of incidents by checking if the current apparatus count meets the total needed for the incident.
This model will be used in the EnvironmentModel to compute the resolution time for fire incidents and handle their resolution.
*/ 
class HardCodedFireModel : public FireModel {
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

struct ResolutionStats {
    double mean;
    double variance;
    int count; 
};

class DepartmentFireModel : public FireModel {
public:
    DepartmentFireModel(unsigned int seed, const std::string& csv_path, const std::string& resolution_stats_path = "");

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
class MLFireModel : public FireModel {
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
    
    // ... existing interface
};
#endif // FIRE_H
