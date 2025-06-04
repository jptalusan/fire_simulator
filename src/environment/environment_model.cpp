#include "environment/environment_model.h"
#include "data/incident.h"
#include "simulator/station_action.h"
#include <iostream>

void EnvironmentModel::handleEvent(State& state, const Event& event) {
    std::cout << "[EnvironmentModel] Handling event at time: " << event.event_time << "\n";

    switch (event.event_type) {
        case EventType::Incident: {
            auto incident = std::dynamic_pointer_cast<Incident>(event.payload);
            if (incident) {
                std::cout << "Handling Incident: " << incident->incident_type
                          << " | Level: " << incident->incident_level << "\n";

                // Example logic: mark incident as responded
                state.addRespondedIncident(incident->incident_id);
            }
            break;
        }

        case EventType::StationAction: {
            auto action = std::dynamic_pointer_cast<FireStationAction>(event.payload);
            if (action) {
                std::cout << "Handling StationAction: " << action->action_type << "\n";
                // Example: dispatch or resolve, etc.
            }
            break;
        }

        default:
            std::cerr << "Unhandled event type.\n";
            break;
    }

    state.advanceTime(event.event_time); // Update system time in state
}
