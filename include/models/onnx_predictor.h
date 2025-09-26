// include/models/onnx_predictor.h
#ifndef ONNX_PREDICTOR_H
#define ONNX_PREDICTOR_H

#include <onnxruntime/onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <memory>

class ONNXPredictor {
public:
    ONNXPredictor();
    ~ONNXPredictor();
    
    bool loadModel(const std::string& model_path);
    float predict(const std::vector<float>& features);
    
private:
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> session_options_;
    
    std::vector<const char*> input_names_;
    std::vector<const char*> output_names_;
    std::vector<int64_t> input_dims_;
};

#endif // ONNX_PREDICTOR_H