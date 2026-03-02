#include "policy/firebeats_dispatch.h"
#include "models/travel_time_model.h"
#include "services/chunks.h"
#include "utils/error.h"
#include "utils/logger.h"
#include <iostream>

// Firebeats naming convention is a bit confusing. and currently this is incomplete.
// Check the preprocess notebook for a list of beats that I have no idea what they mean (DSOP,BAR,HQ, etc.)
FireBeatsDispatch::FireBeatsDispatch(TravelTimeModel& travelTimeModel,
                                     const std::string& fireBeatsMatrixPath,
                                     const std::string& zoneIDToNameMapPath,
                                     std::vector<FireStation> fireStations)
    : DispatchPolicy(std::move(fireStations), travelTimeModel),
      fireBeatsMatrix_(nullptr)  {
        std::ifstream fireBeatsFile(fireBeatsMatrixPath);
        if (fireBeatsFile) {
            fireBeatsMatrix_ = getFireBeats(fireBeatsMatrixPath, fireBeatsHeight_, fireBeatsWidth_);
            LOG_INFO("FireBeats matrix loaded with dimensions: {}x{}", fireBeatsWidth_, fireBeatsHeight_);
        } else {
            LOG_ERROR("FireBeats matrix file not found: {}", fireBeatsMatrixPath);
            throw std::runtime_error("FireBeats matrix file not found: " + fireBeatsMatrixPath);
        }

        beatsIndexToNameMap_ = readZoneIndexToNameMapCSV(zoneIDToNameMapPath);
    }

FireBeatsDispatch::~FireBeatsDispatch() {
    delete[] fireBeatsMatrix_; // Clean up the fire beats data if it was allocated
}

// Relies on the preprocessed bin, if its not correct then the key suddenly has the string "Station", that means it failed.
/*
Essentially reading a row: run(station) x col: zones(beats) matrix.
signifying the order or stations for each fire beats. -1 means there is no more station allocated there.
[[ 0  8  8 ... 12  7  7]
 [23 25 25 ...  7 10  3]
 [ 1 18 18 ...  3  3 12]
*/
int* FireBeatsDispatch::getFireBeats(const std::string& filename, int& height, int& width) const {
    // Open the file for reading
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file for reading: " << filename << "\n";
        return nullptr;
    }

    in.read(reinterpret_cast<char*>(&width), sizeof(int));
    in.read(reinterpret_cast<char*>(&height), sizeof(int));
    std::cout << "FireBeats matrix width: " << width << ", height: " << height << std::endl;
    if (width <= 0 || height <= 0 || width > 10000 || height > 10000) {
        std::cerr << "Invalid matrix dimensions!" << std::endl;
        return nullptr;
}

    // Allocate memory for the fire beats data
    int* fireBeatsData = new int[width * height];
    if (!fireBeatsData) {
        std::cerr << "Failed to allocate memory for fire beats data.\n";
        return nullptr;
    }

    // Read the fire beats data from the file
    in.read(reinterpret_cast<char*>(fireBeatsData), sizeof(int) * width * height);

    in.close();
    return fireBeatsData;
}

 /**
 * @brief Determines the best station to dispatch to an unresolved incident.
 *
 * This function retrieves the next unresolved incident from the simulation state,
 * gathers the locations of all stations, and queries the OSRM service to obtain
 * travel times from each station to the incident location. It is designed to help
 * select the nearest available station for dispatching resources.
 *
 * Steps:
 * @note 1. Retrieve the next unresolved incident from the state.
 * @note 2. Collect all station locations from the state.
 * @note 3. (Planned) Select the station with the lowest travel time and available trucks.
 * @note 4. (Planned) Emit a station_action event for dispatch.
 *
 * @param state The current simulation state, containing incidents and stations.
 * @return The incident ID of the unresolved incident (placeholder; will return station ID in future).
 */
// TODO: This function is gigantic!!!
const std::vector<Action> FireBeatsDispatch::getAction(const State& state) const {
    std::vector<Action> actions = {};
    if (!state.newIncident_.has_value()) {
        return actions;
    }
    const Incident& incident = state.newIncident_.value();

    // Maybe this is expensive
    std::vector<Location> emsVehicleLocations;
    std::vector<Vehicle> emsVehicles;

    for (const auto& station : fireStations_) {
        std::vector<int> _emsVehicleIds = station.getAvailableApparatus(ApparatusType::Medic);
        
        // Add vehicles from this station to the overall collection
        for (const auto& vehicleId : _emsVehicleIds) {
            // std::cout << "Found EMS Vehicle ID: " << vehicleId << " at Station: " << station.getStationId() << std::endl;
            const Vehicle& vehicle = state.getConstVehicleList().at(vehicleId);
            if (vehicle.getStatus() != ApparatusStatus::Available) {
                continue; // Skip non-available vehicles
            }
            emsVehicles.push_back(vehicle);
            emsVehicleLocations.push_back(vehicle.getCurrentLocation());
        }
    }

    int zoneIndex = incident.zoneIndex;
    // Check if the zone index is valid
    if (beatsIndexToNameMap_.find(zoneIndex) == beatsIndexToNameMap_.end()) {
        LOG_WARN("Invalid zone index: {} for incident {}. Skipping incident.", zoneIndex, incident.incident_id);
        // Return a special "skip" action to indicate this incident should be skipped
        Action skipAction = Action::createDoNothingAction();
        skipAction.shouldSkipIncident = true;
        return std::vector<Action>{skipAction};
    }

    // Given the zoneIndex (or beats ID like 38R4, find the column for that which is the order of first to last station in the beats)
    std::vector<int> beatStationIndices = getColumn(fireBeatsMatrix_, fireBeatsWidth_, fireBeatsHeight_, zoneIndex);

    std::unordered_map<ApparatusType, int> remainingNeeded = getRemainingApparatusNeeded(incident);
    
    // TODO: Check if this matches.
    std::vector<std::vector<double>> tableMatrix = 
        travelTimeModel_.getTravelTimeMatrix(fireStationLocations_, 
                                             std::vector{incident.getLocation()});
    const std::vector<double> durationColumn = getColumn(tableMatrix, size_t(0));

    std::vector<std::vector<double>> emsMatrix = 
        travelTimeModel_.getTravelTimeMatrix(emsVehicleLocations, 
                                             std::vector{incident.getLocation()});
    const std::vector<double> emsDurationColumn = getColumn(emsMatrix, size_t(0));
    std::vector<int> emsSortedIndices = getSortedIndicesByDuration(emsDurationColumn);

    for (const auto& [type, neededCount] : remainingNeeded) {
        int dispatchedCount = 0;
        bool enoughDispatched = false;
        if (type == ApparatusType::Medic) { // For EMS
            for (int index : emsSortedIndices) {
                if (index < 0) {
                    LOG_DEBUG("[{}] No more available {}, Dispatched {}, needed {}", utils::formatTime(state.getSystemTime()), to_string(type), dispatchedCount, neededCount);
                    continue;
                }

                const Vehicle& vehicle = emsVehicles.at(index);
                if (vehicle.getStatus() != ApparatusStatus::Available) {
                    continue; // Skip non-available vehicles
                }
                // Dispatch this vehicle
                Action action = Action::createDispatchAction(vehicle.getStationIndex(), 
                                                             incident.incidentIndex, 
                                                             vehicle.getVehicleId(),
                                                             type, 1, emsDurationColumn[index]);
                actions.push_back(action);
                dispatchedCount++;
                if (dispatchedCount >= neededCount) {
                    break; // Already dispatched enough of this type
                }
            }
        } else { // Fire apparatus
            for (const auto& index : beatStationIndices) {
                
                if (index < 0) {
                    LOG_DEBUG("[{}] No more available {}, Dispatched {}, needed {}", utils::formatTime(state.getSystemTime()), to_string(type), dispatchedCount, neededCount);
                    continue;
                }
                const FireStation& station = state.getAllStations().at(index);

                std::vector<int> vehicleIds = station.getAvailableApparatus(type);
                // Add vehicles from this station to the overall collection
                for (const auto& vehicleId : vehicleIds) {
                    const Vehicle& vehicle = state.getConstVehicleList().at(vehicleId);
                    if (vehicle.getStatus() != ApparatusStatus::Available) {
                        continue; // Skip non-available vehicles
                    }
                    // Dispatch this vehicle
                    Action action = Action::createDispatchAction(vehicle.getStationIndex(), 
                                                                incident.incidentIndex, 
                                                                vehicle.getVehicleId(),
                                                                type, 1, durationColumn[index]);
                    actions.push_back(action);
                    dispatchedCount++;
                    if (dispatchedCount >= neededCount) {
                        enoughDispatched = true;
                        break; // Already dispatched enough of this type
                    }
                }

                if (enoughDispatched) {
                    break; // Already dispatched enough of this type
                }
            }
        }
    }

    return actions;
}

/*
Reads a CSV with the following format.
ZoneID,Zone Name
0,01
1,10
2,10A
3,10B
4,10C
5,10D
6,10E
7,10F
8,11
9,11B
10,11C
11,11D
12,11E
*/
std::unordered_map<int, std::string> FireBeatsDispatch::readZoneIndexToNameMapCSV(const std::string &filename) const {
    std::unordered_map<int, std::string> zoneMap;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return zoneMap;
    }

    std::string line;
    // Skip the header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // Read ZoneID
        int zoneID = -1;
        std::getline(ss, token, ',');
        try {
            zoneID = std::stoi(token);
        } catch (...) {
            LOG_ERROR("Invalid zone ID: {}", token);
            throw InvalidValueError("Invalid zone ID in CSV file: " + token);
        }

        // Read Zone Name
        std::getline(ss, token);
        std::string zoneName = token;

        zoneMap[zoneID] = zoneName;
    }

    return zoneMap;
}
