#ifndef FIREBEATS_DISPATCH_H
#define FIREBEATS_DISPATCH_H

#include "models/travel_time_model.h"
#include "dispatch_policy.h"
    
class FireBeatsDispatch : public DispatchPolicy {
public:
    FireBeatsDispatch(TravelTimeModel& travelTimeModel,
                      const std::string& fireBeatsMatrixPath="",
                      const std::string& zoneIDToNameMapPath="",
                      std::vector<FireStation> fireStations = {});

    const std::vector<Action> getAction(const State& state) const override;

    ~FireBeatsDispatch();
private:
    int width_;
    int height_;
    // FireBeats matrix
    std::string fireBeatsMatrixPath_;
    int* fireBeatsMatrix_;
    int fireBeatsWidth_;
    int fireBeatsHeight_;
    std::unordered_map<int, std::string> beatsIndexToNameMap_;

    int* getFireBeats(const std::string& filename, int& height, int& width) const;
    std::unordered_map<int, std::string> readZoneIndexToNameMapCSV(const std::string& filename) const;
};

#endif // FIREBEATS_DISPATCH_H
