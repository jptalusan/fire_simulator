#include <string>
#include <vector>
#include "config/EnvLoader.h"
#include "objects/location.h"
#include "objects/incident.h"
#include "objects/vehicle.h"
#include "simulator/event.h"
#include "models/travel_time_model.h"

namespace loader {
std::pair<std::vector<FireStation>, std::vector<Vehicle>> loadStationsFromCSV();
std::vector<Incident> loadIncidentsFromCSV();

EventQueue generateEvents(const std::vector<Incident>& incidents);
void preComputingMatrices(std::vector<FireStation>& stations, 
                          std::vector<Incident>& incidents,
                          std::vector<Vehicle>& apparatuses,
                          size_t chunk_size = 100,
                          TravelTimeModel* travelTimeModel = nullptr);
}