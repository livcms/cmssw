#ifndef RecoTracker_MkFit_MkFitInputWrapper_h
#define RecoTracker_MkFit_MkFitInputWrapper_h

#include "RecoTracker/MkFit/interface/MkFitHitIndexMap.h"

#include <memory>
#include <vector>

namespace mkfit {
  class Hit;
  class Track;
  class LayerNumberConverter;
  using HitVec = std::vector<Hit>;
  using TrackVec = std::vector<Track>;
}

class MkFitInputWrapper {
public:
  MkFitInputWrapper();
  MkFitInputWrapper(MkFitHitIndexMap&& hitIndexMap,
                    std::vector<mkfit::HitVec>&& hits,
                    mkfit::TrackVec&& seeds);
  ~MkFitInputWrapper();

  MkFitInputWrapper(MkFitInputWrapper const&) = delete;
  MkFitInputWrapper& operator=(MkFitInputWrapper const&) = delete;
  MkFitInputWrapper(MkFitInputWrapper&&);
  MkFitInputWrapper& operator=(MkFitInputWrapper&&);

  MkFitHitIndexMap const& hitIndexMap() const { return hitIndexMap_; }
  mkfit::TrackVec const& seeds() const { return *seeds_; }
  std::vector<mkfit::HitVec> const& hits() const { return hits_; }

private:
  MkFitHitIndexMap hitIndexMap_;
  std::vector<mkfit::HitVec> hits_;
  std::unique_ptr<mkfit::TrackVec> seeds_;            // for pimpl pattern
};

#endif
