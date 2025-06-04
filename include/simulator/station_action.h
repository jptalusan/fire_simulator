// station_action.h
#ifndef STATION_ACTION_H
#define STATION_ACTION_H

#include <string>
#include "simulator/event_data.h"

class FireStationAction : public EventData {
public:
    std::string action_type;
    std::string target;

    FireStationAction(const std::string& action, const std::string& target);
    void printInfo() const override;
};

#endif // STATION_ACTION_H
