#include "data/fire_truck.h"

FireTruck::FireTruck(int id)
    : id_(id), time_to_destination_(0) {}

int FireTruck::getId() const { return id_; }

int FireTruck::getTimeToDestination() const { return time_to_destination_; }

void FireTruck::setTimeToDestination(int seconds) {
    time_to_destination_ = seconds;
}

void FireTruck::tick() {
    if (time_to_destination_ > 0) {
        --time_to_destination_;
    }
}
