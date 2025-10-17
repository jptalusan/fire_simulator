#ifndef VEHICLE_H
#define VEHICLE_H

#include "enums.h"
#include <string>
#include <ctime>

// Forward declare Location to avoid circular include
#include "objects/common.h"

class Vehicle {
    public:
        Vehicle():
            stationIndex(-1),
            vehicleId(-1),
            incidentIndex(-1),
            timeToIncident(-1),
            timeToReturn(-1),
            stationId(""),
            currentLocation(), // default constructed
            stationLocation(), // default constructed
            apparatusType(ApparatusType::Invalid),
            apparatusStatus(ApparatusStatus::Available)
        {}
        // Constructor used across the codebase (no Location provided)
        Vehicle(int stationIndex,
                const std::string& stationId,
                int vehicleId,
                ApparatusType apparatusType,
                ApparatusStatus apparatusStatus) :
            stationIndex(stationIndex),
            vehicleId(vehicleId),
            incidentIndex(-1),
            timeToIncident(-1),
            timeToReturn(-1),
            stationId(stationId),
            currentLocation(), // default constructed
            stationLocation(), // default constructed
            apparatusType(apparatusType),
            apparatusStatus(apparatusStatus)
        {}

        // Optional constructor that accepts a Location by reference
        Vehicle(int stationIndex,
                const std::string& stationId,
                int vehicleId,
                const Location& location,
                ApparatusType apparatusType,
                ApparatusStatus apparatusStatus) :
            stationIndex(stationIndex),
            vehicleId(vehicleId),
            incidentIndex(-1),
            timeToIncident(-1),
            timeToReturn(-1),
            stationId(stationId),
            currentLocation(location),
            stationLocation(location),
            apparatusType(apparatusType),
            apparatusStatus(apparatusStatus)
        {}

        // Print vehicle details including all properties
        void printDetails() const;
        ApparatusStatus getStatus() const noexcept { return apparatusStatus; }
        ApparatusType getType() const noexcept { return apparatusType; }
        const Location& getCurrentLocation() const noexcept { return currentLocation; }
        const Location& getStationLocation() const noexcept { return stationLocation; }
        int getVehicleId() const noexcept { return vehicleId; }
        int getStationIndex() const noexcept { return stationIndex; }
        const std::string& getStationId() const noexcept { return stationId; }
        void setStatus(ApparatusStatus status) { apparatusStatus = status; }
        void setTimeToIncident(time_t time) { timeToIncident = time; }
        void setTimeToReturn(time_t time) { timeToReturn = time; }
        void setIncidentIndex(int index) { incidentIndex = index; }
        int getIncidentIndex() const noexcept { return incidentIndex; }
        time_t getTimeToIncident() const noexcept { return timeToIncident; }
        time_t getTimeToReturn() const noexcept { return timeToReturn; }
        void setCurrentLocation(const Location& location) { currentLocation = location; }
        time_t timeToStartedReturning = -1;
        time_t timeStartedToDispatch = -1;

    private:
        int stationIndex;
        int vehicleId;
        int incidentIndex;
        time_t timeToIncident;
        time_t timeToReturn;
        std::string stationId;
        Location currentLocation;
        Location stationLocation;
        ApparatusType apparatusType;
        ApparatusStatus apparatusStatus;
};

#endif // VEHICLE_H