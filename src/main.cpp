#include "models/fire_model.h"
#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#include "policy/firebeats_dispatch.h"
#include "models/incident_model.h"
#include "models/travel_time_model.h"
#include "utils/constants.h"
#include "utils/logger.h"
#include "io/loaders.h"
#include "services/chunks.h"
#include <memory>
#include <nlohmann/json.hpp>
#include "environment/environment_model.h"

#ifdef HAVE_SPDLOG_STOPWATCH
#include "spdlog/stopwatch.h"
#endif

using json = nlohmann::json;

/**
 * Print usage information
 */
void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n";
    std::cout << "\nOPTIONS:\n";
    std::cout << "  --run-python                    Run Python post-processing script after simulation\n";
    std::cout << "  --DISPATCH_POLICY=STRING        Dispatch Policy (options: [NEAREST,FIREBEATS], default: NEAREST)\n";
    std::cout << "  --FIRE_MODEL_TYPE=STRING        Fire Model Type (options: [HISTORICAL,ML], default: HISTORICAL)\n";
    std::cout << "  --INCIDENT_MODEL_TYPE=STRING    Incident Model Type (options: [EMPIRICAL], default: EMPIRICAL)\n";
    std::cout << "  --TRAVEL_TIME_MODEL_TYPE=STRING Travel Time Model Type (options: [OSRM,GIS], default: OSRM)\n";
    std::cout << "  --INCIDENTS_CSV_PATH=PATH       Path to incidents CSV file (default: ../data/incidents_5000.csv)\n";
    std::cout << "  --APPARATUS_CSV_PATH=PATH       Path to apparatus CSV file (default: ../data/stations_with_apparatus.csv)\n";
    std::cout << "  --BOUNDS_GEOJSON_PATH=PATH      Path to bounds GeoJSON file (default: ../data/bounds.geojson)\n";
    std::cout << "  --RANDOM_SEED=NUMBER            Random seed for simulation (default: 42)\n";
    std::cout << "  --PYTHON_PATH=PATH              Path to Python executable (default: ../../venvBOC/bin/python)\n";
    std::cout << "  --ENV_PATH=PATH                 Path to .env file. Overrides all other arguments.\n";
    std::cout << "  --CONSOLE_LOG_LEVEL=LEVEL       Set console log level (options: [trace, debug, info, warn, err, critical, off], default: debug)\n";
    std::cout << "  --help                          Show this help message\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program_name << " --INCIDENTS_CSV_PATH=../data/custom_incidents.csv --RANDOM_SEED=123\n";
    std::cout << "  " << program_name << " --ENV_PATH=../.env\n";
}

/**
 * Parse command line arguments and build JSON configuration string
 * Arguments should be in the format: --KEY=VALUE
 * Only provided arguments will override the defaults
 */
std::string parseArgumentsAndBuildConfig(int argc, char* argv[]) {
    // Default configuration values
    json config = {
        {"OSRM_URL", "http://localhost:8080/table/v1/driving/"},
        {"BASE_OSRM_URL", "http://localhost:8080"},
        {"DISPATCH_POLICY", "NEAREST"},
        {"FIRE_MODEL_TYPE", "DEPARTMENT"},
        {"INCIDENT_MODEL_TYPE", "EMPIRICAL"},
        {"TRAVEL_TIME_MODEL_TYPE", "OSRM"},
        {"INCIDENTS_CSV_PATH", "../data/incidents_5000.csv"},
        {"STATIONS_CSV_PATH", "../data/stations.csv"},
        {"APPARATUS_CSV_PATH", "../data/stations_with_apparatus.csv"},
        {"BOUNDS_GEOJSON_PATH", "../data/bounds.geojson"},
        {"NFD_RESPONSE_CSV_PATH", "../data/NFDResponse.csv"},
        {"RESOLUTION_STATS_CSV_PATH", "../data/response_time_summary.csv"},
        {"REPORT_CSV_PATH", "../logs/incident_report.csv"},
        {"STATION_REPORT_CSV_PATH", "../logs/station_report.csv"},
        {"DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"},
        {"DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"},
        {"MATRIX_CSV_PATH", "../logs/matrix.csv"},
        {"FIREBEATS_MATRIX_PATH", "../logs/beats.bin"},
        {"ZONE_MAP_PATH", "../data/zones.csv"},
        {"BEATS_SHAPEFILE_PATH", "../data/beats_shpfile.geojson"},
        {"RANDOM_SEED", 42},
        {"PYTHON_PATH", "../../venvBOC/bin/python"},
        {"CONSOLE_LOG_LEVEL", "debug"}
    };

    // Parse command line arguments starting from index 1 (skip program name)
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        // Handle help flag
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            exit(0);
        }
        
        // Skip the --run-python flag as it's handled separately
        if (arg == "--run-python") {
            continue;
        }
        
        // Look for arguments in format --KEY=VALUE
        if (arg.size() > 2 && arg.substr(0, 2) == "--") {
            size_t equals_pos = arg.find('=');
            if (equals_pos != std::string::npos) {
                std::string key = arg.substr(2, equals_pos - 2);  // Remove -- prefix
                std::string value = arg.substr(equals_pos + 1);
                
                // Check if this is a valid configuration key
                if (config.contains(key)) {
                    // Handle RANDOM_SEED as integer, others as string
                    if (key == "RANDOM_SEED") {
                        try {
                            config[key] = std::stoi(value);
                        } catch (const std::exception& e) {
                            std::cerr << "Warning: Invalid value for RANDOM_SEED: " << value 
                                      << ". Using default." << std::endl;
                        }
                    } else {
                        config[key] = value;
                    }
                } else {
                    std::cerr << "Warning: Unknown configuration key: " << key << std::endl;
                }
            } else {
                std::cerr << "Warning: Invalid argument format: " << arg 
                          << ". Expected --KEY=VALUE" << std::endl;
            }
        }
    }
    
    return config.dump();
}

int main(int argc, char* argv[]) {
    // ###### ACTUAL CODE ######
    if (argc == 1) {
        argc = 2;
        argv[1] = (char*)"--ENV_PATH=../.env";
        // printUsage(argv[0]);
        // return 0;
    }

    std::string arg(argv[1]);
    if (argc == 2 && arg.find("ENV_PATH") != std::string::npos) {
        std::cout << "Using env file from: " <<  argv[1] << std::endl;
        std::string env_path_arg = argv[1];
        std::string env_path = env_path_arg.substr(std::string("--ENV_PATH=").size());
        EnvLoader::init(env_path, "file");
    } else {
        // Parse command line arguments and build configuration
        std::string json_config = parseArgumentsAndBuildConfig(argc, argv);
        EnvLoader::init(json_config, "json");
    }

    std::shared_ptr<EnvLoader> env = EnvLoader::getInstance();

    // Initialize logger (this needs the env, might need to update)
    utils::Logger::init("fire_simulator");

    LOG_INFO("Starting Fire Simulator...");

    // Additional logic can be added here
    std::string incidents_path = env->get(constants::INCIDENTS_CSV_PATH, "../data/incidents.csv");

    std::vector<Incident> incidents = {};
    std::vector<FireStation> stations = {};
    std::vector<Vehicle> vehicles = {};
    loader::preComputingMatrices(stations, incidents, vehicles);

    #ifdef HAVE_SPDLOG_STOPWATCH
    spdlog::stopwatch sw;
    #endif

    EventQueue events = loader::generateEvents(incidents);

    State initial_state;
    initial_state.addStations(stations);
    initial_state.setVehicleList(vehicles);

    std::string policy_name = env->get(constants::POLICY_DISPATCH, "NEAREST");
    std::string fire_model_type = env->get(constants::POLICY_FIRE_MODEL, "HISTORICAL");
    std::string incident_model_type = env->get(constants::POLICY_INCIDENT_MODEL, "EMPIRICAL");
    std::string travel_time_model_type = env->get(constants::POLICY_TRAVEL_TIME_MODEL, "OSRM");

    std::unique_ptr<DispatchPolicy> policy;
    std::unique_ptr<ServiceTimeAndApparatusModel> fireModel;
    std::unique_ptr<IncidentModel> incidentModel;
    std::unique_ptr<TravelTimeModel> travelTimeModel;

    int seed = std::stoi(env->get(constants::RANDOM_SEED, "42"));
    std::string nfd_path = env->get(constants::NFD_RESPONSE_CSV_PATH, "");
    if (fire_model_type == "HISTORICAL") {
        LOG_INFO("Using Historical Fire Model.");
        std::string resolution_stats_path = env->get(constants::RESOLUTION_STATS_CSV_PATH, "../data/response_time_summary.csv");
        fireModel = std::make_unique<HistoricalFireModel>(seed, nfd_path, resolution_stats_path);
    } else if (fire_model_type == "ML") {
        LOG_INFO("Using ML Fire Model.");
        std::string model_path = env->get(constants::MODEL_PATH, "../models/fire_incident_gb_model.onnx");
        std::string features_path = env->get(constants::FEATURES_PATH, "../models/fire_model_features_mapping.json");
        fireModel = std::make_unique<MLFireModel>(seed, model_path, features_path, nfd_path);
    } else {
        throw std::runtime_error("Only HISTORICAL or ML fire model supported");
    }

    if (travel_time_model_type == constants::POLICY_OSRM) {
        LOG_INFO("Using OSRM Travel Time Model.");
        travelTimeModel = std::make_unique<OSRMTravelTimeModel>(
            env->get("BASE_OSRM_URL", "http://localhost:8080")
        );
    } else if (travel_time_model_type == constants::POLICY_GIS) {
        LOG_INFO("Using GIS Travel Time Model.");
        // travelTimeModel = std::make_unique<GISTravelTimeModel>();
    } else {
        throw std::runtime_error("Only OSRM or GIS travel time model supported");
    }

    if (incident_model_type == constants::POLICY_EMPIRICAL) {
        LOG_INFO("Using Empirical Incident Model.");
        incidentModel = std::make_unique<EmpiricalIncidentModel>(*fireModel);
        // HACK: Last incident that gets reported the next day
        Incident extra_incident = incidents.back();
        extra_incident.reportTime += 86400; // Add one day in seconds
        extra_incident.incidentIndex += 1;
        extra_incident.incident_id += 1;
        incidents.push_back(extra_incident);
        incidentModel->load(incidents);
    } else {
        throw std::runtime_error("Only EMPIRICAL incident model supported");
    }

    // Dispatch policy needs the fire stations and travel time model
    if (policy_name == constants::POLICY_NEAREST) {
        LOG_INFO("Using {} dispatching policy", policy_name);
        policy = std::make_unique<NearestDispatch>(stations, *travelTimeModel);
    } else if (policy_name == constants::POLICY_FIREBEATS) {
        LOG_INFO("Using {} dispatching policy", policy_name);
        policy = std::make_unique<FireBeatsDispatch>(
            *travelTimeModel,
            env->get(constants::FIREBEATS_MATRIX_PATH, "../logs/firebeats_matrix.bin"),
            env->get(constants::ZONE_MAP_PATH, "../data/zones.csv"),
            stations
        );
    } else {
        throw std::runtime_error("Only FIREBEATS or NEAREST policy supported");
    }

    EnvironmentModel environment_model(*fireModel);
    Simulator simulator(initial_state, *incidentModel, *travelTimeModel, environment_model, *policy);
    initial_state = simulator.reset();

    int num_steps = incidents.size();
    for (int step = 0; step < num_steps; ++step) {
        std::vector<Action> actions = policy->getAction(initial_state);
        StepResult result = simulator.step(actions);
        initial_state = result.state;
        if (result.done) {
            break;
        }
    }
    
    simulator.writeIncidentReport();
    simulator.writeActionReport(initial_state);
    // Too much data
    // simulator.writeVehicleReport();
    
    #ifdef HAVE_SPDLOG_STOPWATCH
    LOG_ERROR("Simulation completed successfully in {:.3} s.", sw);
    #endif
    
    // No need to delete fireModel, unique_ptr handles it automatically

    // Call Python script after simulation finishes
    if (argc > 1 && std::string(argv[1]) == "--run-python") {
        std::string python_path = env->get("PYTHON_PATH", "/opt/homebrew/bin/python3");

        int status = std::system((python_path + " ../scripts/process_csv.py").c_str());

        if (status == 0) {
            std::cout << "GeoJSON generated successfully." << std::endl;
        } else {
            std::cerr << "Failed to run Python script." << std::endl;
        }

        return 0;
    } else {
        LOG_INFO("Skipping CSV generation as per command line argument.");
        return 0;
    }
}
