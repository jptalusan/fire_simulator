#include <iostream>
#include "data/incident.h"
#include "services/queries.h"
#include "simulator/simulator.h"

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
    for (const auto& s : stations) {
        std::cout << "Station ID: " << s.getStationId() << ", Name: " << s.getFacilityName() << std::endl;
    }
    
    Queries queries;

    std::vector<Location> sources = {
        Location(36.1627, -86.7816), // Nashville approx
        Location(36.29107537, -86.73860485) // Another point in Nashville
    };
    std::vector<Location> destinations = {
        Location(36.1745, -86.7679), // Nashville airport approx
        // Location(36.16882547, -86.86187364) // Another point near Nashville airport
    };
    
    queries.queryTableService(sources, destinations);
    return;
}

int main() {

    std::cout << "Hello, World!" << std::endl;
    // Additional logic can be added here
    EnvLoader env("../.env");
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

    foo(stations_path);

    State initial_state;
    initial_state.advanceTime(events.front().event_time); // Set initial time to the first event's time
    initial_state.getStations() = loadStationsFromCSV(stations_path);

    EnvironmentModel environment_model;
    Simulator simulator(initial_state, events, environment_model);
    simulator.run();

    simulator.replay();
    std::cout << "Simulation completed successfully." << std::endl;

    return 0;
}