#ifndef HeterogeneousCore_CUDACore_TestCUDAProducerGPUKernel_h
#define HeterogeneousCore_CUDACore_TestCUDAProducerGPUKernel_h

#include <cuda/api_wrappers.h>

class TestCUDAProducerGPUKernel {
public:
  TestCUDAProducerGPUKernel();
  ~TestCUDAProducerGPUKernel() = default;

  // returns (non-owning) pointer to device memory
  float *runAlgo(const std::string& label, cuda::stream_t<>& stream) {
    return runAlgo(label, nullptr, stream);
  }
  float *runAlgo(const std::string& label, const float *d_input, cuda::stream_t<>& stream);

private:
  // stored for the job duration
  cuda::memory::host::unique_ptr<float[]> h_a;
  cuda::memory::host::unique_ptr<float[]> h_b;
  cuda::memory::device::unique_ptr<float[]> d_a;
  cuda::memory::device::unique_ptr<float[]> d_b;
  cuda::memory::device::unique_ptr<float[]> d_c;
  cuda::memory::device::unique_ptr<float[]> d_ma;
  cuda::memory::device::unique_ptr<float[]> d_mb;
  cuda::memory::device::unique_ptr<float[]> d_mc;

  // temporary storage, need to be somewhere to allow async execution
  cuda::memory::device::unique_ptr<float[]> d_d;
};

#endif
