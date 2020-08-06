#ifndef DataFormats_Common_getThinned_implementation_h
#define DataFormats_Common_getThinned_implementation_h

#include <optional>
#include <tuple>

#include "DataFormats/Provenance/interface/BranchID.h"
#include "DataFormats/Provenance/interface/ProductID.h"
#include "DataFormats/Provenance/interface/ThinnedAssociationsHelper.h"

namespace edm {
  class WrapperBase;

  namespace detail {
    constexpr unsigned int kThinningDoNotLookForThisIndex = std::numeric_limits<unsigned int>::max();

    class ThinnedOrSlimmedProduct {
    public:
      ThinnedOrSlimmedProduct() = default;
      explicit ThinnedOrSlimmedProduct(bool thinnedAvailable) : thinnedAvailable_{thinnedAvailable} {}
      explicit ThinnedOrSlimmedProduct(WrapperBase const* thinned, unsigned int key)
          : thinnedProduct_{thinned}, thinnedKey_{key}, thinnedAvailable_{true} {}
      explicit ThinnedOrSlimmedProduct(ThinnedAssociation const* slimmed, unsigned int key)
          : slimmedAssociation_{slimmed}, thinnedKey_{key} {}

      bool thinnedAvailable() const { return thinnedAvailable_; }
      bool hasThinned() const { return thinnedProduct_ != nullptr; }
      bool hasSlimmed() const { return slimmedAssociation_ != nullptr; }

      std::tuple<WrapperBase const*, unsigned int> thinnedProduct() const {
        return std::tuple(thinnedProduct_, thinnedKey_);
      }

      std::tuple<ThinnedAssociation const*, unsigned int> slimmedAssociation() const {
        return std::tuple(slimmedAssociation_, thinnedKey_);
      }

    private:
      WrapperBase const* thinnedProduct_ = nullptr;
      ThinnedAssociation const* slimmedAssociation_ = nullptr;
      unsigned int thinnedKey_ = 0;
      bool thinnedAvailable_ = false;
    };

    class SlimmedProducts {
    public:
      SlimmedProducts() = default;
      explicit SlimmedProducts(bool thinnedAvailable) : thinnedAvailable_{thinnedAvailable} {}
      explicit SlimmedProducts(ThinnedAssociation const* slimmed, std::vector<unsigned int> keys)
          : slimmedAssociation_{slimmed}, slimmedKeys_{std::move(keys)} {}

      SlimmedProducts(SlimmedProducts const&) = delete;
      SlimmedProducts& operator=(SlimmedProducts const&) = delete;
      SlimmedProducts(SlimmedProducts&&) = default;
      SlimmedProducts& operator=(SlimmedProducts&&) = default;

      bool thinnedAvailable() const { return thinnedAvailable_; }
      bool hasSlimmed() const { return slimmedAssociation_ != nullptr; }

      std::tuple<ThinnedAssociation const*, std::vector<unsigned int>> moveSlimmedAssociation() {
        return std::tuple(slimmedAssociation_, std::move(slimmedKeys_));
      }

    private:
      ThinnedAssociation const* slimmedAssociation_ = nullptr;
      std::vector<unsigned int> slimmedKeys_;
      bool thinnedAvailable_ = false;
    };

    template <typename F1, typename F2, typename F3>
    ThinnedOrSlimmedProduct getThinnedOnlyProduct(ProductID const& pid,
                                                  unsigned int key,
                                                  ThinnedAssociationsHelper const& thinnedAssociationsHelper,
                                                  F1&& pidToBid,
                                                  F2&& getThinnedAssociation,
                                                  F3&& getByProductID) {
      BranchID parent = pidToBid(pid);

      // Loop over thinned containers which were made by selecting elements from the parent container
      ThinnedAssociation const* slimmedAssociation = nullptr;
      unsigned int slimmedIndex = 0;
      bool thinnedAvailable = false;
      for (auto associatedBranches = thinnedAssociationsHelper.parentBegin(parent),
                iEnd = thinnedAssociationsHelper.parentEnd(parent);
           associatedBranches != iEnd;
           ++associatedBranches) {
        ThinnedAssociation const* thinnedAssociation = getThinnedAssociation(associatedBranches->association());
        if (thinnedAssociation == nullptr)
          continue;

        if (associatedBranches->parent() != pidToBid(thinnedAssociation->parentCollectionID())) {
          continue;
        }

        // Get the thinned container if it exists (need to check
        // before the key because of constraints on slimmed
        // containers
        ProductID const& thinnedCollectionPID = thinnedAssociation->thinnedCollectionID();
        WrapperBase const* thinnedCollection = getByProductID(thinnedCollectionPID);
        if (thinnedCollection != nullptr and not associatedBranches->isSlimmed()) {
          thinnedAvailable = true;
        }

        unsigned int thinnedIndex = 0;
        // Does this thinned container have the element referenced by key?
        // If yes, thinnedIndex is set to point to it in the thinned container
        if (!thinnedAssociation->hasParentIndex(key, thinnedIndex)) {
          continue;
        }

        // if the thinned container is also slimmed, store the
        // association for possible later use and ignore for now
        if (associatedBranches->isSlimmed()) {
          assert(slimmedAssociation == nullptr);
          slimmedAssociation = thinnedAssociation;
          slimmedIndex = thinnedIndex;
          continue;
        }

        // Return a pointer to thinned container if we can find it
        if (thinnedCollection != nullptr) {
          return ThinnedOrSlimmedProduct(thinnedCollection, thinnedIndex);
        }

        // Thinned container is not found, try looking recursively in thinned containers
        // which were made by selecting elements from this thinned container.
        auto thinnedOrSlimmed = getThinnedOnlyProduct(thinnedCollectionPID,
                                                      thinnedIndex,
                                                      thinnedAssociationsHelper,
                                                      std::forward<F1>(pidToBid),
                                                      std::forward<F2>(getThinnedAssociation),
                                                      std::forward<F3>(getByProductID));
        if (thinnedOrSlimmed.hasThinned()) {
          return thinnedOrSlimmed;
        } else if (thinnedOrSlimmed.thinnedAvailable()) {
          thinnedAvailable = true;
        } else if (thinnedOrSlimmed.hasSlimmed()) {
          assert(slimmedAssociation == nullptr);
          std::tie(slimmedAssociation, slimmedIndex) = thinnedOrSlimmed.slimmedAssociation();
        }
      }

      if (not thinnedAvailable and slimmedAssociation != nullptr) {
        return ThinnedOrSlimmedProduct(slimmedAssociation, slimmedIndex);
      }
      return ThinnedOrSlimmedProduct(thinnedAvailable);
    }

    // the return value is to a slimmed collection in case one is found
    template <typename F1, typename F2, typename F3>
    SlimmedProducts getThinnedOnlyProducts(ProductID const& pid,
                                           ThinnedAssociationsHelper const& thinnedAssociationsHelper,
                                           F1&& pidToBid,
                                           F2&& getThinnedAssociation,
                                           F3&& getByProductID,
                                           std::vector<WrapperBase const*>& foundContainers,
                                           std::vector<unsigned int>& keys) {
      BranchID parent = pidToBid(pid);

      // Loop over thinned containers which were made by selecting elements from the parent container
      ThinnedAssociation const* slimmedAssociation = nullptr;
      std::vector<unsigned int> slimmedIndexes;
      bool thinnedAvailable = false;
      for (auto associatedBranches = thinnedAssociationsHelper.parentBegin(parent),
                iEnd = thinnedAssociationsHelper.parentEnd(parent);
           associatedBranches != iEnd;
           ++associatedBranches) {
        ThinnedAssociation const* thinnedAssociation = getThinnedAssociation(associatedBranches->association());
        if (thinnedAssociation == nullptr)
          continue;

        if (associatedBranches->parent() != pidToBid(thinnedAssociation->parentCollectionID())) {
          continue;
        }

        // Get the thinned container if it exists (need to check
        // before the key because of constraints on slimmed
        // containers
        ProductID thinnedCollectionPID = thinnedAssociation->thinnedCollectionID();
        WrapperBase const* thinnedCollection = getByProductID(thinnedCollectionPID);
        if (thinnedCollection != nullptr and not associatedBranches->isSlimmed()) {
          thinnedAvailable = true;
        }

        unsigned nKeys = keys.size();
        std::vector<unsigned int> thinnedIndexes(nKeys, kThinningDoNotLookForThisIndex);
        bool hasAny = false;
        for (unsigned k = 0; k < nKeys; ++k) {
          // Already found this one
          if (foundContainers[k] != nullptr) {
            continue;
          }
          // Already know this one is not in this thinned container
          if (keys[k] == kThinningDoNotLookForThisIndex) {
            continue;
          }
          // Does the thinned container hold the entry of interest?
          // Modifies thinnedIndexes[k] only if it returns true and
          // sets it to the index in the thinned collection.
          if (thinnedAssociation->hasParentIndex(keys[k], thinnedIndexes[k])) {
            hasAny = true;
          }
        }
        if (!hasAny) {
          continue;
        }

        // if the thinned container is also slimmed, store the
        // association and the keys to thinned for possible later use
        // and ignore for now
        if (associatedBranches->isSlimmed()) {
          assert(slimmedAssociation == nullptr);
          slimmedAssociation = thinnedAssociation;
          slimmedIndexes = std::move(thinnedIndexes);
          continue;
        }

        // Set the pointers and indexes into the thinned container (if we can find it)
        if (thinnedCollection == nullptr) {
          // Thinned container is not found, try looking recursively in thinned containers
          // which were made by selecting elements from this thinned container.
          auto slimmed = getThinnedOnlyProducts(thinnedCollectionPID,
                                                thinnedAssociationsHelper,
                                                std::forward<F1>(pidToBid),
                                                std::forward<F2>(getThinnedAssociation),
                                                std::forward<F3>(getByProductID),
                                                foundContainers,
                                                thinnedIndexes);
          if (slimmed.thinnedAvailable()) {
            thinnedAvailable = true;
          } else if (slimmed.hasSlimmed()) {
            assert(slimmedAssociation == nullptr);
            std::tie(slimmedAssociation, slimmedIndexes) = slimmed.moveSlimmedAssociation();
          }
          for (unsigned k = 0; k < nKeys; ++k) {
            if (foundContainers[k] == nullptr)
              continue;
            if (thinnedIndexes[k] == kThinningDoNotLookForThisIndex)
              continue;
            keys[k] = thinnedIndexes[k];
          }
        } else {
          for (unsigned k = 0; k < nKeys; ++k) {
            if (thinnedIndexes[k] == kThinningDoNotLookForThisIndex)
              continue;
            keys[k] = thinnedIndexes[k];
            foundContainers[k] = thinnedCollection;
          }
        }
      }
      if (not thinnedAvailable and slimmedAssociation != nullptr) {
        return SlimmedProducts(slimmedAssociation, std::move(slimmedIndexes));
      }
      return SlimmedProducts(thinnedAvailable);
    }

    // This function provides a common implementation of
    // EDProductGetter::getThinnedProduct() for EventPrincipal,
    // DataGetterHelper, and BareRootProductGetter.
    //
    // getThinnedProduct assumes getIt was already called and failed to find
    // the product. The input key is the index of the desired element in the
    // container identified by ProductID (which cannot be found).
    // If the return value is not null, then the desired element was
    // found in a thinned container. If the desired element is not
    // found, then an optional without a value is returned.
    template <typename F1, typename F2, typename F3>
    std::optional<std::tuple<WrapperBase const*, unsigned int>> getThinnedProduct(
        ProductID const& pid,
        unsigned int key,
        ThinnedAssociationsHelper const& thinnedAssociationsHelper,
        F1&& pidToBid,
        F2&& getThinnedAssociation,
        F3&& getByProductID) {
      auto thinnedOrSlimmed = getThinnedOnlyProduct(pid,
                                                    key,
                                                    thinnedAssociationsHelper,
                                                    std::forward<F1>(pidToBid),
                                                    std::forward<F2>(getThinnedAssociation),
                                                    std::forward<F3>(getByProductID));

      if (thinnedOrSlimmed.hasThinned()) {
        return thinnedOrSlimmed.thinnedProduct();
      } else if (thinnedOrSlimmed.hasSlimmed()) {
        auto [slimmedAssociation, slimmedIndex] = thinnedOrSlimmed.slimmedAssociation();
        ProductID const& slimmedCollectionPID = slimmedAssociation->thinnedCollectionID();
        WrapperBase const* slimmedCollection = getByProductID(slimmedCollectionPID);
        if (slimmedCollection == nullptr) {
          // Slimmed container is not found, try looking recursively in thinned containers
          // which were made by selecting elements from this thinned container.
          return getThinnedProduct(slimmedCollectionPID,
                                   slimmedIndex,
                                   thinnedAssociationsHelper,
                                   std::forward<F1>(pidToBid),
                                   std::forward<F2>(getThinnedAssociation),
                                   std::forward<F3>(getByProductID));
        }
        return std::tuple(slimmedCollection, slimmedIndex);
      }
      return std::nullopt;
    }

    // This function provides a common implementation of
    // EDProductGetter::getThinnedProducts() for EventPrincipal,
    // DataGetterHelper, and BareRootProductGetter.
    //
    // getThinnedProducts assumes getIt was already called and failed to find
    // the product. The input keys are the indexes into the container identified
    // by ProductID (which cannot be found). On input the WrapperBase pointers
    // must all be set to nullptr (except when the function calls itself
    // recursively where non-null pointers mark already found elements).
    // Thinned containers derived from the product are searched to see
    // if they contain the desired elements. For each that is
    // found, the corresponding WrapperBase pointer is set and the key
    // is modified to be the key into the container where the element
    // was found. The WrapperBase pointers might or might not all point
    // to the same thinned container.
    template <typename F1, typename F2, typename F3>
    void getThinnedProducts(ProductID const& pid,
                            ThinnedAssociationsHelper const& thinnedAssociationsHelper,
                            F1&& pidToBid,
                            F2&& getThinnedAssociation,
                            F3&& getByProductID,
                            std::vector<WrapperBase const*>& foundContainers,
                            std::vector<unsigned int>& keys) {
      auto slimmed = getThinnedOnlyProducts(pid,
                                            thinnedAssociationsHelper,
                                            std::forward<F1>(pidToBid),
                                            std::forward<F2>(getThinnedAssociation),
                                            std::forward<F3>(getByProductID),
                                            foundContainers,
                                            keys);
      if (not slimmed.thinnedAvailable() and slimmed.hasSlimmed()) {
        // no thinned procucts found, try out slimmed next if one is available
        auto [slimmedAssociation, slimmedIndexes] = slimmed.moveSlimmedAssociation();
        ProductID const& slimmedCollectionPID = slimmedAssociation->thinnedCollectionID();
        WrapperBase const* slimmedCollection = getByProductID(slimmedCollectionPID);
        unsigned const nKeys = keys.size();
        if (slimmedCollection == nullptr) {
          getThinnedProducts(slimmedCollectionPID,
                             thinnedAssociationsHelper,
                             std::forward<F1>(pidToBid),
                             std::forward<F2>(getThinnedAssociation),
                             std::forward<F3>(getByProductID),
                             foundContainers,
                             slimmedIndexes);
          for (unsigned k = 0; k < nKeys; ++k) {
            if (foundContainers[k] == nullptr)
              continue;
            if (slimmedIndexes[k] == kThinningDoNotLookForThisIndex)
              continue;
            keys[k] = slimmedIndexes[k];
          }
        } else {
          for (unsigned k = 0; k < nKeys; ++k) {
            if (slimmedIndexes[k] == kThinningDoNotLookForThisIndex)
              continue;
            keys[k] = slimmedIndexes[k];
            foundContainers[k] = slimmedCollection;
          }
        }
      }
    }

  }  // namespace detail
}  // namespace edm

#endif
