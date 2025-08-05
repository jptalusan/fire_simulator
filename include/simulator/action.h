#ifndef ACTION_H
#define ACTION_H

struct ActionPayload {
    double travelTime = 0.0; // in seconds
    int stationIndex = -1;
    int incidentIndex = -1;
    int apparatusCount = -1;
    int priority = 0;
    ApparatusType apparatusType;
    
    ActionPayload() = default;
};

class Action {
public:
    StationActionType type;
    ActionPayload payload;  // Direct struct instead of map
    
    Action() = default;
    Action(StationActionType type_, const ActionPayload& payload_ = {})
        : type(type_), payload(payload_) {}
    
    // Factory methods for type safety
    static Action createDispatchAction(int stationIndex, int incidentIndex, 
                                       ApparatusType type, int count, double travelTime) {
        Action action;
        action.type = StationActionType::Dispatch;
        action.payload.stationIndex = stationIndex;
        action.payload.incidentIndex = incidentIndex;
        action.payload.apparatusType = type;
        action.payload.apparatusCount = count;
        action.payload.travelTime = travelTime;
        return action;
    }
    
    static Action createDoNothingAction() {
        return Action(StationActionType::DoNothing, ActionPayload());
    }
};

#endif // ACTION_H
