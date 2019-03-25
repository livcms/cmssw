#ifndef RecoTracker_MkFit_MkFitInputWrapper_h
#define RecoTracker_MkFit_MkFitInputWrapper_h

#include "RecoTracker/MkFit/interface/MkFitIndexLayer.h"

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
  MkFitInputWrapper(MkFitIndexLayer&& indexLayers, std::vector<mkfit::HitVec>&& hits, mkfit::TrackVec&& seeds, mkfit::LayerNumberConverter&& lnc);
  ~MkFitInputWrapper();

  MkFitInputWrapper(MkFitInputWrapper&&) = default;
  MkFitInputWrapper& operator=(MkFitInputWrapper&&) = default;

  MkFitIndexLayer const& indexLayers() const { return indexLayers_; }
  mkfit::TrackVec const& seeds() const { return *seeds_; }
  std::vector<mkfit::HitVec> const& hits() const { return hits_; }
  mkfit::LayerNumberConverter const& layerNumberConverter() const { return *lnc_; }
  unsigned int nlayers() const;

private:
  MkFitIndexLayer indexLayers_; //!
  std::vector<mkfit::HitVec> hits_; //!
  std::unique_ptr<mkfit::TrackVec> seeds_; //! for pimpl pattern
  std::unique_ptr<mkfit::LayerNumberConverter> lnc_; //! for pimpl pattern
};

#endif
