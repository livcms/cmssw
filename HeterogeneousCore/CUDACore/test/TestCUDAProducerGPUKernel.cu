#include "TestCUDAProducerGPUKernel.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/Exception.h"

namespace {
  template<typename T>
  __global__
  void vectorAdd(const T *a, const T *b, T *c, int numElements) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if(i < numElements) { c[i] = a[i] + b[i]; }
  }

  template <typename T>
  __global__
  void vectorProd(const T *a, const T *b, T *c, int numElements) {
    int row = blockIdx.y*blockDim.y + threadIdx.y;
    int col = blockIdx.x*blockDim.x + threadIdx.x;

    if(row < numElements && col < numElements) {
      c[row*numElements + col] = a[row]*b[col];
    }
  }

  template <typename T>
  __global__
  void matrixMul(const T *a, const T *b, T *c, int numElements) {
    int row = blockIdx.y*blockDim.y + threadIdx.y;
    int col = blockIdx.x*blockDim.x + threadIdx.x;

    if(row < numElements && col < numElements) {
      T tmp = 0;
      for(int i=0; i<numElements; ++i) {
        tmp += a[row*numElements + i] * b[i*numElements + col];
      }
      c[row*numElements + col] = tmp;
    }
  }

  template <typename T>
  __global__
  void matrixMulVector(const T *a, const T *b, T *c, int numElements) {
    int row = blockIdx.y*blockDim.y + threadIdx.y;

    if(row < numElements) {
      T tmp = 0;
      for(int i=0; i<numElements; ++i) {
        tmp += a[row*numElements + i] * b[i];
      }
      c[row] = tmp;
    }
  }
}

namespace {
  constexpr int NUM_VALUES = 10000;
}

TestCUDAProducerGPUKernel::TestCUDAProducerGPUKernel() {
  h_a = cuda::memory::host::make_unique<float[]>(NUM_VALUES);
  h_b = cuda::memory::host::make_unique<float[]>(NUM_VALUES);

  auto current_device = cuda::device::current::get();
  d_a = cuda::memory::device::make_unique<float[]>(current_device, NUM_VALUES);
  d_b = cuda::memory::device::make_unique<float[]>(current_device, NUM_VALUES);
  d_c = cuda::memory::device::make_unique<float[]>(current_device, NUM_VALUES);

  d_ma = cuda::memory::device::make_unique<float[]>(current_device, NUM_VALUES*NUM_VALUES);
  d_mb = cuda::memory::device::make_unique<float[]>(current_device, NUM_VALUES*NUM_VALUES);
  d_mc = cuda::memory::device::make_unique<float[]>(current_device, NUM_VALUES*NUM_VALUES);
}

float *TestCUDAProducerGPUKernel::runAlgo(const std::string& label, const float *d_input, cuda::stream_t<>& stream) {
  // First make the sanity check
  if(d_input != nullptr) {
    auto h_check = std::make_unique<float[]>(NUM_VALUES);
    cuda::memory::copy(h_check.get(), d_input, NUM_VALUES*sizeof(float));
    for(int i=0; i<NUM_VALUES; ++i) {
      if(h_check[i] != i) {
        throw cms::Exception("Assert") << "Sanity check on element " << i << " failed, expected " << i << " got " << h_check[i];
      }
    }
  }

  for (auto i=0; i<NUM_VALUES; i++) {
    h_a[i] = i;
    h_b[i] = i*i;
  }

  cuda::memory::async::copy(d_a.get(), h_a.get(), NUM_VALUES*sizeof(float), stream.id());
  cuda::memory::async::copy(d_b.get(), h_b.get(), NUM_VALUES*sizeof(float), stream.id());

  int threadsPerBlock {32};
  int blocksPerGrid = (NUM_VALUES + threadsPerBlock - 1) / threadsPerBlock;

  auto current_device = cuda::device::current::get();
  edm::LogPrint("TestHeterogeneousEDProducerGPU") << "  " << label << " GPU launching kernels device " << current_device.id() << " CUDA stream " << stream.id();
  vectorAdd<<<blocksPerGrid, threadsPerBlock, 0, stream.id()>>>(d_a.get(), d_b.get(), d_c.get(), NUM_VALUES);

  dim3 threadsPerBlock3{NUM_VALUES, NUM_VALUES};
  dim3 blocksPerGrid3{1,1};
  if(NUM_VALUES*NUM_VALUES > 32) {
    threadsPerBlock3.x = 32;
    threadsPerBlock3.y = 32;
    blocksPerGrid3.x = ceil(double(NUM_VALUES)/double(threadsPerBlock3.x));
    blocksPerGrid3.y = ceil(double(NUM_VALUES)/double(threadsPerBlock3.y));
  }
  vectorProd<<<blocksPerGrid3, threadsPerBlock3, 0, stream.id()>>>(d_a.get(), d_b.get(), d_ma.get(), NUM_VALUES);
  vectorProd<<<blocksPerGrid3, threadsPerBlock3, 0, stream.id()>>>(d_a.get(), d_c.get(), d_mb.get(), NUM_VALUES);
  matrixMul<<<blocksPerGrid3, threadsPerBlock3, 0, stream.id()>>>(d_ma.get(), d_mb.get(), d_mc.get(), NUM_VALUES);

  matrixMulVector<<<blocksPerGrid, threadsPerBlock, 0, stream.id()>>>(d_mc.get(), d_b.get(), d_c.get(), NUM_VALUES);

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << "  " << label << " GPU kernels launched, returning return pointer device " << current_device.id() << " CUDA stream " << stream.id();
  return d_a.get();
}
