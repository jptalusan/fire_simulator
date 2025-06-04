#ifndef FIRE_TRUCK_H
#define FIRE_TRUCK_H

class FireTruck {
public:
    FireTruck(int id);

    int getId() const;
    int getTimeToDestination() const;
    void setTimeToDestination(int seconds);
    void tick(); // Reduce time by 1 unit

private:
    int id_;
    int time_to_destination_; // in seconds
};

#endif // FIRE_TRUCK_H
