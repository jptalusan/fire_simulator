#ifndef ENUMS_H
#define ENUMS_H

#include <string>

enum class EventType : uint8_t {
    Incident,
    CheckIncident,
    ApparatusArrivalAtIncident,
    IncidentResolution,
    ApparatusReturnToStation
};

enum class StationActionType : uint8_t {
    Dispatch,
    DoNothing
};

enum class IncidentLevel : uint8_t {
    Invalid,
    Low,
    Moderate,
    High,
    Critical
};

enum class IncidentType : uint8_t {
    Fire,
    Medical,
    Invalid
};

enum class IncidentStatus : uint8_t {
    hasBeenReported,
    hasBeenRespondedTo,
    isBeingResolved,
    hasBeenResolved
};

enum class ApparatusStatus : uint8_t {
    Available,
    Dispatched,
    EnRouteToIncident,
    AtIncident,
    ReturningToStation
};

enum class ApparatusType : uint8_t {
    Pumper,
    Engine,
    Truck,
    Rescue,
    Hazard,
    Chief,
    Squad,
    Fast,
    Medic,
    Brush,
    Boat,
    UTV,
    Reach,
    Invalid // Added for safety, should not be used in practice
};

// to_string for IncidentType
inline std::string to_string(IncidentType type) {
    switch (type) {
        case IncidentType::Fire: return "Fire";
        case IncidentType::Medical: return "Medical";
        case IncidentType::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

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


inline const char* to_string(ApparatusType type) {
    switch (type) {
        case ApparatusType::Pumper: return "Pumper";
        case ApparatusType::Engine: return "Engine";
        case ApparatusType::Truck: return "Truck";
        case ApparatusType::Rescue: return "Rescue";
        case ApparatusType::Hazard: return "Hazard";
        case ApparatusType::Chief: return "Chief";
        case ApparatusType::Squad: return "Squad";
        case ApparatusType::Fast: return "Fast";
        case ApparatusType::Medic: return "Medic";
        case ApparatusType::Brush: return "Brush";
        case ApparatusType::Boat: return "Boat";
        case ApparatusType::UTV: return "UTV";
        case ApparatusType::Reach: return "Reach";
        default: return "Invalid";
    }
}
#endif // ENUMS_H
