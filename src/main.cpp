#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include "data/incident.h"
#include "services/queries.h"
#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#ifdef HAVE_SPDLOG_STOPWATCH
#include "spdlog/stopwatch.h"
#endif
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "utils/helpers.h"
#include "services/chunks.h"
#include "utils/error.h"
#include "models/fire.h"
#include "utils/util.h"
#include "data/geometry.h"

std::vector<Event> generateEvents(const std::vector<Incident>& incidents) {
    std::vector<Event> events;
    events.reserve(incidents.size());  // Preallocate memory for efficiency
    for (const auto& incident : incidents) {
        auto inc_ptr = std::make_shared<Incident>(incident);
        Event e(EventType::Incident, incident.reportTime, inc_ptr);
        events.push_back(e);
    }

    // HACK: Add a last event that is 1 day after the latest actual incident. will probably if the last real incident takes forever to solve.
    // Otherwise, the simulator ends prematurely.
    if (!incidents.empty()) {
        auto last_incident = incidents.back();
        last_incident.incident_id = last_incident.incident_id + 1; // Increment ID for the synthetic incident
        last_incident.incidentIndex = last_incident.incidentIndex + 1; // Increment ID for the synthetic incident
        last_incident.incident_level = IncidentLevel::Low;
        auto inc_ptr = std::make_shared<Incident>(last_incident);
        auto last_time = std::chrono::system_clock::from_time_t(last_incident.reportTime);
        auto new_time = last_time + std::chrono::hours(2); // Add 24 hours (86400 seconds) to the last incident time
        Event e(EventType::Incident, std::chrono::system_clock::to_time_t(new_time), inc_ptr);
        events.push_back(e);
    }

    return events;
}

void setupLogger(EnvLoader& env) {
    std::string logs_path = env.get("LOGS_PATH", "../logs/output.log");

    try 
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logs_path, true);

        console_sink->set_level(spdlog::level::err);
        console_sink->set_pattern("[%^%l%$] %v");

        file_sink->set_level(spdlog::level::info);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

        // Create a logger with both sinks
        auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, file_sink});
        // logger level is the one that it respects, then gets filtered into sinks above.
        logger->set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
}

void writeReportToCSV(State& state, const EnvLoader& env) {
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    std::unordered_map<int, Incident>& doneIncidents = state.doneIncidents_;

    // Insert all elements from doneIncidents into activeIncidents
    activeIncidents.insert(doneIncidents.begin(), doneIncidents.end()); // Existing keys in activeIncidents are NOT overwritten

    std::string report_path = env.get("REPORT_CSV_PATH", "../logs/incident_report.csv");
    std::string station_report_path = env.get("STATION_REPORT_CSV_PATH", "../logs/station_report.csv");
    
    std::ofstream csv(report_path);
    csv << "IncidentIndex,IncidentID,Reported,Responded,Resolved,EngineCount,Zone,Status\n";

    // Copy incidents to a vector and sort by reportTime if desired
    // Can be expensive, but its already at the end of the run
    std::vector<Incident> sortedIncidents;
    sortedIncidents.reserve(activeIncidents.size());  // Preallocate memory for efficiency
    for (const auto& [id, incident] : activeIncidents) {
        sortedIncidents.push_back(incident);
    }
    std::sort(sortedIncidents.begin(), sortedIncidents.end(),
        [](const Incident& a, const Incident& b) {
            return a.reportTime < b.reportTime;
        });

    // HACK: Ignores the last incident generated above (see generateEvents function)
    for (size_t i = 0; i < sortedIncidents.size() - 1; ++i) {
        const auto& incident = sortedIncidents[i];
        if (incident.resolvedTime < 0 || incident.resolvedTime > 2147483647) {
            spdlog::error("Incident {} has a resolved time out of bounds: {}", incident.incidentIndex, incident.resolvedTime);
            continue; // Skip this incident
        }
        csv << std::fixed << std::setprecision(6);
        csv << incident.incidentIndex << ","
            << incident.incident_id << ","
            << formatTime(incident.reportTime) << ","
            << formatTime(incident.timeRespondedTo) << ","
            << formatTime(incident.resolvedTime) << ","
            << incident.currentApparatusCount << ","
            // << incident.lat << ","
            // << incident.lon << ","
            << incident.zone << ","
            << to_string(incident.status) << "\n";
    }
    csv.close();

    // Writing station report
    std::string stationMetricHeader = "DispatchTime,StationID,StationIndex,EnginesDispatched,EnginesRemaining,TravelTime,IncidentID,IncidentIndex";
    std::ofstream station_csv(station_report_path);
    station_csv << stationMetricHeader << "\n";
    // HACK: Ignore the last incident generated above (see generateEvents function)
    for (size_t i = 0; i < state.getStationMetrics().size() - 1; ++i) {
        station_csv << state.getStationMetrics()[i] << "\n";
    }
    station_csv.close();
}

// Debug tool
void preComputingMatrices(std::vector<Station>& stations, std::vector<Incident>& incidents, size_t chunk_size = 100) {
    spdlog::info("Starting Precomputation...");
    //spdlog::stopwatch sw;
    EnvLoader env("../.env");
    // Additional logic can be added here
    std::string stations_path = env.get("STATIONS_CSV_PATH", "../data/stations.csv");
    std::string incidents_path = env.get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string matrix_csv_path = env.get("MATRIX_CSV_PATH", "../logs/matrix.csv");
    std::string distance_matrix_path = env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin");
    std::string duration_matrix_path = env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin");
    std::string osrmUrl_ = env.get("BASE_OSRM_URL", "http://router.project-osrm.org");

    if (checkOSRM(osrmUrl_)) {
        spdlog::info("OSRM server is reachable and working correctly.");
    } else {
        spdlog::error("OSRM server is not reachable.");
        throw OSRMError();
    }

    stations = loadStationsFromCSV(stations_path);
    incidents = loadIncidentsFromCSV(incidents_path);
    
    // START Adding zones per incident (maybe costly?)
    std::vector<std::pair<std::string, Polygon>> polygons_with_names = loadBoostPolygonsFromGeoJSON("../data/beats_shpfile.geojson");
    std::vector<Polygon> polygons;
    polygons.reserve(polygons_with_names.size());
    for (const auto& pair : polygons_with_names) {
        polygons.push_back(pair.second);
    }
    std::vector<Point> points;
    for (auto& incident : incidents) {
        points.push_back(Point(incident.lon, incident.lat));
    }
    auto results = getPointToPolygonIndices(points, polygons);
    int notThere = 0;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i]) {
            std::string zone = polygons_with_names.at(*results[i]).first;
            incidents.at(i).zone = zone;
        } else {
            notThere++;
        }
    }
    spdlog::error("There are {} incidents that are not in any polygon.", notThere);
    spdlog::error("There are {} incidents in polygon.", results.size() - notThere);
    // END Adding zones per incident (maybe costly?)

    std::vector<Location> sources;
    sources.reserve(stations.size());  // Preallocate memory for efficiency
    for (const auto& station : stations) {
        sources.push_back(station.getLocation());
    }
    std::vector<Location> destinations;
    destinations.reserve(incidents.size());  // Preallocate memory for efficiency
    for (const auto& incident : incidents) {
        destinations.push_back(incident.getLocation());
    }

    auto result = generate_osrm_table_chunks(sources, destinations, chunk_size);
    const std::vector<std::vector<double>>& full_distance_matrix = result.first;
    const std::vector<std::vector<double>>& full_duration_matrix = result.second;

    std::cout << full_duration_matrix.size() << " sources, " 
              << full_duration_matrix[0].size() << " destinations.\n";
    // print_matrix(full_duration_matrix, 5, 5);
    // For readability
    write_matrix_to_csv(full_duration_matrix, matrix_csv_path, 2, false);
    save_matrix_binary(full_duration_matrix, duration_matrix_path);
    save_matrix_binary(full_distance_matrix, distance_matrix_path);
   // spdlog::info("Preprocessing completed successfully in {:.3} s.", sw);
}

double* loadMatrixFromBinary(const std::string& filename, int& height, int& width) {
    return load_matrix_binary_flat(filename, height, width);
}

int main() {
    // ###### ACTUAL CODE ######
    EnvLoader env("../.env");
    setupLogger(env);

    spdlog::info("Starting Fire Simulator...");

    // Additional logic can be added here
    std::string incidents_path = env.get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string stations_path = env.get("STATIONS_CSV_PATH", "../data/stations.csv");

    std::vector<Incident> incidents = {};
    std::vector<Station> stations = {};

    size_t chunk_size = 500;
    preComputingMatrices(stations, incidents, chunk_size);

    #ifdef HAVE_SPDLOG_STOPWATCH
    spdlog::stopwatch sw;
    #endif
    std::vector<Event> events = generateEvents(incidents);

    sortEventsByTimeAndType(events);

    State initial_state;
    initial_state.advanceTime(events.front().event_time); // Set initial time to the first event's time
    initial_state.addStations(stations);

    DispatchPolicy* policy = new NearestDispatch(env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
                                                 env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"));

    int seed = std::stoi(env.get("RANDOM_SEED", "42"));
    FireModel* fireModel = new HardCodedFireModel(seed);

    EnvironmentModel environment_model(*fireModel);
    Simulator simulator(initial_state, events, environment_model, *policy);
    simulator.run();

    #ifdef HAVE_SPDLOG_STOPWATCH
    spdlog::error("Simulation completed successfully in {:.3} s.", sw);
    #endif

    writeReportToCSV(simulator.getCurrentState(), env);
    delete policy;
    delete fireModel;
    return 0;
}