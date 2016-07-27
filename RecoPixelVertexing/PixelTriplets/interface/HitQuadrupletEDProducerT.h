#ifndef RecoPixelVertexing_PixelTriplets_HitQuadrupletEDProducerT_H
#define RecoPixelVertexing_PixelTriplets_HitQuadrupletEDProducerT_H

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Utilities/interface/RunningAverage.h"

/#include "RecoTracker/TkSeedingLayers/interface/SeedingHitSet.h"
#include "RecoPixelVertexing/PixelTriplets/interface/OrderedHitQuadruplets.h"
#include "RecoPixelVertexing/PixelTriplets/interface/IntermediateHitTriplets.h"
#include "RecoPixelVertexing/PixelTriplets/interface/LayerQuadruplets.h"

template <typename T_HitQuadrupletGenerator>
class HitQuadrupletEDProducerT: public edm::stream::EDProducer<> {
public:
  HitQuadrupletEDProducerT(const edm::ParameterSet& iConfig);
  ~HitQuadrupletEDProducerT() = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  virtual void produce(edm::Event& iEvent, const edm::EventSetup& iSetup) override;

private:
  edm::EDGetTokenT<IntermediateHitTriplets> tripletToken_;

  edm::RunningAverage localRA_;

  T_HitQuadrupletGenerator generator_;
};

template <typename T_HitQuadrupletGenerator>
HitQuadrupletEDProducerT<T_HitQuadrupletGenerator>::HitQuadrupletEDProducerT(const edm::ParameterSet& iConfig):
  tripletToken_(consumes<IntermediateHitTriplets>(iConfig.getParameter<edm::InputTag>("triplets"))),
  generator_(iConfig, consumesCollector())
{
  produces<std::vector<SeedingHitSet> >();
}

template <typename T_HitQuadrupletGenerator>
void HitQuadrupletEDProducerT<T_HitQuadrupletGenerator>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;

  desc.add<edm::InputTag>("triplets", edm::InputTag("hitTripletEDProducer"));
  T_HitQuadrupletGenerator::fillDescriptions(desc);

  descriptions.add(T_HitQuadrupletGenerator::fillDescriptionsLabel(), desc);
}

template <typename T_HitQuadrupletGenerator>
void HitQuadrupletEDProducerT<T_HitQuadrupletGenerator>::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  edm::Handle<IntermediateHitTriplets> htriplets;
  iEvent.getByToken(tripletToken_, htriplets);
  const auto& regionTriplets = *htriplets;

  const SeedingLayerSetsHits& seedingLayerHits = regionTriplets.seedingLayerHits();
  if(seedingLayerHits.numberOfLayersInSet() < 4) {
    throw cms::Exception("Configuration") << "HitQuadrupletEDProducerT expects SeedingLayerSetsHits::numberOfLayersInSet() to be >= 4, got " << seedingLayerHits.numberOfLayersInSet();
  }

  auto seedingHitSets = std::make_unique<std::vector<SeedingHitSet> >();
  seedingHitSets->reserve(localRA_.upper());

  // match-making of triplet and quadruplet layers
  std::vector<LayerQuadruplets::LayerSetAndLayers> quadlayers = LayerQuadruplets::layers(seedingLayerHits);

  std::vector<OrderedHitSeeds> quadruplets;
  quadruplets.reserve(localRA_.upper());

  for(const auto& regionLayerPairAndLayers: regionTriplets) {
    const TrackingRegion& region = regionLayerPairAndLayers.region();

    for(const auto& layerTriplet: regionLayerPairAndLayers) {
      auto found = std::find_if(quadlayers.begin(), quadlayers.end(), [&](const LayerQuadruplets::LayerSetAndLayers& a) {
          return a.first[0].index() == layerTriplet.innerLayerIndex() &&
                 a.first[1].index() == layerTriplet.middleLayerIndex() &&
                 a.first[2].index() == layerTriplet.outerLayerIndex();
        });
      if(found == quadlayers.end()) {
        auto exp = cms::Exception("LogicError") << "Did not find the layer triplet from vector<triplet+fourth layers>. This is a sign of some internal inconsistency\n";
        exp << "I was looking for layer triplet " << layerTriplet.innerLayerIndex() << "," << layerTriplet.middleLayerIndex() << "," << layerTriplet.outerLayerIndex()
            << ". Quadruplets have the following triplets:\n";
        for(const auto& a: trilayers) {
          exp << " " << a.first[0].index() << "," << a.first[1].index() << "," << a.first[2].index() << ": 4th layers";
          for(const auto& b: a.second) {
            exp << " " << b.index();
          }
          exp << "\n";
        }
        throw exp;
      }
      const auto& fourthLayers = found->second;

      LayerHitMapCache hitCache;
      hitCache.extend(layerTriplet.cache());

      generator_.hitQuadruplets(region, quadruplets, iEvent, iSetup, layerPair.doublets(), thirdLayers, &tripletLastLayerIndex, hitCache);
      if(triplets.empty())
        continue;

      triplets_total += triplets.size();
      if(produceSeedingHitSets_) {
        for(const auto& trpl: triplets) {
          seedingHitSets->emplace_back(trpl.inner(), trpl.middle(), trpl.outer());
        }
      }
      if(produceIntermediateHitTriplets_) {
        if(tripletLastLayerIndex.size() != triplets.size()) {
          throw cms::Exception("LogicError") << "tripletLastLayerIndex.size() " << tripletLastLayerIndex.size()
                                             << " triplets.size() " << triplets.size();
        }
        tripletPermutation.resize(tripletLastLayerIndex.size());
        std::iota(tripletPermutation.begin(), tripletPermutation.end(), 0); // assign 0,1,2,...,N
        std::stable_sort(tripletPermutation.begin(), tripletPermutation.end(), [&](size_t i, size_t j) {
            return tripletLastLayerIndex[i] < tripletLastLayerIndex[j];
          });

        intermediateHitTriplets->addTriplets(thirdLayers, triplets, tripletLastLayerIndex, tripletPermutation);
        triplets.clear();
      }
    }
  }
  localRA_.update(quadruplets.size());

  iEvent.put(std::move(seedingHitSets));
}


#endif
