#include "models/onnx_predictor.h"
#include "utils/logger.h"
#include <cstring>

ONNXPredictor::ONNXPredictor() {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FireSimulator");
    session_options_ = std::make_unique<Ort::SessionOptions>();
    session_options_->SetIntraOpNumThreads(1);
}

ONNXPredictor::~ONNXPredictor() {
    // Clean up allocated strings
    for (const char* name : input_names_) {
        delete[] name;
    }
    for (const char* name : output_names_) {
        delete[] name;
    }
}

bool ONNXPredictor::loadModel(const std::string& model_path) {
    try {
        session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(), *session_options_);
        
        // Get input info
        size_t num_input_nodes = session_->GetInputCount();
        if (num_input_nodes > 0) {
            auto input_name = session_->GetInputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
            // Store a copy of the string to prevent memory issues
            std::string input_name_str(input_name.get());
            char* input_name_copy = new char[input_name_str.length() + 1];
            std::strcpy(input_name_copy, input_name_str.c_str());
            input_names_.push_back(input_name_copy);
            
            auto input_type_info = session_->GetInputTypeInfo(0);
            auto tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
            input_dims_ = tensor_info.GetShape();
        }
        
        // Get output info
        size_t num_output_nodes = session_->GetOutputCount();
        if (num_output_nodes > 0) {
            auto output_name = session_->GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
            // Store a copy of the string to prevent memory issues
            std::string output_name_str(output_name.get());
            char* output_name_copy = new char[output_name_str.length() + 1];
            std::strcpy(output_name_copy, output_name_str.c_str());
            output_names_.push_back(output_name_copy);
        }
        
        LOG_INFO("Loaded ONNX model: {}", model_path);
        // LOG_INFO("Input dimensions: [{}]", fmt::join(input_dims_, ", "));
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load ONNX model {}: {}", model_path, e.what());
        return false;
    }
}

float ONNXPredictor::predict(const std::vector<float>& features) {
    try {
        // Prepare input tensor
        std::vector<int64_t> input_shape = {1, static_cast<int64_t>(features.size())};
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, 
            const_cast<float*>(features.data()), 
            features.size(), 
            input_shape.data(), 
            input_shape.size()
        );
        
        // Run inference
        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names_.data(),
            &input_tensor,
            1,
            output_names_.data(),
            1
        );
        
        // Extract result
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        return output_data[0];
        
    } catch (const std::exception& e) {
        LOG_ERROR("ONNX prediction failed: {}", e.what());
        return -1.0f;
    }
}