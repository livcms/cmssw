#ifndef HeterogeneousCore_CUDACore_CUDAScopedContext_h
#define HeterogeneousCore_CUDACore_CUDAScopedContext_h

#include "FWCore/Concurrency/interface/WaitingTaskWithArenaHolder.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "CUDADataFormats/Common/interface/CUDAProduct.h"
#include "HeterogeneousCore/CUDACore/interface/CUDAContextState.h"

#include <cuda/api_wrappers.h>

#include <optional>

namespace cudatest {
  class TestCUDAScopedContext;
}

namespace impl {
  // This class is intended to be derived by other CUDAScopedContext*, not for general use
  class CUDAScopedContextBase {
  public:
    int device() const { return currentDevice_; }

    cuda::stream_t<>& stream() { return *stream_; }
    const cuda::stream_t<>& stream() const { return *stream_; }
    const std::shared_ptr<cuda::stream_t<>>& streamPtr() const { return stream_; }

    template <typename T>
    const T& get(const CUDAProduct<T>& data) {
      synchronizeStreams(data.device(), data.stream(), data.isAvailable(), data.event());
      return data.data_;
    }

  protected:
    explicit CUDAScopedContextBase(edm::StreamID streamID);

    explicit CUDAScopedContextBase(const CUDAProductBase& data);

    explicit CUDAScopedContextBase(int device, std::shared_ptr<cuda::stream_t<>> stream);

    std::shared_ptr<cuda::stream_t<>>& streamPtr() { return stream_; }

  private:
    void synchronizeStreams(int dataDevice,
                            const cuda::stream_t<>& dataStream,
                            bool available,
                            const cuda::event_t* dataEvent);

    int currentDevice_;
    cuda::device::current::scoped_override_t<> setDeviceForThisScope_;
    std::shared_ptr<cuda::stream_t<>> stream_;
  };

  class CUDAScopedContextGetterBase : public CUDAScopedContextBase {
  public:
    using CUDAScopedContextBase::get;

    template <typename T>
    const T& get(const edm::Event& iEvent, edm::EDGetTokenT<CUDAProduct<T>> token) {
      return get(iEvent.get(token));
    }

  protected:
    template <typename... Args>
    CUDAScopedContextGetterBase(Args&&... args) : CUDAScopedContextBase(std::forward<Args>(args)...) {}
  };

  class CUDAScopedContextHolderHelper {
  public:
    CUDAScopedContextHolderHelper(): hasHolder_{false} {}
    CUDAScopedContextHolderHelper(edm::WaitingTaskWithArenaHolder waitingTaskHolder)
        : waitingTaskHolder_{std::move(waitingTaskHolder)} {}

    template <typename F>
    void pushNextTask(F&& f, CUDAContextState const* state);

    void replaceWaitingTaskHolder(edm::WaitingTaskWithArenaHolder waitingTaskHolder) {
      waitingTaskHolder_ = std::move(waitingTaskHolder);
      hasHolder_ = true;
    }

    void enqueueCallback(int device, cuda::stream_t<>& stream);

  private:
    edm::WaitingTaskWithArenaHolder waitingTaskHolder_;
    bool hasHolder_ = true;
  };
}  // namespace impl

/**
 * The aim of this class is to do necessary per-event "initialization" in ExternalWork acquire():
 * - setting the current device
 * - calling edm::WaitingTaskWithArenaHolder::doneWaiting() when necessary
 * - synchronizing between CUDA streams if necessary
 * and enforce that those get done in a proper way in RAII fashion.
 */
class CUDAScopedContextAcquire : public impl::CUDAScopedContextGetterBase {
public:
  /// Constructor to create a new CUDA stream (no need for context beyond acquire())
  explicit CUDAScopedContextAcquire(edm::StreamID streamID, edm::WaitingTaskWithArenaHolder waitingTaskHolder)
      : CUDAScopedContextGetterBase(streamID), holderHelper_{std::move(waitingTaskHolder)} {}

  /// Constructor to create a new CUDA stream, and the context is needed after acquire()
  explicit CUDAScopedContextAcquire(edm::StreamID streamID,
                                    edm::WaitingTaskWithArenaHolder waitingTaskHolder,
                                    CUDAContextState& state)
      : CUDAScopedContextGetterBase(streamID), holderHelper_{std::move(waitingTaskHolder)}, contextState_{&state} {}

  /// Constructor to (possibly) re-use a CUDA stream (no need for context beyond acquire())
  explicit CUDAScopedContextAcquire(const CUDAProductBase& data, edm::WaitingTaskWithArenaHolder waitingTaskHolder)
      : CUDAScopedContextGetterBase(data), holderHelper_{std::move(waitingTaskHolder)} {}

  /// Constructor to (possibly) re-use a CUDA stream, and the context is needed after acquire()
  explicit CUDAScopedContextAcquire(const CUDAProductBase& data,
                                    edm::WaitingTaskWithArenaHolder waitingTaskHolder,
                                    CUDAContextState& state)
      : CUDAScopedContextGetterBase(data), holderHelper_{std::move(waitingTaskHolder)}, contextState_{&state} {}

  // I'm really unsure if these two overloads should be allowed in the end...
  explicit CUDAScopedContextAcquire(edm::StreamID streamID,
                                    CUDAContextState& state)
      : CUDAScopedContextGetterBase(streamID), contextState_{&state} {}
  explicit CUDAScopedContextAcquire(const CUDAProductBase& data,
                                    CUDAContextState& state)
      : CUDAScopedContextGetterBase(data), contextState_{&state} {}

  ~CUDAScopedContextAcquire();

  template <typename F>
  void pushNextTask(F&& f) {
    if (contextState_ == nullptr)
      throwNoState();
    holderHelper_.pushNextTask(std::forward<F>(f), contextState_);
  }

  void replaceWaitingTaskHolder(edm::WaitingTaskWithArenaHolder waitingTaskHolder) {
    holderHelper_.replaceWaitingTaskHolder(std::move(waitingTaskHolder));
  }

private:
  void throwNoState();

  impl::CUDAScopedContextHolderHelper holderHelper_;
  CUDAContextState* contextState_ = nullptr;
};

/**
 * The aim of this class is to do necessary per-event "initialization" in ExternalWork produce() or normal produce():
 * - setting the current device
 * - synchronizing between CUDA streams if necessary
 * and enforce that those get done in a proper way in RAII fashion.
 */
class CUDAScopedContextProduce : public impl::CUDAScopedContextGetterBase {
public:
  /// Constructor to create a new CUDA stream (non-ExternalWork module)
  explicit CUDAScopedContextProduce(edm::StreamID streamID) : CUDAScopedContextGetterBase(streamID) {}

  /// Constructor to (possibly) re-use a CUDA stream (non-ExternalWork module)
  explicit CUDAScopedContextProduce(const CUDAProductBase& data) : CUDAScopedContextGetterBase(data) {}

  /// Constructor to re-use the CUDA stream of acquire() (ExternalWork module)
  explicit CUDAScopedContextProduce(CUDAContextState& token)
      : CUDAScopedContextGetterBase(token.device(), std::move(token.streamPtr())) {}

  ~CUDAScopedContextProduce();

  template <typename T>
  std::unique_ptr<CUDAProduct<T>> wrap(T data) {
    // make_unique doesn't work because of private constructor
    //
    // CUDAProduct<T> constructor records CUDA event to the CUDA
    // stream. The event will become "occurred" after all work queued
    // to the stream before this point has been finished.
    std::unique_ptr<CUDAProduct<T>> ret(new CUDAProduct<T>(device(), streamPtr(), std::move(data)));
    createEventIfStreamBusy();
    ret->setEvent(event_);
    return ret;
  }

  template <typename T, typename... Args>
  auto emplace(edm::Event& iEvent, edm::EDPutTokenT<T> token, Args&&... args) {
    auto ret = iEvent.emplace(token, device(), streamPtr(), std::forward<Args>(args)...);
    createEventIfStreamBusy();
    const_cast<T&>(*ret).setEvent(event_);
    return ret;
  }

private:
  friend class cudatest::TestCUDAScopedContext;

  // This construcor is only meant for testing
  explicit CUDAScopedContextProduce(int device,
                                    std::unique_ptr<cuda::stream_t<>> stream,
                                    std::unique_ptr<cuda::event_t> event)
      : CUDAScopedContextGetterBase(device, std::move(stream)), event_{std::move(event)} {}

  void createEventIfStreamBusy();

  std::shared_ptr<cuda::event_t> event_;
};

/**
 * The aim of this class is to do necessary per-task "initialization" tasks created in ExternalWork acquire():
 * - setting the current device
 * - calling edm::WaitingTaskWithArenaHolder::doneWaiting() when necessary
 * and enforce that those get done in a proper way in RAII fashion.
 */
class CUDAScopedContextTask : public impl::CUDAScopedContextBase {
public:
  /// Constructor to re-use the CUDA stream of acquire() (ExternalWork module)
  explicit CUDAScopedContextTask(CUDAContextState const* state, edm::WaitingTaskWithArenaHolder waitingTaskHolder)
      : CUDAScopedContextBase(state->device(), state->streamPtr()),  // don't move, state is re-used afterwards
        holderHelper_{std::move(waitingTaskHolder)},
        contextState_{state} {}

  ~CUDAScopedContextTask();

  template <typename F>
  void pushNextTask(F&& f) {
    holderHelper_.pushNextTask(std::forward<F>(f), contextState_);
  }

  void replaceWaitingTaskHolder(edm::WaitingTaskWithArenaHolder waitingTaskHolder) {
    holderHelper_.replaceWaitingTaskHolder(std::move(waitingTaskHolder));
  }

private:
  impl::CUDAScopedContextHolderHelper holderHelper_;
  CUDAContextState const* contextState_;
};

/**
 * The aim of this class is to do necessary per-event "initialization" in analyze()
 * - setting the current device
 * - synchronizing between CUDA streams if necessary
 * and enforce that those get done in a proper way in RAII fashion.
 */
/**
 * The aim of this class is to do necessary per-event "initialization" in ExternalWork produce() or normal produce():
 * - setting the current device
 * - synchronizing between CUDA streams if necessary
 * and enforce that those get done in a proper way in RAII fashion.
 */
class CUDAScopedContextAnalyze : public impl::CUDAScopedContextGetterBase {
public:
  /// Constructor to (possibly) re-use a CUDA stream
  explicit CUDAScopedContextAnalyze(const CUDAProductBase& data) : CUDAScopedContextGetterBase(data) {}
};

namespace impl {
  template <typename F>
  void CUDAScopedContextHolderHelper::pushNextTask(F&& f, CUDAContextState const* state) {
    replaceWaitingTaskHolder(edm::WaitingTaskWithArenaHolder{
        edm::make_waiting_task_with_holder(tbb::task::allocate_root(),
                                           std::move(waitingTaskHolder_),
                                           [state, func = std::forward<F>(f)](edm::WaitingTaskWithArenaHolder h) {
                                             func(CUDAScopedContextTask{state, std::move(h)});
                                           })});
  }
}  // namespace impl

#endif
