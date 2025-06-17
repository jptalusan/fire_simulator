#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include "data/incident.h"
#include "services/queries.h"
#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#include "spdlog/stopwatch.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "utils/helpers.h"

std::vector<Event> generateEvents(const std::vector<Incident>& incidents) {
    std::vector<Event> events;
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

void writeReportToCSV(const std::vector<State>& state_history, const std::string& filename) {
    State state = state_history.back();
    std::unordered_map<int, Incident>& activeIncidents = state.getActiveIncidents();
    std::ofstream csv(filename);
    csv << "IncidentID,Reported,Responded,Resolved,TravelTime,StationID,EngineCount\n";

    // Copy incidents to a vector and sort by reportTime if desired
    std::vector<Incident> sortedIncidents;
    for (const auto& [id, incident] : activeIncidents) {
        sortedIncidents.push_back(incident);
    }
    std::sort(sortedIncidents.begin(), sortedIncidents.end(),
        [](const Incident& a, const Incident& b) {
            return a.reportTime < b.reportTime;
        });

    for (const auto& incident : sortedIncidents) {
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

int main() {
    EnvLoader env("../.env");
    setupLogger(env);
    
    spdlog::info("Starting Fire Simulator...");
    spdlog::stopwatch sw;

    // Additional logic can be added here
    std::string incidents_path = env.get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string stations_path = env.get("STATIONS_CSV_PATH", "../data/stations.csv");
    std::string report_path = env.get("REPORT_CSV_PATH", "../logs/incident_report.csv");

    std::vector<Incident> incidents = loadIncidentsFromCSV(incidents_path);
    std::vector<Event> events = generateEvents(incidents);

    sortEventsByTime(events);

    for (const auto& event : events) {
        event.payload->printInfo(); // Dispatches to correct derived class print method
    }

    State initial_state;
    initial_state.advanceTime(events.front().event_time); // Set initial time to the first event's time
    initial_state.addStations(loadStationsFromCSV(stations_path));

    DispatchPolicy* policy = new NearestDispatch(env.get("BASE_OSRM_URL", "http://router.project-osrm.org"));

    EnvironmentModel environment_model;
    Simulator simulator(initial_state, events, environment_model, *policy);
    simulator.run();

    // simulator.replay();
    spdlog::info("Simulation completed successfully.");
    spdlog::info("Elapsed {:.3}", sw);

    std::vector<State> state_history = simulator.getStateHistory();
    writeReportToCSV(state_history, report_path);
    delete policy;
    return 0;
}