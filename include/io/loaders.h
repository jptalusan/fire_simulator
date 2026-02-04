#include <vector>
#include "objects/firestation.h"
#include "objects/incident.h"
#include "objects/vehicle.h"
#include "objects/hospital.h"
#include "simulator/event.h"

namespace loader {
std::pair<std::vector<FireStation>, std::vector<Vehicle>> loadStationsFromCSV();
std::vector<Incident> loadIncidentsFromCSV();
std::vector<Hospital> loadHospitalsFromCSV(const std::string& path = "");

EventQueue generateEvents(const std::vector<Incident>& incidents);
void preComputingMatrices(std::vector<FireStation>& stations, 
                          std::vector<Incident>& incidents,
                          std::vector<Vehicle>& apparatuses);
}