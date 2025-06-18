#ifndef ACTION_H
#define ACTION_H

#include "enums.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>

/**
 * @brief Represents an action to be performed by a station, including its type and an optional payload.
 */
class Action {
public:
    StationActionType type;
    // The payload can be a dictionary of string keys to string values (customize as needed)
    std::unordered_map<std::string, std::string> payload;

    Action() = default;
    Action(StationActionType type_, const std::unordered_map<std::string, std::string>& payload_ = {})
        : type(type_), payload(payload_) {}

    /**
     * @brief Returns a string representation of the Action for logging.
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "Action Type: ";
        switch (type) {
            case StationActionType::Dispatch:      oss << "Dispatch"; break;
            case StationActionType::DoNothing:     oss << "DoNothing"; break;
            default: oss << "Unknown"; break;
        }
        oss << ", Payload: {";
        bool first = true;
        for (const auto& kv : payload) {
            if (!first) oss << ", ";
            oss << kv.first << ": " << kv.second;
            first = false;
        }
        oss << "}";
        return oss.str();
    }
};

#endif // ACTION_H
