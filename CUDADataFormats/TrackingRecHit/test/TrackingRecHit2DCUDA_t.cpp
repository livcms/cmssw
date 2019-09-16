#include "CUDADataFormats/TrackingRecHit/interface/TrackingRecHit2DCUDA.h"
#include "HeterogeneousCore/CUDAUtilities/interface/copyAsync.h"
#include "HeterogeneousCore/CUDAUtilities/interface/exitSansCUDADevices.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

namespace testTrackingRecHit2D {

  void runKernels(TrackingRecHit2DSOAView* hits);

}

int main() {
  exitSansCUDADevices();

  cudaStream_t stream;
  cudaCheck(cudaStreamCreate(&stream));

  // innert scope to deallocate memory before destroyn the stream
  {
    auto nHits = 200;
    TrackingRecHit2DCUDA tkhit(nHits, nullptr, nullptr, stream);

    testTrackingRecHit2D::runKernels(tkhit.view());
  }

  cudaCheck(cudaStreamDestroy(stream));

  return 0;
}
