// station_action.cpp
#include "simulator/station_action.h"
#include <iostream>

FireStationAction::FireStationAction(const std::string& action, const std::string& target)
    : action_type(action), target(target) {}

void FireStationAction::printInfo() const {
    std::cout << "Action: " << action_type << ", Target: " << target << "\n";
}
