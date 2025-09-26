#include <onnxruntime/onnxruntime_cxx_api.h>
#include <iostream>

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