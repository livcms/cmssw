#ifndef CUDADataFormats_Common_CUDAProductBase_h
#define CUDADataFormats_Common_CUDAProductBase_h

#include <atomic>
#include <memory>

#include <cuda/api_wrappers.h>

/**
 * Base class for all instantiations of CUDA<T> to hold the
 * non-T-dependent members.
 */
class CUDAProductBase {
public:
  CUDAProductBase() = default; // Needed only for ROOT dictionary generation

  CUDAProductBase(CUDAProductBase&& other):
    stream_{std::move(other.stream_)},
    event_{std::move(other.event_)},
    mayReuseStream_{other.mayReuseStream_.load()},
    device_{other.device_}
  {}
  CUDAProductBase& operator=(CUDAProductBase&& other) {
    stream_ = std::move(other.stream_);
    event_ = std::move(other.event_);
    mayReuseStream_ = other.mayReuseStream_.load();
    device_ = other.device_;
    return *this;
  }

  bool isValid() const { return stream_.get() != nullptr; }
  bool isAvailable() const;

  int device() const { return device_; }

  const cuda::stream_t<>& stream() const { return *stream_; }
  cuda::stream_t<>& stream() { return *stream_; }

  const cuda::event_t *event() const { return event_.get(); }
  cuda::event_t *event() { return event_.get(); }

protected:
  explicit CUDAProductBase(int device, std::shared_ptr<cuda::stream_t<>> stream, std::shared_ptr<cuda::event_t> event);

private:
  friend class CUDAScopedContext;

  // Intended to be used only from CUDAScopedContext
  const std::shared_ptr<cuda::stream_t<>>& streamPtr() const { return stream_; }

  bool mayReuseStream() const {
    bool expected = true;
    bool changed = mayReuseStream_.compare_exchange_strong(expected, false);
    // If the current thread is the one flipping the flag, it may
    // reuse the stream.
    return changed;
  }

  // The cuda::stream_t is really shared among edm::Event products, so
  // using shared_ptr also here
  std::shared_ptr<cuda::stream_t<>> stream_; //!
  // shared_ptr because of caching in CUDAService
  std::shared_ptr<cuda::event_t> event_; //!

  // This flag tellswhether the CUDA stream may be reused by a
  // consumer or not. The goal is to have a "chain" of modules to
  // queue their work to the same stream.
  mutable std::atomic<bool> mayReuseStream_ = true; //!

  int device_ = -1; //!
};

#endif
