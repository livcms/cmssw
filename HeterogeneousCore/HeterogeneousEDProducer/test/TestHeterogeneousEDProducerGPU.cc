#include "HeterogeneousCore/HeterogeneousEDProducer/interface/HeterogeneousEDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "HeterogeneousCore/CUDAServices/interface/CUDAService.h"
#include "HeterogeneousCore/CUDACore/interface/GPUCuda.h"
#include "HeterogeneousCore/Product/interface/HeterogeneousProduct.h"

#include "TestHeterogeneousEDProducerGPUHelpers.h"

#include <chrono>
#include <random>
#include <thread>

#include <cuda.h>
#include <cuda_runtime.h>


class TestHeterogeneousEDProducerGPU: public HeterogeneousEDProducer<heterogeneous::HeterogeneousDevices <
                                                                       heterogeneous::GPUCuda,
                                                                       heterogeneous::CPU
                                                                       > > {
public:
  explicit TestHeterogeneousEDProducerGPU(edm::ParameterSet const& iConfig);
  ~TestHeterogeneousEDProducerGPU() = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  using OutputType = HeterogeneousProductImpl<heterogeneous::CPUProduct<unsigned int>,
                                              heterogeneous::GPUCudaProduct<TestHeterogeneousEDProducerGPUTask::ResultTypeRaw>>;

  void beginStreamGPUCuda(edm::StreamID streamId, cuda::stream_t<>& cudaStream) override;

  void acquireCPU(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) override;
  void acquireGPUCuda(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) override;

  void produceCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) override;
  void produceGPUCuda(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) override;

  std::string label_;
  edm::EDGetTokenT<HeterogeneousProduct> srcToken_;

  // GPU stuff
  std::unique_ptr<TestHeterogeneousEDProducerGPUTask> gpuAlgo_;
  TestHeterogeneousEDProducerGPUTask::ResultType gpuOutput_;

    // output
  unsigned int output_;
};

TestHeterogeneousEDProducerGPU::TestHeterogeneousEDProducerGPU(edm::ParameterSet const& iConfig):
  label_(iConfig.getParameter<std::string>("@module_label"))
{
  auto srcTag = iConfig.getParameter<edm::InputTag>("src");
  if(!srcTag.label().empty()) {
    srcToken_ = consumesHeterogeneous(srcTag);
  }

  produces<HeterogeneousProduct>();
}

void TestHeterogeneousEDProducerGPU::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("src", edm::InputTag());
  descriptions.add("testHeterogeneousEDProducerGPU2", desc);
}

void TestHeterogeneousEDProducerGPU::beginStreamGPUCuda(edm::StreamID streamId, cuda::stream_t<>& cudaStream) {
  edm::Service<CUDAService> cs;

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << " " << label_ << " TestHeterogeneousEDProducerGPU::beginStreamGPUCuda begin stream " << streamId << " device " << cs->getCurrentDevice();

  gpuAlgo_ = std::make_unique<TestHeterogeneousEDProducerGPUTask>();

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << " " << label_ << " TestHeterogeneousEDProducerGPU::beginStreamGPUCuda end stream " << streamId << " device " << cs->getCurrentDevice();
}

void TestHeterogeneousEDProducerGPU::acquireCPU(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) {
  edm::LogPrint("TestHeterogeneousEDProducerGPU") << " " << label_ << " TestHeterogeneousEDProducerGPU::acquireCPU begin event " << iEvent.id().event() << " stream " << iEvent.streamID();

  unsigned int input = 0;
  if(!srcToken_.isUninitialized()) {
    edm::Handle<unsigned int> hin;
    iEvent.getByToken<OutputType>(srcToken_, hin);
    input = *hin;
  }

  std::random_device r;
  std::mt19937 gen(r());
  auto dist = std::uniform_real_distribution<>(1.0, 3.0); 
  auto dur = dist(gen);
  edm::LogPrint("TestHeterogeneousEDProducerGPU") << "  Task (CPU) for event " << iEvent.id().event() << " in stream " << iEvent.streamID() << " will take " << dur << " seconds";
  std::this_thread::sleep_for(std::chrono::seconds(1)*dur);

  output_ = input + iEvent.streamID()*100 + iEvent.id().event();

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << " " << label_ << " TestHeterogeneousEDProducerGPU::acquireCPU end event " << iEvent.id().event() << " stream " << iEvent.streamID();
}

void TestHeterogeneousEDProducerGPU::acquireGPUCuda(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) {
  edm::Service<CUDAService> cs;
  edm::LogPrint("TestHeterogeneousEDProducerGPU") << " " << label_ << " TestHeterogeneousEDProducerGPU::acquireGPUCuda begin event " << iEvent.id().event() << " stream " << iEvent.streamID() << " device " << cs->getCurrentDevice();

  gpuOutput_.first.reset();
  gpuOutput_.second.reset();

  TestHeterogeneousEDProducerGPUTask::ResultTypeRaw input = std::make_pair(nullptr, nullptr);
  if(!srcToken_.isUninitialized()) {
    edm::Handle<TestHeterogeneousEDProducerGPUTask::ResultTypeRaw> hin;
    iEvent.getByToken<OutputType>(srcToken_, hin);
    input = *hin;
  }

  gpuOutput_ = gpuAlgo_->runAlgo(label_, 0, input, cudaStream);

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << " " << label_ << " TestHeterogeneousEDProducerGPU::acquireGPUCuda end event " << iEvent.id().event() << " stream " << iEvent.streamID() << " device " << cs->getCurrentDevice();
}

void TestHeterogeneousEDProducerGPU::produceCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) {
  edm::LogPrint("TestHeterogeneousEDProducerGPU") << label_ << " TestHeterogeneousEDProducerGPU::produceCPU begin event " << iEvent.id().event() << " stream " << iEvent.streamID();

  iEvent.put<OutputType>(std::make_unique<unsigned int>(output_));

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << label_ << " TestHeterogeneousEDProducerGPU::produceCPU end event " << iEvent.id().event() << " stream " << iEvent.streamID() << " result " << output_;
}

void TestHeterogeneousEDProducerGPU::produceGPUCuda(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) {
  edm::Service<CUDAService> cs;
  edm::LogPrint("TestHeterogeneousEDProducerGPU") << label_ << " TestHeterogeneousEDProducerGPU::produceGPUCuda begin event " << iEvent.id().event() << " stream " << iEvent.streamID() << " device " << cs->getCurrentDevice();

  gpuAlgo_->release(label_, cudaStream);
  iEvent.put<OutputType>(std::make_unique<TestHeterogeneousEDProducerGPUTask::ResultTypeRaw>(gpuOutput_.first.get(), gpuOutput_.second.get()),
                         [this, eventId=iEvent.event().id().event(), streamId=iEvent.event().streamID()](const TestHeterogeneousEDProducerGPUTask::ResultTypeRaw& src, unsigned int& dst) {
                           edm::LogPrint("TestHeterogeneousEDProducerGPU") << "  " << label_ << " Copying from GPU to CPU for event " << eventId << " in stream " << streamId;
                           dst = TestHeterogeneousEDProducerGPUTask::getResult(src);
                         });

  edm::LogPrint("TestHeterogeneousEDProducerGPU") << label_ << " TestHeterogeneousEDProducerGPU::produceGPUCuda end event " << iEvent.id().event() << " stream " << iEvent.streamID() << " device " << cs->getCurrentDevice();
}

DEFINE_FWK_MODULE(TestHeterogeneousEDProducerGPU);
