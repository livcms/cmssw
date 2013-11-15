#ifndef PixelTripletLargeTipGenerator_H
#define PixelTripletLargeTipGenerator_H

/** A HitTripletGenerator from HitPairGenerator and vector of
    Layers. The HitPairGenerator provides a set of hit pairs.
    For each pair the search for compatible hit(s) is done among
    provided Layers
 */

#include "RecoTracker/TkHitPairs/interface/HitPairGenerator.h"
#include "RecoPixelVertexing/PixelTriplets/interface/HitTripletGenerator.h"
#include "CombinedHitTripletGenerator.h"
#include "RecoTracker/TkSeedingLayers/interface/SeedingLayer.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "RecoPixelVertexing/PixelTriplets/interface/HitTripletGeneratorFromPairAndLayers.h"

#include <utility>
#include <vector>


class PixelTripletLargeTipGenerator : public HitTripletGeneratorFromPairAndLayers {

typedef CombinedHitTripletGenerator::LayerCacheType       LayerCacheType;

public:
  PixelTripletLargeTipGenerator( const edm::ParameterSet& cfg);

  virtual ~PixelTripletLargeTipGenerator() { delete thePairGenerator; }

  PixelTripletLargeTipGenerator *clone() const override;

  virtual void init( const HitPairGenerator & pairs,
      const std::vector<ctfseeding::SeedingLayer> & layers, LayerCacheType* layerCache);

  virtual void hitTriplets( const TrackingRegion& region, OrderedHitTriplets & trs, 
      const edm::Event & ev, const edm::EventSetup& es);

  const HitPairGenerator & pairGenerator() const { return *thePairGenerator; }
  const std::vector<ctfseeding::SeedingLayer> & thirdLayers() const { return theLayers; }

private:

  bool checkPhiInRange(float phi, float phi1, float phi2) const;
  std::pair<float,float> mergePhiRanges(
      const std::pair<float,float> &r1, const std::pair<float,float> &r2) const;


private:
  HitPairGenerator * thePairGenerator;
  std::vector<ctfseeding::SeedingLayer> theLayers;
  LayerCacheType * theLayerCache;

  bool useFixedPreFiltering;
  float extraHitRZtolerance;
  float extraHitRPhitolerance;
  bool useMScat;
  bool useBend;
  float dphi;
};
#endif


