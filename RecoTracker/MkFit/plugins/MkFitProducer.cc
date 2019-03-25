#include "FWCore/Framework/interface/global/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"
#include "DataFormats/SiStripDetId/interface/StripSubdetector.h"

#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"

#include "RecoTracker/MkFit/interface/MkFitInputWrapper.h"
#include "RecoTracker/MkFit/interface/MkFitOutputWrapper.h"

// MkFit includes
#include "ConfigWrapper.h"
#include "Event.h"
#include "mkFit/buildtestMPlex.h"
#include "mkFit/MkBuilderWrapper.h"

// TBB includes
#include "tbb/task_arena.h"

// std includes
#include <functional>

class MkFitProducer: public edm::global::EDProducer<edm::StreamCache<mkfit::MkBuilderWrapper> > {
public:
  explicit MkFitProducer(edm::ParameterSet const& iConfig);
  ~MkFitProducer() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  std::unique_ptr<mkfit::MkBuilderWrapper> beginStream(edm::StreamID) const;
private:
  void produce(edm::StreamID, edm::Event& iEvent, const edm::EventSetup& iSetup) const override;

  edm::EDGetTokenT<MkFitInputWrapper> hitsSeedsToken_;
  edm::EDPutTokenT<MkFitOutputWrapper> putToken_;
  std::function<double(mkfit::Event&, mkfit::MkBuilder&)> buildFunction_;
  bool backwardFitInCMSSW_;
  bool mkfitSilent_;
};

MkFitProducer::MkFitProducer(edm::ParameterSet const& iConfig):
  hitsSeedsToken_(consumes<MkFitInputWrapper>(iConfig.getParameter<edm::InputTag>("hitsSeeds"))),
  putToken_(produces<MkFitOutputWrapper>()),
  backwardFitInCMSSW_(iConfig.getParameter<bool>("backwardFitInCMSSW")),
  mkfitSilent_(iConfig.getUntrackedParameter<bool>("mkfitSilent"))
{
  const auto build = iConfig.getParameter<std::string>("buildingRoutine");
  bool isFV = false;
  if(build == "bestHit") {
    buildFunction_ = mkfit::runBuildingTestPlexBestHit;
  }
  else if(build == "standard") {
    buildFunction_ = mkfit::runBuildingTestPlexStandard;
  }
  else if(build == "cloneEngine") {
    buildFunction_ = mkfit::runBuildingTestPlexCloneEngine;
  }
  else if(build == "fullVector") {
    isFV = true;
    buildFunction_ = mkfit::runBuildingTestPlexFV;
  }
  else {
    throw cms::Exception("Configuration") << "Invalid value for parameter 'buildingRoutine' " << build << ", allowed are bestHit, standard, cloneEngine, fullVector";
  }

  const auto seedClean = iConfig.getParameter<std::string>("seedCleaning");
  auto seedCleanOpt = mkfit::ConfigWrapper::SeedCleaningOpts::noCleaning;
  if(seedClean == "none") {
    seedCleanOpt = mkfit::ConfigWrapper::SeedCleaningOpts::noCleaning;
  }
  else if(seedClean == "N2") {
    seedCleanOpt = mkfit::ConfigWrapper::SeedCleaningOpts::cleanSeedsN2;
  }
  else {
    throw cms::Exception("Configuration") << "Invalida value for parameter 'seedCleaning' " << seedClean << ", allowed are none, N2";
  }

  auto backwardFitOpt = backwardFitInCMSSW_ ? mkfit::ConfigWrapper::BackwardFit::noFit : mkfit::ConfigWrapper::BackwardFit::toFirstLayer;

  // TODO: what to do when we have multiple instances of MkFitProducer in a job?
  mkfit::MkBuilderWrapper::populate(isFV);
  mkfit::ConfigWrapper::initializeForCMSSW(seedCleanOpt, backwardFitOpt, mkfitSilent_);
}

void MkFitProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;

  desc.add("hitsSeeds", edm::InputTag("mkFitInputConverter"));
  desc.add<std::string>("buildingRoutine", "cloneEngine")->setComment("Valid values are: 'bestHit', 'standard', 'cloneEngine', 'fullVector'");
  desc.add<std::string>("seedCleaning", "N2")->setComment("Valid values are: 'none', 'N2'");
  desc.add("backwardFitInCMSSW", false)->setComment("Do backward fit (to innermost hit) in CMSSW (true) or mkFit (false)");
  desc.addUntracked("mkfitSilent", true)->setComment("Allows to enables printouts from mkfit with 'False'");

  descriptions.add("mkFitProducer", desc);
}

std::unique_ptr<mkfit::MkBuilderWrapper> MkFitProducer::beginStream(edm::StreamID iID) const {
  return std::make_unique<mkfit::MkBuilderWrapper>();
}

namespace {
  std::once_flag geometryFlag;
}
void MkFitProducer::produce(edm::StreamID iID, edm::Event& iEvent, const edm::EventSetup& iSetup) const {
  edm::Handle<MkFitInputWrapper> hhitsSeeds;
  iEvent.getByToken(hitsSeedsToken_, hhitsSeeds);
  const auto& hitsSeeds = *hhitsSeeds;

  edm::ESHandle<TrackerGeometry> trackerGeometry;
  iSetup.get<TrackerDigiGeometryRecord>().get(trackerGeometry);
  const auto& geom = *trackerGeometry;

  // First do some initialization
  // TODO: the mechanism needs to be improved...
  std::call_once(geometryFlag, [&geom, nlayers=hitsSeeds.nlayers()]() {
      // TODO: eventually automatize fully
      // For now it is easier to use purely the infrastructure from mkfit
      /*
      const auto barrelLayers = geom.numberOfLayers(PixelSubdetector::PixelBarrel) + geom.numberOfLayers(StripSubdetector::TIB) + geom.numberOfLayers(StripSubdetector::TOB);
      const auto endcapLayers = geom.numberOfLayers(PixelSubdetector::PixelEndcap) + geom.numberOfLayers(StripSubdetector::TID) + geom.numberOfLayers(StripSubdetector::TEC);
      // TODO: Number of stereo layers is hardcoded for now, so this won't work for phase2 tracker
      const auto barrelStereo = 2 + 2;
      const auto endcapStereo = geom.numberOfLayers(StripSubdetector::TID) + geom.numberOfLayers(StripSubdetector::TEC);
      const auto nlayers = barrelLayers + barrelStereo + 2*(endcapLayers + endcapStereo);
      LogDebug("MkFitProducer") << "Total number of tracker layers (stereo counted separately) " << nlayers;
      */
      if(geom.numberOfLayers(PixelSubdetector::PixelBarrel) != 4 || geom.numberOfLayers(PixelSubdetector::PixelEndcap) != 3) {
        throw cms::Exception("Assert") << "For now this code works only with phase1 tracker, you have something else";
      }
      mkfit::ConfigWrapper::setNTotalLayers(nlayers);
    });

  // CMSSW event ID (64-bit unsigned) does not fit in int
  // In addition, unique ID requires also lumi and run
  // But does the event ID really matter within mkfit?
  mkfit::Event ev(iEvent.id().event());

  ev.setInputFromCMSSW(hitsSeeds.hits(), hitsSeeds.seeds());

  tbb::this_task_arena::isolate([&](){
      buildFunction_(ev, streamCache(iID)->get());
    });

  iEvent.emplace(putToken_, std::move(ev.candidateTracks_), std::move(ev.fitTracks_));
}

DEFINE_FWK_MODULE(MkFitProducer);
