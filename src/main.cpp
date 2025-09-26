#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#include "policy/firebeats_dispatch.h"
#ifdef HAVE_SPDLOG_STOPWATCH
#include "spdlog/stopwatch.h"
#endif
#include "utils/constants.h"
#include "utils/logger.h"
#include "utils/loaders.h"
#include "services/chunks.h"
#include "data/location.h"
#include "models/processors.h"

#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * Print usage information
 */
void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n";
    std::cout << "\nOPTIONS:\n";
    std::cout << "  --run-python                    Run Python post-processing script after simulation\n";
    std::cout << "  --OSRM_URL=URL                  OSRM table API URL (default: http://localhost:8080/table/v1/driving/)\n";
    std::cout << "  --BASE_OSRM_URL=URL             Base OSRM URL (default: http://localhost:8080)\n";
    std::cout << "  --DISPATCH_POLICY=STRING        Dispatch Policy (options: NEAREST/FIREBEATS, default: NEAREST)\n";
    std::cout << "  --INCIDENTS_CSV_PATH=PATH       Path to incidents CSV file (default: ../data/incidents_5000.csv)\n";
    std::cout << "  --STATIONS_CSV_PATH=PATH        Path to stations CSV file (default: ../data/stations.csv)\n";
    std::cout << "  --APPARATUS_CSV_PATH=PATH       Path to apparatus CSV file (default: ../data/stations_with_apparatus.csv)\n";
    std::cout << "  --BOUNDS_GEOJSON_PATH=PATH      Path to bounds GeoJSON file (default: ../data/bounds.geojson)\n";
    std::cout << "  --RANDOM_SEED=NUMBER            Random seed for simulation (default: 42)\n";
    std::cout << "  --PYTHON_PATH=PATH              Path to Python executable (default: ../../venvBOC/bin/python)\n";
    std::cout << "  --ENV_PATH=PATH                 Path to .env file. Overrides all other arguments.\n";
    std::cout << "  --help                          Show this help message\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program_name << " --INCIDENTS_CSV_PATH=../data/custom_incidents.csv --RANDOM_SEED=123\n";
    std::cout << "  " << program_name << " --ENV_PATH=../.env\n";
}

// Run on main to test Overpass
/* Should be run for each incident, find worst-case building nearby and use that as basis.
Overpass API Query: [out:json];(way(around:20, 36.145,-86.7933)["building"];node(around:20, 36.145,-86.7933)["building"];relation(around:20, 36.145,-86.7933)["building"];);out center;
Found 2 elements
Element 742727372 (way): Vanderbilt University : ISIS & ISDE | lat=36.144900, lon=-86.793202, building=office, levels=4, tourism=none
Element 1073870768 (way):  | lat=36.145171, lon=-86.793097, building=retail, levels=1, tourism=none
*/
void testingOverpass() {
    queryOverpassAPI(Location(36.144951, -86.793274), 20.0);
}

void testingONNX() {
    // --- initialize environment ---
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "infer");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    
    run_sk_model(env, session_options);
    run_torch_model(env, session_options);
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
        {"PYTHON_PATH", "../../venvBOC/bin/python"}
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
    // testingOverpass();
    // testingONNX();
    // return 0;
    if (argc == 1) {
        printUsage(argv[0]);
        return 0;
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
    utils::Logger::init("boilerplate_app");
    utils::Logger::setLevel("info");

    LOG_INFO("Starting Fire Simulator...");

    // Additional logic can be added here
    std::string incidents_path = env->get("INCIDENTS_CSV_PATH", "../data/incidents.csv");
    std::string stations_path = env->get("STATIONS_CSV_PATH", "../data/stations.csv");

    std::vector<Incident> incidents = {};
    std::vector<Station> stations = {};
    std::vector<Apparatus> apparatuses = {};
    size_t chunk_size = 500;
    loader::preComputingMatrices(stations, incidents, apparatuses, chunk_size);

    #ifdef HAVE_SPDLOG_STOPWATCH
    spdlog::stopwatch sw;
    #endif

    EventQueue events = loader::generateEvents(incidents);

    State initial_state;
    initial_state.populateAllIncidents(incidents); // Populate all incidents in the state (only used for passing incidents cheaply)
    initial_state.advanceTime(events.top().event_time); // Set initial time to the first event's time
    initial_state.addStations(stations);
    initial_state.setApparatusList(apparatuses);
    initial_state.matchApparatusesWithStations(); // Match apparatuses with their respective stations

    std::string policy_name = env->get("DISPATCH_POLICY", "NEAREST");
    std::unique_ptr<DispatchPolicy> policy;
    if (policy_name == constants::POLICY_NEAREST) {
        LOG_INFO("Using {} dispatching policy", policy_name);
        policy = std::make_unique<NearestDispatch>(
            env->get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
            env->get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"));
    } else if (policy_name == constants::POLICY_FIREBEATS) {
        LOG_INFO("Using {} dispatching policy", policy_name);
        policy = std::make_unique<FireBeatsDispatch>(
            env->get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
            env->get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"),
            env->get("FIREBEATS_MATRIX_PATH", "../logs/firebeats_matrix.bin"),
            env->get("ZONE_MAP_PATH", "../data/zones.csv")
        );
    } else {
        throw std::runtime_error("Only FIREBEATS or NEAREST policy supported");
    }

    int seed = std::stoi(env->get("RANDOM_SEED", "42"));
    std::string nfd_path = env->get("NFD_RESPONSE_CSV_PATH", "");
    std::string resolution_stats_path = env->get("RESOLUTION_STATS_CSV_PATH", "../data/response_time_summary.csv");
    std::unique_ptr<DepartmentFireModel> fireModel = std::make_unique<DepartmentFireModel>(seed, nfd_path, resolution_stats_path);

    std::string model_path = env->get("MODEL_PATH", "../models/gradient_boost_fire_model.onnx");
    std::string features_path = env->get("FEATURES_PATH", "../models/fire_model_features_mapping.json");

    FireModel* fireModel = new MLFireModel(seed, model_path, features_path, nfd_path);
    EnvironmentModel environment_model(*fireModel);
    Simulator simulator(initial_state, events, environment_model, *policy);
    simulator.run();

    #ifdef HAVE_SPDLOG_STOPWATCH
    LOG_ERROR("Simulation completed successfully in {:.3} s.", sw);
    #endif

    simulator.writeReportToCSV();
    simulator.writeActions();
    
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
