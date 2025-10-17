#ifndef ACTION_H
#define ACTION_H

struct ActionPayload {
    double travelTime = 0.0; // in seconds
    int stationIndex = -1;
    int incidentIndex = -1;
    int vehicleIndex = -1;
    int apparatusCount = -1;
    int priority = 0;
    ApparatusType apparatusType;
    
    ActionPayload() = default;

    void print() const {
        std::cout << "  Station Index: " << stationIndex << "\n";
        std::cout << "  Incident Index: " << incidentIndex << "\n";
        std::cout << "  Vehicle Index: " << vehicleIndex << "\n";
        std::cout << "  Vehicle Type: " << to_string(apparatusType) << "\n";
        std::cout << "  Vehicle Count: " << apparatusCount << "\n";
        std::cout << "  Travel Time: " << travelTime << " seconds\n";
    }
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
                                       int vehicleIndex, ApparatusType type, int count, double travelTime) {
        Action action;
        action.type = StationActionType::Dispatch;
        action.payload.stationIndex = stationIndex;
        action.payload.incidentIndex = incidentIndex;
        action.payload.vehicleIndex = vehicleIndex;
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
