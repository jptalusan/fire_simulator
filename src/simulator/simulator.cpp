#include "simulator/simulator.h"
#include "simulator/action.h"
#include "utils/helpers.h"
#include "utils/logger.h"

Simulator::Simulator(State &initialState, EventQueue &events,
                     EnvironmentModel &environmentModel,
                     DispatchPolicy &dispatchPolicy)
    : state_(initialState), events_(events), environment_(environmentModel),
      dispatchPolicy_(dispatchPolicy) {}

void Simulator::run() {
  LOG_INFO("[{}] Starting simulation.", utils::formatTime(state_.getSystemTime()));

  while (!events_.empty()) {
    Event currentEvent = events_.top();

    std::vector<Event> handleEvents =
        environment_.handleEvent(state_, currentEvent);

    events_.pop();

    std::vector<Action> actions = dispatchPolicy_.getAction(state_);

    std::vector<Event> newEvents = environment_.takeActions(state_, actions);

    for (const auto& event : handleEvents) {
        events_.push(event);
    }

    for (const auto& event : newEvents) {
        events_.push(event);
    }
    
    for (const auto& action : actions) {
      if (action.type == StationActionType::DoNothing) {
        continue;
      }
      action_history_.push_back(action);
    }
  }

  LOG_DEBUG("Number of events addressed: {}", state_.getActiveIncidents().size());
}

const std::vector<State>& Simulator::getStateHistory() const {
  return state_history_;
}

const std::vector<Action>& Simulator::getActionHistory() const {
  return action_history_;
}

State& Simulator::getCurrentState() { return state_; }

void Simulator::writeReportToCSV() {
  //All required apparatus counts and all recieved appartus counts
    std::unordered_map<int, Incident>& activeIncidents = state_.getActiveIncidents();
    std::unordered_map<int, Incident>& doneIncidents = state_.doneIncidents_;

    // Insert all elements from doneIncidents into activeIncidents
    activeIncidents.insert(doneIncidents.begin(), doneIncidents.end()); // Existing keys in activeIncidents are NOT overwritten

    std::string report_path = EnvLoader::getInstance()->get("REPORT_CSV_PATH", "../logs/incident_report.csv");

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
            LOG_ERROR("Incident {} has a resolved time out of bounds: {}", incident.incidentIndex, incident.resolvedTime);
            continue; // Skip this incident
        }
        // TODO: Fix apparatus count and add type.
        csv << std::fixed << std::setprecision(6);
        csv << incident.incidentIndex << ","
            << incident.incident_id << ","
            << utils::formatTime(incident.reportTime) << ","
            << utils::formatTime(incident.timeRespondedTo) << ","
            << utils::formatTime(incident.resolvedTime) << ","
            // << incident.currentApparatusCount << ","
            // << incident.lat << ","
            // << incident.lon << ","
            << incident.zoneIndex << ","
            << to_string(incident.status) << "\n";
    }
    csv.close();
}

void Simulator::writeActions() {
  //TODO: Remaining appaaratus count for the station and what they dispatched and how many they dispatched for the incident.
    std::string station_report_path = EnvLoader::getInstance()->get("STATION_REPORT_CSV_PATH", "../logs/station_report.csv");

    std::vector<Action> actionHistory = getActionHistory();

    std::unordered_map<int, Incident>& activeIncidents = state_.getActiveIncidents();
    std::unordered_map<int, Incident>& doneIncidents = state_.doneIncidents_;

    // Insert all elements from doneIncidents into activeIncidents
    activeIncidents.insert(doneIncidents.begin(), doneIncidents.end()); // Existing keys in activeIncidents are NOT overwritten
    
    std::string stationMetricHeader = "DispatchTime,StationID,EnginesDispatched,EnginesRemaining,TravelTime,IncidentID,IncidentIndex";
    std::ofstream station_csv(station_report_path);
    station_csv << stationMetricHeader << "\n";

    // TODO: Fix, this is totally wrong now because of apparatus count and type.
    for (const auto& action : actionHistory) {
        if (action.type != StationActionType::Dispatch) {
            continue; // Skip non-dispatch actions
        }
        std::string metrics;
        metrics.reserve(128);

        const Incident& incident = activeIncidents.at(action.payload.incidentIndex);
        const Station& station = state_.getStation(action.payload.stationIndex);
        int apparatusCount = action.payload.apparatusCount;
        int fireTrucksRemainingAtTime = station.getNumFireTrucks() - apparatusCount;
        double travelTime = action.payload.travelTime;
        fmt::format_to(std::back_inserter(metrics), 
                    "{},{},{},{},{:.2f},{},{}",
        utils::formatTime(incident.timeRespondedTo), station.getStationIndex(), 
        apparatusCount, fireTrucksRemainingAtTime,
        travelTime, incident.incident_id, incident.incidentIndex);
        station_csv << metrics << "\n";
    }
    station_csv.close();
}
