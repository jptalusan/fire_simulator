#ifndef EMS_SERVICE_MODEL_H
#define EMS_SERVICE_MODEL_H

#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include "enums.h"
#include "objects/hospital.h"

// Forward declarations
class Incident;
class State;
class TravelTimeModel;

struct EMSSceneTimeStats {
    double mean = 0.0;
    double variance = 0.0;
    double std = 0.0;
    double min = 0.0;
    int count = 0;
};

struct EMSTransportStats {
    double transportProbability = 0.0;
    int count = 0;
};

struct HospitalTimeStats {
    double mean = 0.0;
    double variance = 0.0;
    double std = 0.0;
    int count = 0;
};

struct ZoneHospitalProb {
    std::string hospitalName;
    double probability;
    int count;
};

struct SceneTimeCouplingParams {
    double baseline_sec = 495.0;
    double ceiling_sec = 937.0;
    double residual_mean_sec = 20.0;
    double residual_std_sec = 373.0;
    bool loaded = false;
};

struct MultiMedicTransportProb {
    int medic_count;
    int transport_count;
    double probability;
};

/**
 * Abstract base class for EMS service models.
 * Different implementations can provide different strategies for:
 * - Computing EMS scene times
 * - Deciding whether transport is required
 * - Selecting hospitals for transport
 * - Computing time spent at hospital
 */
class EMSServiceModel {
public:
    virtual ~EMSServiceModel() = default;

    // Select hospital for transport
    virtual int selectHospital(const Incident& incident, const State& state,
                               TravelTimeModel& travelTimeModel) = 0;

    // Get EMS on-scene time
    virtual double computeEMSSceneTime(const Incident& incident) = 0;

    // Determine if transport is needed
    virtual bool requiresHospitalTransport(const Incident& incident) = 0;

    // Get time spent at hospital (overall fallback)
    virtual double computeHospitalTime() = 0;

    // Get time spent at hospital (per-hospital, defaults to overall)
    virtual double computeHospitalTime(const std::string& /*hospitalName*/) {
        return computeHospitalTime();
    }

    // Compute coupled scene time based on fire resolution
    virtual double computeCoupledSceneTime(const Incident& incident, time_t medicArrivalTime) = 0;

    // Determine how many medics should transport (0 or 1 in simplified model)
    virtual int determineTransportCount(int medicCount, const Incident& incident) = 0;
};

/**
 * Historical EMS Service Model - uses historical data from CSV files
 * to drive EMS behavior with statistical distributions.
 */
class HistoricalEMSServiceModel : public EMSServiceModel {
public:
    HistoricalEMSServiceModel(unsigned int seed);

    // Load historical data from CSV files
    // Returns true if loaded successfully, false otherwise (will use defaults)
    bool loadSceneTimeStats(const std::string& csvPath);
    bool loadTransportStats(const std::string& csvPath);
    bool loadHospitalTimeStats(const std::string& csvPath);
    bool loadZoneHospitalProbs(const std::string& csvPath);

    // New EMS transport model loaders
    bool loadSceneTimeCouplingParams(const std::string& csvPath);
    bool loadHospitalTimeByDest(const std::string& csvPath);
    bool loadMultiMedicTransportDist(const std::string& csvPath);

    // EMSServiceModel interface implementations
    int selectHospital(const Incident& incident, const State& state,
                       TravelTimeModel& travelTimeModel) override;
    double computeEMSSceneTime(const Incident& incident) override;
    bool requiresHospitalTransport(const Incident& incident) override;
    double computeHospitalTime() override;
    double computeHospitalTime(const std::string& hospitalName) override;
    double computeCoupledSceneTime(const Incident& incident, time_t medicArrivalTime) override;
    int determineTransportCount(int medicCount, const Incident& incident) override;

    // Check if model has loaded data
    bool hasSceneTimeData() const { return !sceneTimeStats_.empty(); }
    bool hasTransportData() const { return !transportStats_.empty(); }
    bool hasHospitalTimeData() const { return hospitalTimeStats_.count > 0; }
    bool hasCouplingParams() const { return couplingParams_.loaded; }
    bool hasMultiMedicDist() const { return !multiMedicDist_.empty(); }
    bool hasPerHospitalTimeData() const { return !perHospitalTimeStats_.empty(); }

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> uniformDist_;

    // Historical stats by incident type (key is incident_type_str from CSV)
    std::unordered_map<std::string, EMSSceneTimeStats> sceneTimeStats_;
    std::unordered_map<std::string, EMSTransportStats> transportStats_;

    // Hospital time stats (overall)
    HospitalTimeStats hospitalTimeStats_;

    // Per-hospital turnaround stats (key is hospital name)
    std::unordered_map<std::string, HospitalTimeStats> perHospitalTimeStats_;

    // Scene time coupling model parameters
    SceneTimeCouplingParams couplingParams_;

    // Multi-medic transport probability distribution
    std::vector<MultiMedicTransportProb> multiMedicDist_;

    // Zone-to-hospital probabilities (key is zone/FireBeat)
    std::unordered_map<std::string, std::vector<ZoneHospitalProb>> zoneHospitalProbs_;

    // Map hospital name to index in State's hospital list
    std::unordered_map<std::string, int> hospitalNameToIndex_;

    // Helper to get category key from incident
    std::string getCategoryKey(const Incident& incident) const;

    // Build hospital name to index mapping
    void buildHospitalNameMapping(const State& state);

    // Sample from normal distribution with given mean and std, clamped to minVal
    double sampleNormal(double mean, double std, double minVal = 60.0);
};

#endif // EMS_SERVICE_MODEL_H
