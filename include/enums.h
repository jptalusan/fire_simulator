#ifndef ENUMS_H
#define ENUMS_H

#include <string>

enum class EventType {
    Incident,
    CheckIncident,
    ApparatusArrivalAtIncident,
    IncidentResolution,
    ApparatusReturnToStation
};

enum class StationActionType {
    Dispatch,
    DoNothing
};

enum class IncidentLevel {
    Invalid,
    Low,
    Moderate,
    High,
    Critical
};

enum class IncidentStatus {
    hasBeenReported,
    hasBeenRespondedTo,
    isBeingResolved,
    hasBeenResolved
};

// to_string for EventType
inline std::string to_string(EventType type) {
    switch (type) {
        case EventType::Incident: return "Incident";
        case EventType::IncidentResolution: return "IncidentResolution";
        case EventType::ApparatusArrivalAtIncident: return "ApparatusArrivalAtIncident";
        case EventType::ApparatusReturnToStation: return "ApparatusReturnToStation";
        default: return "Unknown";
    }
}

// to_string for StationActionType
inline std::string to_string(StationActionType type) {
    switch (type) {
        case StationActionType::Dispatch: return "Dispatch";
        case StationActionType::DoNothing: return "DoNothing";
        default: return "Unknown";
    }
}

// to_string for IncidentLevel
inline std::string to_string(IncidentLevel level) {
    switch (level) {
        case IncidentLevel::Invalid: return "Invalid";
        case IncidentLevel::Low: return "Low";
        case IncidentLevel::Moderate: return "Moderate";
        case IncidentLevel::High: return "High";
        case IncidentLevel::Critical: return "Critical";
        default: return "Unknown";
    }
}

inline const char* to_string(IncidentStatus status) {
    switch (status) {
        case IncidentStatus::hasBeenRespondedTo:    return "hasBeenRespondedTo";
        case IncidentStatus::hasBeenReported:       return "hasBeenReported";
        case IncidentStatus::isBeingResolved:       return "isBeingResolved";
        case IncidentStatus::hasBeenResolved:       return "hasBeenResolved";
        default:                                    return "Invalid";
    }
}
#endif // ENUMS_H
