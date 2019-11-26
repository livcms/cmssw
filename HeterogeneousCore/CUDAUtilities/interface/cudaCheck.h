#ifndef HeterogeneousCore_CUDAUtilities_cudaCheck_h
#define HeterogeneousCore_CUDAUtilities_cudaCheck_h

// C++ standard headers
#include <iostream>
#include <sstream>
#include <stdexcept>

// CUDA headers
#include <cuda.h>
#include <cuda_runtime.h>

namespace {
  inline std::ostringstream formatCudaError(const char* file, int line, const char* cmd, const char* error, const char* message) {
    std::ostringstream out;
    out << "\n";
    out << file << ", line " << line << ":\n";
    out << "cudaCheck(" << cmd << ");\n";
    out << error << ": " << message << "\n";
    return out;
  }


  [[noreturn]] inline void abortOnCudaError(const char* file, int line, const char* cmd, const char* error, const char* message) {
    auto out = formatCudaError(file, line, cmd, error, message);
    throw std::runtime_error(out.str());
  }

  const char* cudaErrorName(CUresult result) {
    const char *error;
    cuGetErrorName(result, &error);
    return error;
  }

  const char *cudaErrorString(CUresult result) {
    const char* message;
    cuGetErrorString(result, &message);
    return message;
  }

  const char*cudaErrorName(cudaError_t result) {
    return cudaGetErrorName(result);
  }

  const char*cudaErrorString(cudaError_t result) {
    return cudaGetErrorString(result);
  }
}  // namespace

template <typename RESULT>
inline bool cudaCheck_(const char* file, int line, const char* cmd, RESULT result) {
  if (__builtin_expect(result == CUDA_SUCCESS, true))
    return true;

  const char* error = cudaErrorName(result);
  const char* message = cudaErrorString(result);
  abortOnCudaError(file, line, cmd, error, message);
  return false;
}

#define cudaCheck(ARG) (cudaCheck_(__FILE__, __LINE__, #ARG, (ARG)))


namespace {

  template <typename T>
  [[noreturn]] inline void abortOnCudaErrorVerbose(
      const char* file, int line, const char* cmd, const char* error, const char* message, T const& description) {
    auto out = formatCudaError(file, line, cmd, error, message);
    out << description << "\n";
    throw std::runtime_error(out.str());
  }

}  // namespace

template <typename T>
inline bool cudaCheckVerbose_(const char* file, int line, const char* cmd, CUresult result, T const& description) {
  if (__builtin_expect(result == CUDA_SUCCESS, true))
    return true;

  const char* error;
  const char* message;
  cuGetErrorName(result, &error);
  cuGetErrorString(result, &message);
  abortOnCudaErrorVerbose(file, line, cmd, error, message, description);
  return false;
}

template <typename T>
inline bool cudaCheckVerbose_(const char* file, int line, const char* cmd, cudaError_t result, T const& description) {
  if (__builtin_expect(result == cudaSuccess, true))
    return true;

  const char* error = cudaGetErrorName(result);
  const char* message = cudaGetErrorString(result);
  abortOnCudaErrorVerbose(file, line, cmd, error, message, description);
  return false;
}

#define cudaCheckVerbose(ARG, DESC) (cudaCheckVerbose_(__FILE__, __LINE__, #ARG, (ARG), DESC))

#endif  // HeterogeneousCore_CUDAUtilities_cudaCheck_h
