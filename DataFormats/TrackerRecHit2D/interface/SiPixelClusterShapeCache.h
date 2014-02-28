// -*- c++ -*-
#ifndef DataFormats_SiPixelClusterShapeData_h
#define DataFormats_SiPixelClusterShapeData_h

#include "DataFormats/Provenance/interface/ProductID.h"
#include "DataFormats/Common/interface/HandleBase.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHit.h"

#include <utility>
#include <vector>
#include <algorithm>
#include <cassert>
#include <memory>

class PixelGeomDetUnit;

class SiPixelClusterShapeData {
public:
  typedef std::vector<std::pair<int, int> >::const_iterator const_iterator;
  typedef std::pair<const_iterator, const_iterator> Range;
  SiPixelClusterShapeData(const_iterator begin, const_iterator end, bool isStraight, bool isComplete, bool hasBigPixelsOnlyInside):
    begin_(begin), end_(end), isStraight_(isStraight), isComplete_(isComplete), hasBigPixelsOnlyInside_(hasBigPixelsOnlyInside)
  {}
  ~SiPixelClusterShapeData();

  Range size() const { return std::make_pair(begin_, end_); }

  bool isStraight() const { return isStraight_; }
  bool isComplete() const { return isComplete_; }
  bool hasBigPixelsOnlyInside() const { return hasBigPixelsOnlyInside_; }

private:
  const_iterator begin_, end_;
  const bool isStraight_, isComplete_, hasBigPixelsOnlyInside_;
};

class SiPixelClusterShapeCache {
public:
  typedef SiPixelRecHit::ClusterRef ClusterRef;

  struct Field {
    Field(): offset(0), size(0), straight(false), complete(false), has(false), filled(false) {}

    Field(unsigned off, unsigned siz, bool s, bool c, bool h):
      offset(off), size(siz), straight(s), complete(c), has(h), filled(true) {}
    unsigned offset: 24; // room for 2^24/9 = ~2.8e6 clusters, should be enough
    unsigned size: 4; // max 9 elements / cluster (2^4=16)
    unsigned straight:1;
    unsigned complete:1;
    unsigned has:1;
    unsigned filled:1;
  };

  struct LazyGetter {
    LazyGetter();
    virtual ~LazyGetter();

    virtual void fill(const ClusterRef& cluster, const PixelGeomDetUnit *pixDet, SiPixelClusterShapeCache& cache) const = 0;
  };

  SiPixelClusterShapeCache() {};
  explicit SiPixelClusterShapeCache(const edm::HandleBase& handle): productId_(handle.id()) {}
  SiPixelClusterShapeCache(const edm::HandleBase& handle, std::shared_ptr<LazyGetter> getter): getter_(getter), productId_(handle.id()) {}
  explicit SiPixelClusterShapeCache(const edm::ProductID& id): productId_(id) {}
  ~SiPixelClusterShapeCache();

  void resize(size_t size) {
    data_.resize(size);
    sizeData_.reserve(size);
  }

  void swap(SiPixelClusterShapeCache& other) {
    data_.swap(other.data_);
    sizeData_.swap(other.sizeData_);
    std::swap(getter_, other.getter_);
    std::swap(productId_, other.productId_);
  }

#if !defined(__CINT__) && !defined(__MAKECINT__) && !defined(__REFLEX__)
  void shrink_to_fit() {
    data_.shrink_to_fit();
    sizeData_.shrink_to_fit();
  }

  template <typename T>
  void insert(const ClusterRef& cluster, const T& data) {
    static_assert(T::ArrayType::size_max() <= 16, "T::ArrayType::max_size() more than 16, bit field too narrow");
    check_productId(cluster.id());
    check_index(cluster.index());

    data_[cluster.index()] = Field(sizeData_.size(), data.size.size(), data.isStraight, data.isComplete, data.hasBigPixelsOnlyInside);
    std::copy(data.size.begin(), data.size.end(), std::back_inserter(sizeData_));
  }

  SiPixelClusterShapeData get(const ClusterRef& cluster, const PixelGeomDetUnit *pixDet) const {
    check_productId(cluster.id());
    check_index(cluster.index());
    Field f = data_[cluster.index()];
    if(!f.filled) {
      assert(getter_);
      getter_->fill(cluster, pixDet, *(const_cast<SiPixelClusterShapeCache *>(this)));
      f = data_[cluster.index()];
    }

    auto beg = sizeData_.begin()+f.offset;
    auto end = beg+f.size;

    return SiPixelClusterShapeData(beg, end, f.straight, f.complete, f.has);
  }
#endif

private:
  void check_productId(edm::ProductID id) const;
  void check_index(ClusterRef::key_type index) const;

  std::vector<Field> data_;
  std::vector<std::pair<int, int> > sizeData_;
  std::shared_ptr<LazyGetter> getter_;
  edm::ProductID productId_;
};

#endif
