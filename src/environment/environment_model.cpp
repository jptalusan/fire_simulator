#include "environment/environment_model.h"
#include "data/incident.h"
#include "simulator/station_action.h"
#include <iostream>
#include <spdlog/spdlog.h>

void EnvironmentModel::handleEvent(State& state, const Event& event) {
    spdlog::info("[EnvironmentModel] Handling event at time: {}", event.event_time);

    switch (event.event_type) {
        case EventType::Incident: {
            auto incident = std::dynamic_pointer_cast<Incident>(event.payload);
            if (incident) {
                spdlog::debug("Handling Incident: {} | Level: {}", incident->incident_type, incident->incident_level);

                // Example logic: mark incident as responded
                state.addIncident(*incident);
            }
            break;
        }

        case EventType::StationAction: {
            auto action = std::dynamic_pointer_cast<FireStationAction>(event.payload);
            if (action) {
                spdlog::debug("Handling StationAction: {}", action->action_type);
                // Example: dispatch or resolve, etc.
            }
            break;
        }

        default:
            spdlog::warn("Unhandled event type.");
            break;
    }

    state.advanceTime(event.event_time); // Update system time in state
}
