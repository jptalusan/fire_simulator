#include <iostream>
#include <spdlog/spdlog.h>
#include "data/incident.h"
#include "services/queries.h"
#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#include "spdlog/stopwatch.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

std::vector<Event> generateEvents(const std::vector<Incident>& incidents) {
    std::vector<Event> events;
    for (const auto& incident : incidents) {
        auto inc_ptr = std::make_shared<Incident>(incident);
        Event e(EventType::Incident, incident.unix_time, inc_ptr);
        events.push_back(e);
    }
    return events;
}

void foo(std::string filename = "data/stations.csv") {

    auto stations = loadStationsFromCSV(filename);
    // for (const auto& station : stations) {
    //     std::cout << "Station ID: " << station.getStationId() << ", Name: " << station.getFacilityName() << std::endl;
    //     std::cout << "Location: (" << station.getLat() << ", " << station.getLon() << ")" << std::endl;
    // }
    
    Queries queries;

    std::vector<Location> sources = {
        Location(36.1627, -86.7816), // Nashville approx
        Location(36.29107537, -86.73860485) // Another point in Nashville
    };
    std::vector<Location> destinations = {
        Location(36.1745, -86.7679), // Nashville airport approx
        // Location(36.16882547, -86.86187364) // Another point near Nashville airport
    };
    std::vector<double> durations, distances;
    queries.queryTableService(sources, destinations, durations, distances);
    return;
}

int main() {
    EnvLoader env("../.env");
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
    // spdlog::set_level(spdlog::level::info); // Only info, warn, error, critical will be shown
    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
    spdlog::info("Starting Fire Simulator...");
    spdlog::stopwatch sw;

    // Additional logic can be added here
    std::string incidents_path = env.get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string stations_path = env.get("STATIONS_CSV_PATH", "../data/stations.csv");

    std::vector<Incident> incidents = loadIncidentsFromCSV(incidents_path);
    std::vector<Event> events = generateEvents(incidents);
    // Sort events by event_time
    std::sort(events.begin(), events.end(),
        [](const Event& a, const Event& b) {
            return a.event_time < b.event_time;
        });

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

    simulator.replay();
    spdlog::info("Simulation completed successfully.");
    spdlog::info("Elapsed {:.3}", sw);

    delete policy;
    return 0;
}