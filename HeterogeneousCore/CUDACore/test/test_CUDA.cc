#include "catch.hpp"

#include "HeterogeneousCore/CUDACore/interface/CUDA.h"
#include "HeterogeneousCore/CUDACore/interface/CUDAToken.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

#include "TestCUDA.h"

#include <cuda_runtime_api.h>

TEST_CASE("Use of CUDA template", "[CUDACore]") {
  SECTION("Default constructed") {
    auto foo = CUDA<int>();
    REQUIRE(!foo.isValid());

    auto bar = std::move(foo);
  }

  int deviceCount = 0;
  auto ret = cudaGetDeviceCount( &deviceCount );
  if( ret != cudaSuccess ) {
    WARN("Unable to query the CUDA capable devices from the CUDA runtime API: ("
         << ret << ") " << cudaGetErrorString( ret ) 
         << ". Ignoring tests requiring device to be present.");
    return;
  }

  constexpr int defaultDevice = 0;
  {
    auto token = CUDAToken(defaultDevice);
    CUDA<int> data = TestCUDA::create(10, token);

    SECTION("Construct from CUDAToken") {
      REQUIRE(data.isValid());
      REQUIRE(data.device() == defaultDevice);
      REQUIRE(data.stream().id() == token.stream().id());
      REQUIRE(&data.event() != nullptr);
    }

    SECTION("Move constructor") {
      auto data2 = CUDA<int>(std::move(data));
      REQUIRE(data2.isValid());
      REQUIRE(!data.isValid());
    }

    SECTION("Move assignment") {
      CUDA<int> data2;
      data2 = std::move(data);
      REQUIRE(data2.isValid());
      REQUIRE(!data.isValid());
    }
  }

  // Destroy and clean up all resources so that the next test can
  // assume to start from a clean state.
  cudaCheck(cudaSetDevice(defaultDevice));
  cudaCheck(cudaDeviceSynchronize());
  cudaDeviceReset();
}
