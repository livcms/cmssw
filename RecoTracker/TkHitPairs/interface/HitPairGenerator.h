#ifndef HitPairGenerator_H
#define HitPairGenerator_H

#include <vector>

/** abstract interface for generators of ordered RecHit pairs
 *  compatible with a TrackingRegion. 
 *  This is used by the HitPairSeedGenerator to produce TrajectorySeeds
 */

#include "RecoTracker/TkTrackingRegions/interface/OrderedHitsGenerator.h"
#include "RecoTracker/TkHitPairs/interface/OrderedHitPairs.h"
#include "RecoTracker/TkHitPairs/interface/RecHitsSortedInPhi.h"
#include "TrackingTools/TransientTrackingRecHit/interface/SeedingLayerSetsHits.h"

class TrackingRegion;
namespace edm { class Event; class EventSetup; }

class HitPairGenerator : public OrderedHitsGenerator {
public:

  explicit HitPairGenerator(unsigned int size=7500);

  virtual ~HitPairGenerator() { }

  virtual const OrderedHitPairs & run(
    const TrackingRegion& region, const edm::Event & ev, const edm::EventSetup& es);

  virtual void hitPairs( const TrackingRegion& reg, OrderedHitPairs & prs, 
      const edm::Event & ev,  const edm::EventSetup& es) = 0;

  virtual void clear() {
     // back to initial allocation if too large
     if (thePairs.capacity()> 4*m_capacity) {
       OrderedHitPairs tmp; tmp.reserve(m_capacity); tmp.swap(thePairs);
     } 
     thePairs.clear(); 
  } 

private:
  OrderedHitPairs thePairs;
  unsigned int m_capacity;

};

#endif
