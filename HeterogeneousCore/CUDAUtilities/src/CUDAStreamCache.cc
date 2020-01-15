#include "FWCore/Utilities/interface/thread_safety_macros.h"
#include "HeterogeneousCore/CUDAUtilities/interface/CUDAStreamCache.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"
#include "HeterogeneousCore/CUDAUtilities/interface/currentDevice.h"
#include "HeterogeneousCore/CUDAUtilities/interface/deviceCount.h"
#include "HeterogeneousCore/CUDAUtilities/interface/ScopedSetDevice.h"

namespace cudautils {
  void CUDAStreamCache::Deleter::operator()(cudaStream_t stream) const {
    if (device_ != -1) {
      ScopedSetDevice deviceGuard{device_};
      cudaCheck(cudaStreamDestroy(stream));
    }
  }

  // CUDAStreamCache should be constructed by the first call to
  // getCUDAStreamCache() only if we have CUDA devices present
  CUDAStreamCache::CUDAStreamCache() : cache_(cudautils::deviceCount()) {}

  SharedStreamPtr CUDAStreamCache::getCUDAStream() {
    const auto dev = cudautils::currentDevice();
    return cache_[dev].makeOrGet([dev]() {
      cudaStream_t stream;
      cudaCheck(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));
      return std::unique_ptr<BareStream, Deleter>(stream, Deleter{dev});
    });
  }

  void CUDAStreamCache::clear() {
    // Reset the contents of the caches, but leave an
    // edm::ReusableObjectHolder alive for each device. This is needed
    // mostly for the unit tests, where the function-static
    // CUDAStreamCache lives through multiple tests (and go through
    // multiple shutdowns of the framework).
    cache_.clear();
    cache_.resize(cudautils::deviceCount());
  }

  CUDAStreamCache& getCUDAStreamCache() {
    // the public interface is thread safe
    CMS_THREAD_SAFE static CUDAStreamCache cache;
    return cache;
  }
}  // namespace cudautils
