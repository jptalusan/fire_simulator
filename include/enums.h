#ifndef ENUMS_H
#define ENUMS_H

#include <string>

enum class EventType {
    Incident,
    IncidentResolution,
    StationAction
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

// to_string for EventType
inline std::string to_string(EventType type) {
    switch (type) {
        case EventType::Incident: return "Incident";
        case EventType::IncidentResolution: return "IncidentResolution";
        case EventType::StationAction: return "StationAction";
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

#endif // ENUMS_H
