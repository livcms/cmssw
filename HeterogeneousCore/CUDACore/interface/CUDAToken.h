#ifndef HeterogeneousCore_CUDACore_CUDAToken_h
#define HeterogeneousCore_CUDACore_CUDAToken_h

#include <cuda/api_wrappers.h>

#include <memory>

/**
 * The purpose of this class is to deliver the device and CUDA stream
 * information from CUDADeviceChooser to the EDModules with CUDA
 * implementation.
 *
 * Currently the class is declared as transient in the dictionary, but
 * in principle (for debugging purposes) it could be possible to
 * persist it by marking only the CUDA stream as transient.
 */
class CUDAToken {
public:
  CUDAToken() = default;
  explicit CUDAToken(int device);

  int device() const { return device_; }
  const cuda::stream_t<>& stream() const { return *stream_; } // TODO: hmm, cuda::stream_t::synchronize() is non-const...
  
private:
  std::unique_ptr<cuda::stream_t<>> stream_;
  int device_ = -1;
};

#endif
