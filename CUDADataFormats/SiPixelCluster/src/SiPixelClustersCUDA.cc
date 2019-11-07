#include "CUDADataFormats/SiPixelCluster/interface/SiPixelClustersCUDA.h"

#include "HeterogeneousCore/CUDAUtilities/interface/device_unique_ptr.h"
#include "HeterogeneousCore/CUDAUtilities/interface/host_unique_ptr.h"
#include "HeterogeneousCore/CUDAUtilities/interface/copyAsync.h"

SiPixelClustersCUDA::SiPixelClustersCUDA(size_t maxClusters, cuda::stream_t<>& stream) {
  moduleStart_d = cudautils::make_device_unique<uint32_t[]>(maxClusters + 1);
  clusInModule_d = cudautils::make_device_unique<uint32_t[]>(maxClusters);
  moduleId_d = cudautils::make_device_unique<uint32_t[]>(maxClusters);
  clusModuleStart_d = cudautils::make_device_unique<uint32_t[]>(maxClusters + 1);

  // device-side ownership to guarantee that the host memory is alive
  // until the copy finishes
  auto view = cudautils::make_host_unique<DeviceConstView>(stream);
  view->moduleStart_ = moduleStart_d.get();
  view->clusInModule_ = clusInModule_d.get();
  view->moduleId_ = moduleId_d.get();
  view->clusModuleStart_ = clusModuleStart_d.get();

  view_d = cudautils::make_device_unique<DeviceConstView>();
  cudautils::copyAsync(view_d, view, stream);
}
