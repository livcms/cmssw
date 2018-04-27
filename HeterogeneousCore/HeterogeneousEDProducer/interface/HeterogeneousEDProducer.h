#ifndef HeterogeneousCore_HeterogneousEDProducer_HeterogeneousEDProducer_h
#define HeterogeneousCore_HeterogneousEDProducer_HeterogeneousEDProducer_h

#include "FWCore/Concurrency/interface/WaitingTaskWithArenaHolder.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "DataFormats/Common/interface/Handle.h"

#include "HeterogeneousCore/Product/interface/HeterogeneousProduct.h"
#include "HeterogeneousCore/HeterogeneousEDProducer/interface/HeterogeneousEvent.h"

#include <cuda/api_wrappers.h> // TODO: we need to split this file to minimize unnecessary dependencies

namespace heterogeneous {
  template <typename T> struct Mapping;
}
#define MAKE_MAPPING(DEVICE, ENUM) \
  template <> \
  struct Mapping<DEVICE> { \
    template <typename ...Args> \
    static void beginStream(DEVICE& algo, Args&&... args) { algo.call_beginStream##DEVICE(std::forward<Args>(args)...); } \
    template <typename ...Args> \
    static bool acquire(DEVICE& algo, Args&&... args) { return algo.call_acquire##DEVICE(std::forward<Args>(args)...); } \
    template <typename ...Args> \
    static void produce(DEVICE& algo, Args&&... args) { algo.call_produce##DEVICE(std::forward<Args>(args)...); } \
    static constexpr HeterogeneousDevice deviceEnum = ENUM; \
  }


namespace heterogeneous {
  class CPU {
  public:
    void call_beginStreamCPU(edm::StreamID id) {
      beginStreamCPU(id);
    }
    bool call_acquireCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, edm::WaitingTaskWithArenaHolder waitingTaskHolder);
    void call_produceCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) {
      produceCPU(iEvent, iSetup);
    }

  private:
    virtual void beginStreamCPU(edm::StreamID id) {};
    virtual void acquireCPU(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) = 0;
    virtual void produceCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) = 0;
  };
  MAKE_MAPPING(CPU, HeterogeneousDevice::kCPU);

  class GPUMock {
  public:
    void call_beginStreamGPUMock(edm::StreamID id) {
      beginStreamGPUMock(id);
    }
    bool call_acquireGPUMock(DeviceBitSet inputLocation, edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, edm::WaitingTaskWithArenaHolder waitingTaskHolder);
    void call_produceGPUMock(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) {
      produceGPUMock(iEvent, iSetup);
    }

  private:
    virtual void beginStreamGPUMock(edm::StreamID id) {};
    virtual void acquireGPUMock(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, std::function<void()> callback) = 0;
    virtual void produceGPUMock(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) = 0;
  };
  MAKE_MAPPING(GPUMock, HeterogeneousDevice::kGPUMock);

  class GPUCuda {
  public:
    using CallbackType = std::function<void(cuda::device::id_t, cuda::stream::id_t, cuda::status_t)>;

    void call_beginStreamGPUCuda(edm::StreamID id);
    bool call_acquireGPUCuda(DeviceBitSet inputLocation, edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, edm::WaitingTaskWithArenaHolder waitingTaskHolder);
    void call_produceGPUCuda(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup);

  private:
    virtual void beginStreamGPUCuda(edm::StreamID id) {};
    virtual void acquireGPUCuda(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, CallbackType callback) = 0;
    virtual void produceGPUCuda(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) = 0;
  };
  MAKE_MAPPING(GPUCuda, HeterogeneousDevice::kGPUCuda);
}

#undef MAKE_MAPPING

namespace heterogeneous {
  ////////////////////
  template <typename ...Args>
  struct CallBeginStream;
  template <typename T, typename D, typename ...Devices>
  struct CallBeginStream<T, D, Devices...> {
    template <typename ...Args>
    static void call(T& ref, Args&&... args) {
      // may not perfect-forward here in order to be able to forward arguments to next CallBeginStream.
      Mapping<D>::beginStream(ref, args...);
      CallBeginStream<T, Devices...>::call(ref, std::forward<Args>(args)...);
    }
  };
  // break recursion and require CPU to be the last
  template <typename T>
  struct CallBeginStream<T, CPU> {
    template <typename ...Args>
    static void call(T& ref, Args&&... args) {
      Mapping<CPU>::beginStream(ref, std::forward<Args>(args)...);
    }
  };

  ////////////////////
  template <typename ...Args>
  struct CallAcquire;
  template <typename T, typename D, typename ...Devices>
  struct CallAcquire<T, D, Devices...> {
    template <typename ...Args>
    static void call(T& ref, const HeterogeneousProductBase *input, Args&&... args) {
      bool succeeded = true;
      DeviceBitSet inputLocation;
      if(input) {
        succeeded = input->isProductOn(Mapping<D>::deviceEnum);
        if(succeeded) {
          inputLocation = input->onDevices(Mapping<D>::deviceEnum);
        }
      }
      if(succeeded) {
        // may not perfect-forward here in order to be able to forward arguments to next CallAcquire.
        succeeded = Mapping<D>::acquire(ref, inputLocation, args...);
      }
      if(!succeeded) {
        CallAcquire<T, Devices...>::call(ref, input, std::forward<Args>(args)...);
      }
    }
  };
  // break recursion and require CPU to be the last
  template <typename T>
  struct CallAcquire<T, CPU> {
    template <typename ...Args>
    static void call(T& ref, const HeterogeneousProductBase *input, Args&&... args) {
      Mapping<CPU>::acquire(ref, std::forward<Args>(args)...);
    }
  };

  ////////////////////
  template <typename ...Args>
  struct CallProduce;
  template <typename T, typename D, typename ...Devices>
  struct CallProduce<T, D, Devices...> {
    template <typename ...Args>
    static void call(T& ref, edm::HeterogeneousEvent& iEvent, Args&&... args) {
      if(iEvent.location().deviceType() == Mapping<D>::deviceEnum) {
        Mapping<D>::produce(ref, iEvent, std::forward<Args>(args)...);
      }
      else {
        CallProduce<T, Devices...>::call(ref, iEvent, std::forward<Args>(args)...);
      }
    }
  };
  template <typename T>
  struct CallProduce<T> {
    template <typename ...Args>
    static void call(T& ref, Args&&... args) {}
  };


  template <typename ...Devices>
  class HeterogeneousDevices: public Devices... {
  public:
    void call_beginStream(edm::StreamID id) {
      CallBeginStream<HeterogeneousDevices, Devices...>::call(*this, id);
    }

    void call_acquire(const HeterogeneousProductBase *input,
                      edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup,
                      edm::WaitingTaskWithArenaHolder waitingTaskHolder) {
      CallAcquire<HeterogeneousDevices, Devices...>::call(*this, input, iEvent, iSetup, std::move(waitingTaskHolder));
    }

    void call_produce(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) {
      CallProduce<HeterogeneousDevices, Devices...>::call(*this, iEvent, iSetup);
    }
  };
} // end namespace heterogeneous


template <typename Devices, typename ...Capabilities>
class HeterogeneousEDProducer: public Devices, public edm::stream::EDProducer<edm::ExternalWork, Capabilities...> {
public:
  HeterogeneousEDProducer() {}
  ~HeterogeneousEDProducer() = default;

protected:
  edm::EDGetTokenT<HeterogeneousProduct> consumesHeterogeneous(const edm::InputTag& tag) {
    tokens_.push_back(this->template consumes<HeterogeneousProduct>(tag));
    return tokens_.back();
  }

private:
  void beginStream(edm::StreamID id) {
    Devices::call_beginStream(id);
  }

  void acquire(const edm::Event& iEvent, const edm::EventSetup& iSetup, edm::WaitingTaskWithArenaHolder waitingTaskHolder) override final {
    const HeterogeneousProductBase *input = nullptr;

    std::vector<const HeterogeneousProduct *> products;
    for(const auto& token: tokens_) {
      edm::Handle<HeterogeneousProduct> handle;
      iEvent.getByToken(token, handle);
      if(handle.isValid()) {
        // let the user acquire() code to deal with missing products
        // (and hope they don't mess up the scheduling!)
        products.push_back(handle.product());
      }
    }
    if(!products.empty()) {
      // TODO: check all inputs, not just the first one
      input = products[0]->getBase();
    }

    auto eventWrapper = edm::HeterogeneousEvent(&iEvent, &algoExecutionLocation_);
    Devices::call_acquire(input, eventWrapper, iSetup, std::move(waitingTaskHolder));
  }

  void produce(edm::Event& iEvent, const edm::EventSetup& iSetup) override final {
    if(algoExecutionLocation_.deviceType() == HeterogeneousDeviceId::kInvalidDevice) {
      // TODO: eventually fall back to CPU
      throw cms::Exception("LogicError") << "Trying to produce(), but algorithm was not executed successfully anywhere?";
    }
    auto eventWrapper = edm::HeterogeneousEvent(&iEvent, &algoExecutionLocation_);
    Devices::call_produce(eventWrapper, iSetup);
  }

  std::vector<edm::EDGetTokenT<HeterogeneousProduct> > tokens_;
  HeterogeneousDeviceId algoExecutionLocation_;
};

#endif



