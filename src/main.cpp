#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include "data/incident.h"
#include "services/queries.h"
#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
// #include "spdlog/stopwatch.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "utils/helpers.h"
#include "services/chunks.h"
#include "utils/error.h"

std::vector<Event> generateEvents(const std::vector<Incident>& incidents) {
    std::vector<Event> events;
    events.reserve(incidents.size());  // Preallocate memory for efficiency
    for (const auto& incident : incidents) {
        auto inc_ptr = std::make_shared<Incident>(incident);
        Event e(EventType::Incident, incident.reportTime, inc_ptr);
        events.push_back(e);
    }
    return events;
}

void setupLogger(EnvLoader& env) {
    std::string logs_path = env.get("LOGS_PATH", "../logs/basic-log.log");

    try 
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logs_path, true);

        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%^%l%$] %v");

        file_sink->set_level(spdlog::level::debug);
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

void writeReportToCSV(State& state, const std::string& filename) {
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    std::ofstream csv(filename);
    csv << "IncidentID,Reported,Responded,Resolved,TravelTime,StationID,EngineCount\n";

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

    for (const auto& incident : sortedIncidents) {
        if (incident.resolvedTime < 0 || incident.resolvedTime > 2147483647) {
            spdlog::error("Incident {} has a resolved time out of bounds: {}", incident.incident_id, incident.resolvedTime);
            continue; // Skip this incident
        }
        csv << incident.incident_id << ","
            << formatTime(incident.reportTime) << ","
            << formatTime(incident.responseTime) << ","
            << formatTime(incident.resolvedTime) << ","
            << incident.oneWayTravelTimeTo << ","
            << incident.stationIndex << ","
            << incident.engineCount << "\n";
    }
    csv.close();
}

// Debug tool
void preComputingMatrices(std::vector<Station>& stations, std::vector<Incident>& incidents, size_t chunk_size = 100) {
    spdlog::info("Starting Precomputation...");
    //spdlog::stopwatch sw;
    EnvLoader env("../.env");
    // Additional logic can be added here
    std::string stations_path = env.get("STATIONS_CSV_PATH", "../data/stations.csv");
    std::string incidents_path = env.get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string distance_matrix_path = env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin");
    std::string duration_matrix_path = env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin");
    std::string osrmUrl_ = env.get("BASE_OSRM_URL", "http://router.project-osrm.org");

    if (checkOSRM(osrmUrl_)) {
        spdlog::info("OSRM server is reachable and working correctly.");
    } else {
        spdlog::error("OSRM server is not reachable.");
        throw OSMError();
    }

    stations = loadStationsFromCSV(stations_path);
    incidents = loadIncidentsFromCSV(incidents_path);
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
    // write_matrix_to_csv(full_matrix, matrix_path, 2, false);
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
    std::string report_path = env.get("REPORT_CSV_PATH", "../logs/incident_report.csv");

    std::vector<Incident> incidents = {};
    std::vector<Station> stations = {};

    // Debug tool to precompute matrices
    size_t chunk_size = 500;
    preComputingMatrices(stations, incidents, chunk_size);

    //spdlog::stopwatch sw;
    std::vector<Event> events = generateEvents(incidents);

    sortEventsByTimeAndType(events);

    for (const auto& event : events) {
        event.payload->printInfo(); // Dispatches to correct derived class print method
    }

    State initial_state;
    initial_state.advanceTime(events.front().event_time); // Set initial time to the first event's time
    initial_state.addStations(stations);

    DispatchPolicy* policy = new NearestDispatch(env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
                                                 env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"));

    EnvironmentModel environment_model;
    Simulator simulator(initial_state, events, environment_model, *policy);
    simulator.run();

    //spdlog::info("Simulation completed successfully in {:.3} s.", sw);

    writeReportToCSV(simulator.getCurrentState(), report_path);
    delete policy;
    return 0;
}
