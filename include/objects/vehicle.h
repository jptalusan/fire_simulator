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

        // EMS transport state getters/setters
        int getHospitalIndex() const noexcept { return hospitalIndex_; }
        void setHospitalIndex(int index) { hospitalIndex_ = index; }
        time_t getEmsSceneEndTime() const noexcept { return emsSceneEndTime_; }
        void setEmsSceneEndTime(time_t time) { emsSceneEndTime_ = time; }
        time_t getTimeToHospital() const noexcept { return timeToHospital_; }
        void setTimeToHospital(time_t time) { timeToHospital_ = time; }
        time_t getHospitalLeaveTime() const noexcept { return hospitalLeaveTime_; }
        void setHospitalLeaveTime(time_t time) { hospitalLeaveTime_ = time; }
        bool getRequiresTransport() const noexcept { return requiresTransport_; }
        void setRequiresTransport(bool required) { requiresTransport_ = required; }
        time_t getIncidentArrivalTime() const noexcept { return incidentArrivalTime_; }
        void setIncidentArrivalTime(time_t time) { incidentArrivalTime_ = time; }
        int getTransportIncidentIndex() const noexcept { return transportIncidentIndex_; }
        void setTransportIncidentIndex(int index) { transportIncidentIndex_ = index; }

        // Reset all EMS transport state
        void clearEMSTransportState() {
            hospitalIndex_ = -1;
            emsSceneEndTime_ = -1;
            timeToHospital_ = -1;
            hospitalLeaveTime_ = -1;
            requiresTransport_ = false;
            incidentArrivalTime_ = -1;
            transportIncidentIndex_ = -1;
        }

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

        // EMS transport state
        int hospitalIndex_ = -1;           // Target hospital index (-1 if not transporting)
        time_t emsSceneEndTime_ = -1;      // When EMS can leave scene
        time_t timeToHospital_ = -1;       // ETA at hospital
        time_t hospitalLeaveTime_ = -1;    // When to leave hospital
        bool requiresTransport_ = false;   // Whether current call requires transport
        time_t incidentArrivalTime_ = -1;  // When the medic arrived at the incident
        int transportIncidentIndex_ = -1;  // Incident index being transported (for reporting)
};

#endif // VEHICLE_H