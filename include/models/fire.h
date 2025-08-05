#ifndef FIRE_H
#define FIRE_H

#include <random>
#include "simulator/state.h"

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
#endif // FIRE_H
