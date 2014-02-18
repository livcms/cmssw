#ifndef MultiHitGeneratorFromChi2_H
#define MultiHitGeneratorFromChi2_H

/** A MultiHitGenerator from HitPairGenerator and vector of
    Layers. The HitPairGenerator provides a set of hit pairs.
    For each pair the search for compatible hit(s) is done among
    provided Layers
 */

#include "RecoTracker/TkSeedGenerator/interface/MultiHitGenerator.h"
#include "CombinedMultiHitGenerator.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "RecoTracker/TkSeedGenerator/interface/MultiHitGeneratorFromPairAndLayers.h"
#include "RecoPixelVertexing/PixelLowPtUtilities/interface/ClusterShapeHitFilter.h"

#include <utility>
#include <vector>

class HitPairGeneratorFromLayerPair;

class MultiHitGeneratorFromChi2 : public MultiHitGeneratorFromPairAndLayers {

typedef CombinedMultiHitGenerator::LayerCacheType       LayerCacheType;

public:
  MultiHitGeneratorFromChi2(const edm::ParameterSet& cfg);

  virtual ~MultiHitGeneratorFromChi2();

  virtual void hitSets( const TrackingRegion& region, OrderedMultiHits & trs, 
                        const edm::Event & ev, const edm::EventSetup& es,
                        SeedingLayerSetsHits::SeedingLayerSet pairLayers,
                        std::vector<SeedingLayerSetsHits::SeedingLayer> thirdLayers);

private:

  bool checkPhiInRange(float phi, float phi1, float phi2) const;
  std::pair<float,float> mergePhiRanges(
      const std::pair<float,float> &r1, const std::pair<float,float> &r2) const;


private:
  const ClusterShapeHitFilter* filter;

  bool useFixedPreFiltering;
  float extraHitRZtolerance;
  float extraHitRPhitolerance;
  float extraPhiKDBox;
  float dphi;
  const MagneticField* bfield;
  float nomField;
  double nSigmaRZ, nSigmaPhi, fnSigmaRZ;
  bool chi2VsPtCut;
  double maxChi2;
  std::vector<double> pt_interv;
  std::vector<double> chi2_cuts;
  bool refitHits;
  bool debug;
  std::string filterName_;
  std::vector<int> detIdsToDebug;
  
};
#endif


