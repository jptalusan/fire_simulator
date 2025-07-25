#include <cstdlib>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include "data/incident.h"
#include "services/queries.h"
#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#include "policy/firebeats_dispatch.h"
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

EventQueue generateEvents(const std::vector<Incident>& incidents) {
    std::vector<Event> container;
    container.reserve(incidents.size());  // Preallocate memory for efficiency
    EventQueue events(std::greater<Event>(), std::move(container));

    for (size_t i = 0; i < incidents.size(); ++i) {
        int incidentIndex = incidents[i].incidentIndex;
        time_t reportTime = incidents[i].reportTime;
        Event incidentEvent = Event::createIncidentEvent(reportTime, incidentIndex);
        events.push(incidentEvent);
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
        sortedIncidents.emplace_back(incident);
    }
    std::sort(sortedIncidents.begin(), sortedIncidents.end(),
        [](const Incident& a, const Incident& b) {
            return a.reportTime < b.reportTime;
        });

    for (size_t i = 0; i < sortedIncidents.size(); ++i) {
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
            << incident.zoneIndex << ","
            << to_string(incident.status) << "\n";
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
    std::string matrix_csv_path = env.get("MATRIX_CSV_PATH", "../logs/matrix.csv");
    std::string distance_matrix_path = env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin");
    std::string duration_matrix_path = env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin");
    std::string beats_shapefile_path = env.get("BEATS_SHAPEFILE_PATH", "../data/beats_shpfile.geojson");
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
    std::vector<std::pair<int, Polygon>> polygonWithZoneID = loadServiceZonesFromGeojson(beats_shapefile_path);
    std::vector<Polygon> polygons;
    polygons.reserve(polygonWithZoneID.size());
    for (const auto& pair : polygonWithZoneID) {
        polygons.emplace_back(pair.second);
    }
    std::vector<Point> points;
    points.reserve(incidents.size());  // Preallocate memory for efficiency
    for (auto& incident : incidents) {
        points.emplace_back(Point(incident.lon, incident.lat));
    }
    auto results = getPointToPolygonIndices(points, polygons);
    int notThere = 0;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i]) {
            int zoneIndex = polygonWithZoneID.at(*results[i]).first;
            incidents.at(i).zoneIndex = zoneIndex;
        } else {    
            notThere++;
        }
    }
    spdlog::error("There are {} incidents that are not in any service zone.", notThere);
    spdlog::error("There are {} incidents in service zones.", results.size() - notThere);
    // END Adding zones per incident (maybe costly?)

    std::vector<Location> sources;
    sources.reserve(stations.size());  // Preallocate memory for efficiency
    for (const auto& station : stations) {
        sources.emplace_back(station.getLocation());
    }
    std::vector<Location> destinations;
    destinations.reserve(incidents.size());  // Preallocate memory for efficiency
    for (const auto& incident : incidents) {
        destinations.emplace_back(incident.getLocation());
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

void writeActions(const Simulator& simulator, State& state) {
    EnvLoader env("../.env");
    std::string station_report_path = env.get("STATION_REPORT_CSV_PATH", "../logs/station_report.csv");

    std::vector<Action> actionHistory = simulator.getActionHistory();

    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    std::unordered_map<int, Incident>& doneIncidents = state.doneIncidents_;

    // Insert all elements from doneIncidents into activeIncidents
    activeIncidents.insert(doneIncidents.begin(), doneIncidents.end()); // Existing keys in activeIncidents are NOT overwritten
    
    std::string stationMetricHeader = "DispatchTime,StationID,EnginesDispatched,EnginesRemaining,TravelTime,IncidentID,IncidentIndex";
    std::ofstream station_csv(station_report_path);
    station_csv << stationMetricHeader << "\n";

    for (const auto& action : actionHistory) {
        if (action.type != StationActionType::Dispatch) {
            continue; // Skip non-dispatch actions
        }
        std::string metrics;
        metrics.reserve(128);

        const Incident& incident = activeIncidents.at(action.payload.incidentIndex);
        const Station& station = state.getStation(action.payload.stationIndex);
        int numberOfFireTrucksToDispatch = action.payload.enginesCount;
        int fireTrucksRemainingAtTime = station.getNumFireTrucks() - numberOfFireTrucksToDispatch;
        double travelTime = action.payload.travelTime;
        fmt::format_to(std::back_inserter(metrics), 
                    "{},{},{},{},{:.2f},{},{}",
        formatTime(incident.timeRespondedTo), station.getStationIndex(), 
        numberOfFireTrucksToDispatch, fireTrucksRemainingAtTime,
        travelTime, incident.incident_id, incident.incidentIndex);
        station_csv << metrics << "\n";
    }
    station_csv.close();
}

int main(int argc, char* argv[]) {
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

    EventQueue events = generateEvents(incidents);

    State initial_state;
    initial_state.populateAllIncidents(incidents); // Populate all incidents in the state (only used for passing incidents cheaply)
    initial_state.advanceTime(events.top().event_time); // Set initial time to the first event's time
    initial_state.addStations(stations);
    // initial_state.setLastEventId(events.back().eventId); // Set the last event ID

    DispatchPolicy* policy = new NearestDispatch(env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
                                                 env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"));

    // DispatchPolicy* policy = new FireBeatsDispatch(
    //     env.get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
    //     env.get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"),
    //     env.get("FIREBEATS_MATRIX_PATH", "../logs/firebeats_matrix.bin"),
    //     env.get("ZONE_MAP_PATH", "../data/zones.csv")
    // );

    int seed = std::stoi(env.get("RANDOM_SEED", "42"));
    FireModel* fireModel = new HardCodedFireModel(seed);

    EnvironmentModel environment_model(*fireModel);
    Simulator simulator(initial_state, events, environment_model, *policy);
    simulator.run();

    #ifdef HAVE_SPDLOG_STOPWATCH
    spdlog::error("Simulation completed successfully in {:.3} s.", sw);
    #endif

    writeReportToCSV(simulator.getCurrentState(), env);
    writeActions(simulator, simulator.getCurrentState());
    delete policy;
    delete fireModel;

    // Call Python script after simulation finishes
    if (argc > 1 && std::string(argv[1]) == "--run-python") {
        std::string python_path = env.get("PYTHON_PATH", "/opt/homebrew/bin/python3");

        int status = std::system((python_path + " ../scripts/process_csv.py").c_str());

        if (status == 0) {
            std::cout << "GeoJSON generated successfully." << std::endl;
        } else {
            std::cerr << "Failed to run Python script." << std::endl;
        }

        return 0;
    } else {
        spdlog::info("Skipping CSV generation as per command line argument.");
        return 0;
    }
}
