#ifndef ACTION_H
#define ACTION_H

struct ActionPayload {
    double travelTime = 0.0; // in seconds
    int stationIndex = -1;
    int incidentIndex = -1;
    int enginesCount = -1;
    int priority = 0;
    
    ActionPayload() = default;
    ActionPayload(int station, int incident, int engines, double travel, int prio = 0)
        :  travelTime(travel), stationIndex(station), incidentIndex(incident), enginesCount(engines), priority(prio) {}
};

class Action {
public:
    StationActionType type;
    ActionPayload payload;  // Direct struct instead of map
    
    Action() = default;
    Action(StationActionType type_, const ActionPayload& payload_ = {})
        : type(type_), payload(payload_) {}
    
    // Factory methods for type safety
    static Action createDispatchAction(int stationIndex, int incidentIndex, int enginesCount, double travelTime) {
        return Action(StationActionType::Dispatch, 
                     ActionPayload(stationIndex, incidentIndex, enginesCount, travelTime));
    }
    
    static Action createDoNothingAction() {
        return Action(StationActionType::DoNothing, ActionPayload());
    }
};

#endif // ACTION_H
