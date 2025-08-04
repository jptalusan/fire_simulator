#include <string>
#include <vector>
#include "config/EnvLoader.h"
#include "data/station.h"
#include "data/incident.h"
#include "data/apparatus.h"

std::vector<Station> loadStationsFromCSV(const EnvLoader& env);
std::vector<Incident> loadIncidentsFromCSV(const EnvLoader& env);
// TODO: I want it to be a separate file but it should read the same stations.csv as the Station loader above.
std::vector<Apparatus> loadApparatusFromCSV(const EnvLoader& env);
