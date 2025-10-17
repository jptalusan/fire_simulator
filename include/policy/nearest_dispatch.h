#ifndef NEAREST_DISPATCH_H
#define NEAREST_DISPATCH_H

#include <memory>
#include "dispatch_policy.h"
#include "objects/incident.h"
#include "services/queries.h" // You should have an OSRM query utility class or function
    
class NearestDispatch : public DispatchPolicy {
public:
    NearestDispatch(const std::string& distanceMatrixPath="", const std::string& durationMatrixPath="");

    std::vector<Action> getAction(const State& state) const override;

    ~NearestDispatch();

    const std::vector<Action> getAction2(const State& state) const override;

    std::vector<Action> getEMSForIncident(const State& state, const Incident& incident, const std::vector<Location>& locations, const std::vector<Vehicle>& vehicles) const;
    std::vector<Action> getFireVehiclesForIncident(const State& state, const Incident& incident, const std::vector<Location>& locations) const;
private:
    std::string distanceMatrixPath_;
    std::string durationMatrixPath_;
    double* distanceMatrix_;
    double* durationMatrix_;
    int width_;
    int height_;
    std::vector<Location> fireStationLocations;
};

#endif // NEAREST_DISPATCH_H
