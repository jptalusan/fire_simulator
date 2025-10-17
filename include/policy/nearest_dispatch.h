#ifndef NEAREST_DISPATCH_H
#define NEAREST_DISPATCH_H

#include "dispatch_policy.h"
#include "objects/incident.h"
#include "objects/vehicle.h"
    
class NearestDispatch : public DispatchPolicy {
public:
    NearestDispatch(std::vector<FireStation> fireStationLocations,
                    TravelTimeModel& travelTimeModel);
    ~NearestDispatch();
    const std::vector<Action> getAction(const State& state) const override;

    std::vector<Action> getEMSForIncident(const State& state, 
        const Incident& incident, 
        const std::unordered_map<ApparatusType, int>& remainingNeeded,
        const std::vector<Location>& locations, 
        const std::vector<Vehicle>& vehicles) const;

    std::vector<Action> getFireVehiclesForIncident(const State& state, 
        const Incident& incident, 
        const std::unordered_map<ApparatusType, int>& remainingNeeded,
        const std::vector<Location>& locations) const;
};

#endif // NEAREST_DISPATCH_H
