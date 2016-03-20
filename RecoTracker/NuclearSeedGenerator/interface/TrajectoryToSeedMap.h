#ifndef TrajectoryToSeedMap_h
#define TrajectoryToSeedMap_h

#include <vector>
#include "DataFormats/Common/interface/Ref.h"
#include "DataFormats/Common/interface/RefVector.h"
#include "DataFormats/Common/interface/RefProd.h"
#include "DataFormats/Common/interface/AssociationMap.h"

#include "DataFormats/TrajectorySeed/interface/TrajectorySeedFwd.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
class Trajectory;

    typedef std::vector<Trajectory> TrajectoryCollection;

   /// association map
   typedef edm::AssociationMap<edm::OneToMany<TrajectoryCollection, TrajectorySeedCollection> > TrajectoryToSeedsMap;
   typedef  TrajectoryToSeedsMap::value_type TrajectoryToSeeds;

   /// reference to an object in a collection of SeedMap objects
  typedef edm::Ref<TrajectoryToSeedsMap> TrajectoryToSeedsMapRef;

  /// reference to a collection of SeedMap object
  typedef edm::RefProd<TrajectoryToSeedsMap> TrajectoryToSeedsMapRefProd;

  /// vector of references to objects in the same colletion of SeedMap objects
  typedef edm::RefVector<TrajectoryToSeedsMap> TrajectoryToSeedsMapRefVector;

  /// association map
   typedef edm::AssociationMap<edm::OneToMany<reco::TrackCollection, TrajectorySeedCollection> > TrackToSeedsMap;
   typedef  TrackToSeedsMap::value_type TrackToSeeds;

   /// reference to an object in a collection of SeedMap objects
  typedef edm::Ref<TrackToSeedsMap> TrackToSeedsMapRef;

  /// reference to a collection of SeedMap object
  typedef edm::RefProd<TrackToSeedsMap> TrackToSeedsMapRefProd;

  /// vector of references to objects in the same colletion of SeedMap objects
  typedef edm::RefVector<TrackToSeedsMap> TrackToSeedsMapRefVector;

/// association map
   typedef edm::AssociationMap<edm::OneToMany<reco::TrackCollection, reco::TrackCollection> > TrackToTracksMap;
   typedef  TrackToTracksMap::value_type TrackToTracks;

   /// reference to an object in a collection of SeedMap objects
  typedef edm::Ref<TrackToTracksMap> TrackToTracksMapRef;

  /// reference to a collection of SeedMap object
  typedef edm::RefProd<TrackToTracksMap> TrackToTracksMapRefProd;

  /// vector of references to objects in the same colletion of SeedMap objects
  typedef edm::RefVector<TrackToTracksMap> TrackToTracksMapRefVector;

#endif


