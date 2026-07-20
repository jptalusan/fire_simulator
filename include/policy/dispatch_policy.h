#ifndef DISPATCH_POLICY_H
#define DISPATCH_POLICY_H

#include "models/travel_time_model.h"
#include "simulator/state.h"
#include "simulator/event.h"
#include "simulator/action.h"

class DispatchPolicy {
public:
    DispatchPolicy(const std::vector<FireStation>& fireStations, TravelTimeModel& travelTimeModel);
    virtual ~DispatchPolicy() = default;

    // Handle dispatching logic given the state and an event
    virtual const std::vector<Action> getAction(const State& state) const = 0;
    
    // Dispatch apparatus by type priority (you can customize this order)
    std::vector<ApparatusType> dispatchOrder = {
        ApparatusType::Pumper,
        ApparatusType::Engine,
        ApparatusType::Truck,
        ApparatusType::Rescue,
        ApparatusType::Hazard,
        ApparatusType::SuppressionChief,
        ApparatusType::EMSChief,
        ApparatusType::Squad,
        ApparatusType::Fast,
        ApparatusType::Medic,
        ApparatusType::Brush,
        ApparatusType::Boat,
        ApparatusType::UTV,
        ApparatusType::Reach
    };

protected:
    int getNextIncidentIndex(const State& state) const;
    std::vector<int> getSortedIndicesByDuration(const std::vector<double>& durations) const;
    int findMinIndex(const std::vector<double>& durations) const;
    std::vector<double> getColumn(double* matrix, int width, int height, int col_index) const;
    std::vector<double> getColumn(const std::vector<std::vector<double>>& matrix, size_t col_index) const;
    std::vector<int> getColumn(int* matrix, int width, int height, int col_index) const;
    std::vector<Action> getAction_(const Incident& incident, 
        const State& state, 
        const std::vector<int>& stationOrder, 
        const std::vector<double>& durations) const;
    std::unordered_map<ApparatusType, int> getRemainingApparatusNeeded(const Incident& incident) const;

    std::vector<FireStation> fireStations_;
    std::vector<Location> fireStationLocations_;
    TravelTimeModel& travelTimeModel_;
};

#endif // DISPATCH_POLICY_H
