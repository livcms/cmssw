#ifndef Alignment_CommonAlignmentAlgorithm_AlignmentCSCTrackSelector_h
#define Alignment_CommonAlignmentAlgorithm_AlignmentCSCTrackSelector_h

#include "DataFormats/TrackReco/interface/Track.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSetfwd.h"
#include <vector>

namespace edm {
  class Event;
}

class TrackingRecHit;

class AlignmentCSCTrackSelector
{

 public:

  typedef std::vector<const reco::Track*> Tracks; 

  /// constructor
  AlignmentCSCTrackSelector(const edm::ParameterSet & cfg);

  /// destructor
  ~AlignmentCSCTrackSelector();

  static void fillPSetDescription(edm::ParameterSetDescription& desc);

  /// select tracks
  Tracks select(const Tracks& tracks, const edm::Event& evt) const;

 private:

  edm::InputTag m_src;
  int m_stationA, m_stationB, m_minHitsDT, m_minHitsPerStation, m_maxHitsPerStation;

};

#endif

