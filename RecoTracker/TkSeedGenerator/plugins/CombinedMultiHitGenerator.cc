#include "CombinedMultiHitGenerator.h"

#include "RecoTracker/TkHitPairs/interface/HitPairGeneratorFromLayerPair.h"
#include "RecoTracker/TkSeedGenerator/interface/MultiHitGeneratorFromPairAndLayers.h"
#include "RecoTracker/TkSeedGenerator/interface/MultiHitGeneratorFromPairAndLayersFactory.h"
#include "RecoPixelVertexing/PixelTriplets/interface/LayerTriplets.h"
#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"

CombinedMultiHitGenerator::CombinedMultiHitGenerator(const edm::ParameterSet& cfg):
  theSeedingLayerSrc(cfg.getParameter<edm::InputTag>("SeedingLayers"))
{
  edm::ParameterSet generatorPSet = cfg.getParameter<edm::ParameterSet>("GeneratorPSet");
  std::string       generatorName = generatorPSet.getParameter<std::string>("ComponentName");
  theGenerator.reset(MultiHitGeneratorFromPairAndLayersFactory::get()->create(generatorName, generatorPSet));
  theGenerator->init(HitPairGeneratorFromLayerPair( 0, 1, &theLayerCache), &theLayerCache);
}

CombinedMultiHitGenerator::~CombinedMultiHitGenerator() {}

void CombinedMultiHitGenerator::hitSets(
   const TrackingRegion& region, OrderedMultiHits & result,
   const edm::Event& ev, const edm::EventSetup& es)
{
  edm::Handle<SeedingLayerSetsHits> hlayers;
  ev.getByLabel(theSeedingLayerSrc, hlayers);
  const SeedingLayerSetsHits& layers = *hlayers;
  if(layers.numberOfLayersInSet() != 3)
    throw cms::Exception("Configuration") << "CombinedMultiHitGenerator expects SeedingLayerSetsHits::numberOfLayersInSet() to be 3, got " << layers.numberOfLayersInSet();

  std::vector<LayerTriplets::LayerSetAndLayers> trilayers = LayerTriplets::layers(layers);
  for(const auto& setAndLayers: trilayers) {
    theGenerator->setSeedingLayers(setAndLayers.first, setAndLayers.second);
    theGenerator->hitSets( region, result, ev, es);
  }
  theLayerCache.clear();
}

