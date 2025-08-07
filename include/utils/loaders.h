#include <string>
#include <vector>
#include "config/EnvLoader.h"
#include "data/station.h"
#include "data/incident.h"
#include "data/apparatus.h"
#include "simulator/event.h"

namespace loader {
std::vector<Station> loadStationsFromCSV();
std::vector<Incident> loadIncidentsFromCSV();
std::vector<Apparatus> loadApparatusFromCSV();
EventQueue generateEvents(const std::vector<Incident>& incidents);
void preComputingMatrices(std::vector<Station>& stations, 
                          std::vector<Incident>& incidents,
                          std::vector<Apparatus>& apparatuses,
                          size_t chunk_size = 100);
}