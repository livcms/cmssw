#ifndef CUDADataFormatsCommonHeterogeneousSoA_H
#define CUDADataFormatsCommonHeterogeneousSoA_H

#include "HeterogeneousCore/CUDAUtilities/interface/device_unique_ptr.h"
#include "HeterogeneousCore/CUDAUtilities/interface/host_unique_ptr.h"

#include "HeterogeneousCore/CUDAUtilities/interface/copyAsync.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

// a heterogeneous unique pointer...
template <typename T>
class HeterogeneousSoA {
public:
  using Product = T;

  HeterogeneousSoA() = default;  // make root happy
  ~HeterogeneousSoA() = default;
  HeterogeneousSoA(HeterogeneousSoA &&) = default;
  HeterogeneousSoA &operator=(HeterogeneousSoA &&) = default;

  explicit HeterogeneousSoA(cudautils::device::unique_ptr<T> &&p) : dm_ptr(std::move(p)) {}
  explicit HeterogeneousSoA(cudautils::host::unique_ptr<T> &&p) : hm_ptr(std::move(p)) {}
  explicit HeterogeneousSoA(std::unique_ptr<T> &&p) : std_ptr(std::move(p)) {}

  auto const *get() const { return dm_ptr ? dm_ptr.get() : (hm_ptr ? hm_ptr.get() : std_ptr.get()); }

  auto const &operator*() const { return *get(); }

  auto const *operator-> () const { return get(); }

  auto *get() { return dm_ptr ? dm_ptr.get() : (hm_ptr ? hm_ptr.get() : std_ptr.get()); }

  auto &operator*() { return *get(); }

  auto *operator-> () { return get(); }

  // in reality valid only for GPU version...
  cudautils::host::unique_ptr<T> toHostAsync(cuda::stream_t<> &stream) const {
    assert(dm_ptr);
    auto ret = cudautils::make_host_unique<T>(stream);
    cudaCheck(cudaMemcpyAsync(ret.get(), dm_ptr.get(), sizeof(T), cudaMemcpyDefault, stream.id()));
    return ret;
  }

private:
  // a union wan't do it, a variant will not be more efficienct
  cudautils::device::unique_ptr<T> dm_ptr;  //!
  cudautils::host::unique_ptr<T> hm_ptr;    //!
  std::unique_ptr<T> std_ptr;               //!
};

namespace cudaCompat {

  struct GPUTraits {
    template <typename T>
    using unique_ptr = cudautils::device::unique_ptr<T>;

    template <typename T, typename... Args>
    static auto make_unique(Args&&... args) {
      return cudautils::make_device_unique<T>(std::forward<Args>(args)...); 
    }

    template <typename T, typename... Args>
    static auto make_host_unique(Args&&... args) {
      return cudautils::make_host_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    static auto make_device_unique(Args&&... args) {
      return cudautils::make_device_unique<T>(std::forward<Args>(args)...);
    }
  };

  struct HostTraits {
    template <typename T>
    using unique_ptr = cudautils::host::unique_ptr<T>;

    template <typename T, typename... Args>
    static auto make_unique(Args&&... args) {
      return cudautils::make_host_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    static auto make_host_unique(Args&&... args) {
      return cudautils::make_host_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    static auto make_device_unique(Args&&... args) {
      return cudautils::make_device_unique<T>(std::forward<Args>(args)...);
    }
  };

  struct CPUTraits {
    template <typename T>
    using unique_ptr = std::unique_ptr<T>;

    template <typename T>
    static auto make_unique() {
      return std::make_unique<T>();
    }
    template <typename T>
    static auto make_unique(cuda::stream_t<> &) {
      return std::make_unique<T>();
    }

    template <typename T>
    static auto make_unique(size_t size) {
      return std::make_unique<T>(size);
    }
    template <typename T>
    static auto make_unique(size_t size, cuda::stream_t<> &) {
      return std::make_unique<T>(size);
    }

    template <typename T>
    static auto make_host_unique() {
      return std::make_unique<T>();
    }
    template <typename T>
    static auto make_host_unique(cuda::stream_t<> &) {
      return std::make_unique<T>();
    }

    template <typename T>
    static auto make_device_unique() {
      return std::make_unique<T>();
    }
    template <typename T>
    static auto make_device_unique(cuda::stream_t<> &) {
      return std::make_unique<T>();
    }

    template <typename T>
    static auto make_device_unique(size_t size) {
      return std::make_unique<T>(size);
    }
    template <typename T>
    static auto make_device_unique(size_t size, cuda::stream_t<> &) {
      return std::make_unique<T>(size);
    }
  };

}  // namespace cudaCompat

// a heterogeneous unique pointer (of a different sort) ...
template <typename T, typename Traits>
class HeterogeneousSoAImpl {
public:
  template <typename V>
  using unique_ptr = typename Traits::template unique_ptr<V>;

  HeterogeneousSoAImpl();
  ~HeterogeneousSoAImpl() = default;
  HeterogeneousSoAImpl(HeterogeneousSoAImpl &&) = default;
  HeterogeneousSoAImpl &operator=(HeterogeneousSoAImpl &&) = default;

  explicit HeterogeneousSoAImpl(unique_ptr<T> &&p) : m_ptr(std::move(p)) {}

  T const *get() const { return m_ptr.get(); }

  T *get() { return m_ptr.get(); }

  cudautils::host::unique_ptr<T> toHostAsync(cuda::stream_t<> &stream) const;

private:
  unique_ptr<T> m_ptr;  //!
};

template <typename T, typename Traits>
HeterogeneousSoAImpl<T, Traits>::HeterogeneousSoAImpl() {
  m_ptr = Traits::template make_unique<T>();
}

// in reality valid only for GPU version...
template <typename T, typename Traits>
cudautils::host::unique_ptr<T> HeterogeneousSoAImpl<T, Traits>::toHostAsync(cuda::stream_t<> &stream) const {
  auto ret = cudautils::make_host_unique<T>(stream);
  cudaCheck(cudaMemcpyAsync(ret.get(), get(), sizeof(T), cudaMemcpyDefault, stream.id()));
  return ret;
}

template <typename T>
using HeterogeneousSoAGPU = HeterogeneousSoAImpl<T, cudaCompat::GPUTraits>;
template <typename T>
using HeterogeneousSoACPU = HeterogeneousSoAImpl<T, cudaCompat::CPUTraits>;
template <typename T>
using HeterogeneousSoAHost = HeterogeneousSoAImpl<T, cudaCompat::HostTraits>;

#endif
