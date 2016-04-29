#ifndef RecoPixelVertexing_PixelTriplets_HitTripletEDProducerT_H
#define RecoPixelVertexing_PixelTriplets_HitTripletEDProducerT_H

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Utilities/interface/RunningAverage.h"

#include "RecoTracker/TkHitPairs/interface/IntermediateHitDoublets.h"
#include "RecoTracker/TkSeedingLayers/interface/SeedingHitSet.h"
#include "RecoPixelVertexing/PixelTriplets/interface/OrderedHitTriplets.h"
#include "RecoPixelVertexing/PixelTriplets/interface/IntermediateHitTriplets.h"
#include "RecoPixelVertexing/PixelTriplets/interface/LayerTriplets.h"

template <typename T_HitTripletGenerator>
class HitTripletEDProducerT: public edm::stream::EDProducer<> {
public:
  HitTripletEDProducerT(const edm::ParameterSet& iConfig);
  ~HitTripletEDProducerT() = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  virtual void produce(edm::Event& iEvent, const edm::EventSetup& iSetup) override;

private:
  edm::EDGetTokenT<IntermediateHitDoublets> doubletToken_;

  edm::RunningAverage localRA_;

  T_HitTripletGenerator generator_;

  const bool produceSeedingHitSets_;
  const bool produceIntermediateHitTriplets_;
};

template <typename T_HitTripletGenerator>
HitTripletEDProducerT<T_HitTripletGenerator>::HitTripletEDProducerT(const edm::ParameterSet& iConfig):
  doubletToken_(consumes<IntermediateHitDoublets>(iConfig.getParameter<edm::InputTag>("doublets"))),
  generator_(iConfig, consumesCollector()),
  produceSeedingHitSets_(iConfig.getParameter<bool>("produceSeedingHitSets")),
  produceIntermediateHitTriplets_(iConfig.getParameter<bool>("produceIntermediateHitTriplets"))
{
  if(!produceIntermediateHitTriplets_ && !produceSeedingHitSets_)
    throw cms::Exception("Configuration") << "HitTripletEDProducerT requires either produceIntermediateHitTriplets or produceSeedingHitSets to be True. If neither are needed, just remove this module from your sequence/path as it doesn't do anything useful";
  if(produceSeedingHitSets_)
    produces<std::vector<SeedingHitSet> >();
  if(produceIntermediateHitTriplets_)
    produces<IntermediateHitTriplets>();
}

template <typename T_HitTripletGenerator>
void HitTripletEDProducerT<T_HitTripletGenerator>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;

  desc.add<edm::InputTag>("doublets", edm::InputTag("hitPairEDProducer"));
  desc.add<bool>("produceSeedingHitSets", false);
  desc.add<bool>("produceIntermediateHitTriplets", false);

  T_HitTripletGenerator::fillDescriptions(desc);

  auto label = T_HitTripletGenerator::fillDescriptionsLabel() + std::string("EDProducer");
  descriptions.add(label, desc);
}

template <typename T_HitTripletGenerator>
void HitTripletEDProducerT<T_HitTripletGenerator>::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  edm::Handle<IntermediateHitDoublets> hdoublets;
  iEvent.getByToken(doubletToken_, hdoublets);
  const auto& regionDoublets = *hdoublets;

  const SeedingLayerSetsHits& seedingLayerHits = regionDoublets.seedingLayerHits();
  if(seedingLayerHits.numberOfLayersInSet() < 3) {
    throw cms::Exception("Configuration") << "HitTripletEDProducerT expects SeedingLayerSetsHits::numberOfLayersInSet() to be >= 3, got " << seedingLayerHits.numberOfLayersInSet();
  }

  std::unique_ptr<std::vector<SeedingHitSet> > seedingHitSets;
  if(produceSeedingHitSets_) {
    seedingHitSets = std::make_unique<std::vector<SeedingHitSet> >();
    seedingHitSets->reserve(localRA_.upper());
  }
  std::unique_ptr<IntermediateHitTriplets> intermediateHitTriplets;
  if(produceIntermediateHitTriplets_) {
    intermediateHitTriplets = std::make_unique<IntermediateHitTriplets>(&seedingLayerHits);
    intermediateHitTriplets->reserve(regionDoublets.regionSize(), seedingLayerHits.size(), localRA_.upper());
  }

  // match-making of pair and triplet layers
  std::vector<LayerTriplets::LayerSetAndLayers> trilayers = LayerTriplets::layers(seedingLayerHits);
  std::vector<unsigned int> thirdLayerHitBeginIndices;
  thirdLayerHitBeginIndices.reserve(3); // Yes, vector is a bit overkill as there can be at most 3 3rd layers for a doublet. But better be consistent and migrate later everything to e.g. std::array or edm::VecArray. Except with phase1 there can be more than 3 3rd layers...

  OrderedHitTriplets triplets;
  triplets.reserve(localRA_.upper());
  size_t triplets_total = 0;

  for(const auto& regionLayerPairs: regionDoublets) {
    const TrackingRegion& region = regionLayerPairs.region();
    intermediateHitTriplets->beginRegion(&region);

    for(const auto& layerPair: regionLayerPairs) {
      auto found = std::find_if(trilayers.begin(), trilayers.end(), [&](const LayerTriplets::LayerSetAndLayers& a) {
          return a.first[0].index() == layerPair.innerLayerIndex() && a.first[1].index() == layerPair.outerLayerIndex();
        });
      if(found == trilayers.end()) {
        auto exp = cms::Exception("LogicError") << "Did not find the layer pair from vector<pair+third layers>. This is a sign of some internal inconsistency\n";
        exp << "I was looking for layer pair " << layerPair.innerLayerIndex() << "," << layerPair.outerLayerIndex() << ". Triplets have the following pairs:\n";
        for(const auto& a: trilayers) {
          exp << " " << a.first[0].index() << "," << a.first[1].index() << ": 3rd layers";
          for(const auto& b: a.second) {
            exp << " " << b.index();
          }
          exp << "\n";
        }
        throw exp;
      }

      LayerHitMapCache hitCache;
      hitCache.extend(layerPair.cache());

      thirdLayerHitBeginIndices.clear();
      generator_.hitTriplets(region, triplets, iEvent, iSetup, layerPair.doublets(), found->second, &thirdLayerHitBeginIndices, hitCache);
      if(triplets.empty())
        continue;

      triplets_total += triplets.size();
      if(produceSeedingHitSets_) {
        for(const auto& trpl: triplets) {
          seedingHitSets->emplace_back(trpl.inner(), trpl.middle(), trpl.outer());
        }
      }
      if(produceIntermediateHitTriplets_) {
        if(thirdLayerHitBeginIndices.size() != trilayers.size()) {
          throw cms::Exception("LogicError") << "thirdLayerHitBeginIndices.size() " << thirdLayerHitBeginIndices.size()
                                             << " trilayers.size() " << trilayers.size();
        }
        for(size_t i=0, size=trilayers.size(); i<size; ++i) {
          const size_t begin = thirdLayerHitBeginIndices[i];
          const size_t end = i<size ? thirdLayerHitBeginIndices[i+1] : triplets.size();
          intermediateHitTriplets->addTriplets(found->first, found->second[i],
                                               std::next(triplets.begin(), begin), std::next(triplets.begin(), end),
                                               std::move(hitCache));
        }
      }
    }
  }
  localRA_.update(triplets_total);

  if(produceSeedingHitSets_)
    iEvent.put(std::move(seedingHitSets));
  if(produceIntermediateHitTriplets_)
    iEvent.put(std::move(intermediateHitTriplets));
}


#endif
