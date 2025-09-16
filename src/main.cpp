#include "simulator/simulator.h"
#include "policy/nearest_dispatch.h"
#include "policy/firebeats_dispatch.h"
#ifdef HAVE_SPDLOG_STOPWATCH
#include "spdlog/stopwatch.h"
#endif
#include "utils/logger.h"
#include "utils/loaders.h"
#include "services/chunks.h"
#include "data/location.h"
#include <onnxruntime/onnxruntime_cxx_api.h>


int run_sk_model(Ort::Env& env, Ort::SessionOptions& session_options) {
    Ort::Session session(env, "../data/sk_linear_regression.onnx", session_options);
    Ort::AllocatorWithDefaultOptions allocator;

    auto input_name = session.GetInputNameAllocated(0, allocator);
    auto output_name = session.GetOutputNameAllocated(0, allocator);
    auto input_shape = session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();

    std::cout << "Input name: " << input_name << std::endl;
    std::cout << "Input shape: ";
    for (auto dim : input_shape) std::cout << dim << " ";
    std::cout << std::endl;

    std::vector<float> input_tensor_values = {1.0f, 2.0f, 3.0f};
    std::vector<int64_t> input_dims = {1, 3};

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator,
        OrtMemType::OrtMemTypeDefault
    );

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, 
        input_tensor_values.data(), 
        input_tensor_values.size(), 
        input_dims.data(), 
        input_dims.size()
    );

    const char* input_names[] = {input_name.get()};
    const char* output_names[] = {output_name.get()};

    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

    float* floatarr = output_tensors.front().GetTensorMutableData<float>();
    std::cout << "Prediction: " << floatarr[0] << std::endl;

    return 0;
}

int run_torch_model(Ort::Env& env, Ort::SessionOptions& session_options) {
    // ----- 2. Load model -----
    Ort::Session session(env, "../data/torch_linear_regression.onnx", session_options);

    // ----- 3. Prepare input -----
    std::vector<float> input_tensor_values = {3.0f};  // Example: predict y for x=3
    std::vector<int64_t> input_shape = {1, 1};

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_tensor_values.data(),
        input_tensor_values.size(),
        input_shape.data(),
        input_shape.size()
    );

    // ----- 4. Run inference -----
    const char* input_names[] = {"input"};
    const char* output_names[] = {"output"};

    auto output_tensors = session.Run(
        Ort::RunOptions{nullptr},
        input_names,
        &input_tensor,
        1,
        output_names,
        1
    );

    // ----- 5. Extract result -----
    float* float_array = output_tensors.front().GetTensorMutableData<float>();
    std::cout << "Predicted y = " << float_array[0] << std::endl;

    return 0;
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

int main(int argc, char* argv[]) {
    // ###### ACTUAL CODE ######
    // testingOverpass();
    testingONNX();
    return 0;

    EnvLoader::init("../.env");
    std::shared_ptr<EnvLoader> env = EnvLoader::getInstance();
    // Initialize logger
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

    DispatchPolicy* policy = new NearestDispatch(env->get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
                                                 env->get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"));

    // DispatchPolicy* policy = new FireBeatsDispatch(
    //     env->get("DISTANCE_MATRIX_PATH", "../logs/distance_matrix.bin"),
    //     env->get("DURATION_MATRIX_PATH", "../logs/duration_matrix.bin"),
    //     env->get("FIREBEATS_MATRIX_PATH", "../logs/firebeats_matrix.bin"),
    //     env->get("ZONE_MAP_PATH", "../data/zones.csv")
    // );

    int seed = std::stoi(env->get("RANDOM_SEED", "42"));
    std::string nfd_path = env->get("NFD_RESPONSE_CSV_PATH", "");
    std::string resolution_stats_path = env->get("RESOLUTION_STATS_CSV_PATH", "../data/response_time_summary.csv");
    FireModel* fireModel = new DepartmentFireModel(seed, nfd_path, resolution_stats_path);

    EnvironmentModel environment_model(*fireModel);
    Simulator simulator(initial_state, events, environment_model, *policy);
    simulator.run();

    #ifdef HAVE_SPDLOG_STOPWATCH
    LOG_ERROR("Simulation completed successfully in {:.3} s.", sw);
    #endif

    simulator.writeReportToCSV();
    simulator.writeActions();
    delete policy;
    delete fireModel;

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
