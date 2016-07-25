#ifndef RecoPixelVertexing_PixelTriplets_IntermediateHitTriplets_h
#define RecoPixelVertexing_PixelTriplets_IntermediateHitTriplets_h

#include "RecoTracker/TkHitPairs/interface/LayerHitMapCache.h"
#include "RecoTracker/TkHitPairs/interface/IntermediateHitDoublets.h"
#include "TrackingTools/TransientTrackingRecHit/interface/SeedingLayerSetsHits.h"
#include "RecoPixelVertexing/PixelTriplets/interface/OrderedHitTriplets.h"

/**
 * Simple container of temporary information delivered from hit triplet
 * generator to hit quadruplet generator via edm::Event.
 */
class IntermediateHitTriplets {
public:
  using LayerPair = std::tuple<SeedingLayerSetsHits::LayerIndex,
                               SeedingLayerSetsHits::LayerIndex>;
  using LayerTriplet = std::tuple<SeedingLayerSetsHits::LayerIndex,
                                  SeedingLayerSetsHits::LayerIndex,
                                  SeedingLayerSetsHits::LayerIndex>;
  using RegionIndex = ihd::RegionIndex;

  ////////////////////

  class ThirdLayer {
  public:
    ThirdLayer(const SeedingLayerSetsHits::SeedingLayer& thirdLayer, size_t hitsBegin):
      thirdLayer_(thirdLayer.index()), hitsBegin_(hitsBegin), hitsEnd_(hitsBegin)
    {}

    void setHitsEnd(size_t end) { hitsEnd_ = end; }

    SeedingLayerSetsHits::LayerIndex layerIndex() const { return thirdLayer_; }

  private:
    SeedingLayerSetsHits::LayerIndex thirdLayer_;
    size_t hitsBegin_;
    size_t hitsEnd_;
  };

  ////////////////////

  class LayerPairAndLayers {
  public:
    LayerPairAndLayers(const LayerPair& layerPair,
                       size_t thirdLayersBegin, LayerHitMapCache&& cache):
      layerPair_(layerPair),
      thirdLayersBegin_(thirdLayersBegin),
      thirdLayersEnd_(thirdLayersBegin),
      cache_(std::move(cache))
    {}

    void setThirdLayersEnd(size_t end) { thirdLayersEnd_ = end; }

    const LayerPair& layerPair() const { return layerPair_; }
    size_t thirdLayersBegin() const { return thirdLayersBegin_; }
    size_t thirdLayersEnd() const { return thirdLayersEnd_; }

    LayerHitMapCache& cache() { return cache_; }
    const LayerHitMapCache& cache() const { return cache_; }

  private:
    LayerPair layerPair_;
    size_t thirdLayersBegin_;
    size_t thirdLayersEnd_;
    // The reason for not storing layer triplets + hit triplets
    // directly is in this cache, and in my desire to try to keep
    // results unchanged during this refactoring
    LayerHitMapCache cache_;
  };

  ////////////////////

  class LayerTripletHits {
  public:
    LayerTripletHits(const LayerPairAndLayers *layerPairAndLayers,
                     const ThirdLayer *thirdLayer):
      layerPairAndLayers_(layerPairAndLayers),
      thirdLayer_(thirdLayer)
    {}

    SeedingLayerSetsHits::LayerIndex innerLayerIndex() const { return std::get<0>(layerPairAndLayers_->layerPair()); }
    SeedingLayerSetsHits::LayerIndex middleLayerIndex() const { return std::get<1>(layerPairAndLayers_->layerPair()); }
    SeedingLayerSetsHits::LayerIndex outerLayerIndex() const { return thirdLayer_->layerIndex(); }

    const LayerHitMapCache& cache() const { return layerPairAndLayers_->cache(); }
  private:
    const LayerPairAndLayers *layerPairAndLayers_;
    const ThirdLayer *thirdLayer_;
  };

  ////////////////////

  //using RegionLayerHits = ihd::RegionLayerHits<LayerPairAndLayers>;
  class RegionLayerHits {
  public:
    using LayerPairAndLayersConstIterator = std::vector<LayerPairAndLayers>::const_iterator;
    using ThirdLayerConstIterator = std::vector<ThirdLayer>::const_iterator;
    using HitConstIterator = std::vector<OrderedHitTriplet>::const_iterator;

    class const_iterator {
    public:
      using internal_iterator_type = LayerPairAndLayersConstIterator;
      using value_type = LayerTripletHits;
      using difference_type = internal_iterator_type::difference_type;

      const_iterator(const RegionLayerHits *regionLayerHits):
        regionLayerHits_(regionLayerHits),
        iterPair_(regionLayerHits->layerSetsBegin()),
        indThird_(iterPair_->thirdLayersBegin())
      {}

      value_type operator*() const {
        return value_type(&(*iterPair_), &(*(regionLayerHits_->thirdLayerBegin() + indThird_)));
      }

      const_iterator& operator++() {
        auto nextThird = ++indThird_;
        if(nextThird == iterPair_->thirdLayersEnd()) {
          ++iterPair_;
          indThird_ = iterPair_->thirdLayersBegin();
        }
        else {
          indThird_ = nextThird;
        }
        return *this;
      }

      const_iterator operator++(int) {
        const_iterator clone(*this);
        operator++();
        return clone;
      }

      bool operator==(const const_iterator& other) const { return iterPair_ == other.iterPair_ && indThird_ == other.indThird_; }
      bool operator!=(const const_iterator& other) const { return !operator==(other); }

    private:
      const RegionLayerHits *regionLayerHits_;
      internal_iterator_type iterPair_;
      size_t indThird_;
    };

    RegionLayerHits(const TrackingRegion* region,
                    LayerPairAndLayersConstIterator pairBegin, LayerPairAndLayersConstIterator pairEnd,
                    ThirdLayerConstIterator thirdBegin, ThirdLayerConstIterator thirdEnd):
      region_(region),
      layerSetsBegin_(pairBegin), layerSetsEnd_(pairEnd),
      thirdBegin_(thirdBegin), thirdEnd_(thirdEnd)
    {}

    const TrackingRegion& region() const { return *region_; }

    /*
    const_iterator begin() const { return layerSetsBegin_; }
    const_iterator cbegin() const { return begin(); }
    const_iterator end() const { return layerSetsEnd_; }
    const_iterator cend() const { return end(); }
    */

    // used internally
    LayerPairAndLayersConstIterator layerSetsBegin() const { return layerSetsBegin_; }
    LayerPairAndLayersConstIterator layerSetsEnd() const { return layerSetsEnd_; }
    ThirdLayerConstIterator thirdLayerBegin() const { return thirdBegin_; }
    ThirdLayerConstIterator thirdLayerEnd() const { return thirdEnd_; }

  private:
    const TrackingRegion *region_;
    const LayerPairAndLayersConstIterator layerSetsBegin_;
    const LayerPairAndLayersConstIterator layerSetsEnd_;
    const ThirdLayerConstIterator thirdBegin_;
    const ThirdLayerConstIterator thirdEnd_;
  };

  ////////////////////

  using const_iterator = ihd::const_iterator<RegionLayerHits, IntermediateHitTriplets>;

  ////////////////////

  IntermediateHitTriplets(): seedingLayers_(nullptr) {}
  explicit IntermediateHitTriplets(const SeedingLayerSetsHits *seedingLayers): seedingLayers_(seedingLayers) {}
  IntermediateHitTriplets(const IntermediateHitTriplets& rh); // only to make ROOT dictionary generation happy
  ~IntermediateHitTriplets() = default;

  void swap(IntermediateHitTriplets& rh) {
    std::swap(seedingLayers_, rh.seedingLayers_);
    std::swap(regions_, rh.regions_);
    std::swap(layerPairAndLayers_, rh.layerPairAndLayers_);
    std::swap(thirdLayers_, rh.thirdLayers_);
    std::swap(hitTriplets_, rh.hitTriplets_);
  }

  void reserve(size_t nregions, size_t nlayersets, size_t ntriplets) {
    regions_.reserve(nregions);
    layerPairAndLayers_.reserve(nregions*nlayersets);
    thirdLayers_.reserve(nregions*nlayersets);
    hitTriplets_.reserve(ntriplets);
  }

  void shrink_to_fit() {
    regions_.shrink_to_fit();
    layerPairAndLayers_.shrink_to_fit();
    thirdLayers_.shrink_to_fit();
    hitTriplets_.shrink_to_fit();
  }

  void beginRegion(const TrackingRegion *region) {
    regions_.emplace_back(region, layerPairAndLayers_.size());
  }

  LayerHitMapCache *beginPair(const LayerPair& layerPair, LayerHitMapCache&& cache) {
    layerPairAndLayers_.emplace_back(layerPair, thirdLayers_.size(), std::move(cache));
    return &(layerPairAndLayers_.back().cache());
  };

  void addTriplets(const std::vector<SeedingLayerSetsHits::SeedingLayer>& thirdLayers,
                   const OrderedHitTriplets& triplets,
                   const std::vector<int>& thirdLayerIndex,
                   const std::vector<size_t>& permutations) {
    assert(triplets.size() == thirdLayerIndex.size());
    assert(triplets.size() == permutations.size());

    int prevLayer = -1;
    for(size_t i=0, size=permutations.size(); i<size; ++i) {
      // We go through the 'triplets' in the order defined by
      // 'permutations', which is sorted such that we first go through
      // triplets from (3rd) layer 0, then layer 1 and so on.
      const size_t realIndex = permutations[i];

      const int layer = thirdLayerIndex[realIndex];
      if(layer != prevLayer) {
        prevLayer = layer;
        thirdLayers_.emplace_back(thirdLayers[layer], hitTriplets_.size());
        layerPairAndLayers_.back().setThirdLayersEnd(thirdLayers_.size());
      }

      hitTriplets_.emplace_back(triplets[realIndex]);
      thirdLayers_.back().setHitsEnd(hitTriplets_.size());
    }
  }

  const SeedingLayerSetsHits& seedingLayerHits() const { return *seedingLayers_; }

  const_iterator begin() const { return const_iterator(this, regions_.begin()); }
  const_iterator cbegin() const { return begin(); }
  const_iterator end() const { return const_iterator(this, regions_.end()); }
  const_iterator cend() const { return end(); }

  // used internally
  std::vector<RegionIndex>::const_iterator regionsBegin() const { return regions_.begin(); }
  std::vector<RegionIndex>::const_iterator regionsEnd() const { return regions_.end(); }
  std::vector<LayerPairAndLayers>::const_iterator layerSetsBegin() const { return layerPairAndLayers_.begin(); }
  std::vector<LayerPairAndLayers>::const_iterator layerSetsEnd() const { return layerPairAndLayers_.begin(); }

private:
  const SeedingLayerSetsHits *seedingLayers_;

  std::vector<RegionIndex> regions_;
  std::vector<LayerPairAndLayers> layerPairAndLayers_;
  std::vector<ThirdLayer> thirdLayers_;
  std::vector<OrderedHitTriplet> hitTriplets_;
};

#endif
