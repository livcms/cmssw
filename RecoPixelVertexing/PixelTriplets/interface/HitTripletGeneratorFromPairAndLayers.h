#ifndef HitTripletGeneratorFromPairAndLayers_H
#define HitTripletGeneratorFromPairAndLayers_H

/** A HitTripletGenerator from HitPairGenerator and vector of
    Layers. The HitPairGenerator provides a set of hit pairs.
    For each pair the search for compatible hit(s) is done among
    provided Layers
 */

#include "RecoPixelVertexing/PixelTriplets/interface/HitTripletGenerator.h"
#include "RecoTracker/TkHitPairs/interface/HitPairGenerator.h"
#include <vector>
#include "RecoTracker/SeedingLayerSet/interface/SeedingLayerSetNew.h"
#include "RecoTracker/TkHitPairs/interface/LayerHitMapCache.h"


class HitTripletGeneratorFromPairAndLayers : public HitTripletGenerator {

public:
  typedef LayerHitMapCache  LayerCacheType;

  virtual ~HitTripletGeneratorFromPairAndLayers() {}

  virtual void init( const HitPairGenerator & pairs, LayerCacheType* layerCache) = 0;

  virtual void setSeedingLayers(SeedingLayerSetNew::SeedingLayers pairLayers,
                                std::vector<SeedingLayerSetNew::SeedingLayer> thirdLayers) = 0;
};
#endif


