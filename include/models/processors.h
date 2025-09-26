#ifndef PROCESSORS_H
#define PROCESSORS_H

#include <onnxruntime/onnxruntime_cxx_api.h>

// Helpers to run small ONNX models used from `src/models/processors.cpp`.
// Each takes an ONNX Runtime environment and session options and returns 0 on success.
int run_rfsk_model(Ort::Env& env, Ort::SessionOptions& session_options);
int run_sk_model(Ort::Env& env, Ort::SessionOptions& session_options);
int run_torch_model(Ort::Env& env, Ort::SessionOptions& session_options);

#endif // PROCESSORS_H
