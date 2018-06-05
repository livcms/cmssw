#include "DataFormats/Common/interface/DetSetVectorNew.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/SiPixelCluster/interface/SiPixelCluster.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHitCollection.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "HeterogeneousCore/CUDACore/interface/GPUCuda.h"
#include "HeterogeneousCore/Product/interface/HeterogeneousProduct.h"
#include "HeterogeneousCore/Producer/interface/HeterogeneousEDProducer.h"
#include "RecoLocalTracker/SiPixelRecHits/interface/PixelCPEBase.h"
#include "RecoLocalTracker/SiPixelRecHits/interface/PixelCPEFast.h"
#include "RecoLocalTracker/Records/interface/TkPixelCPERecord.h"

#include "EventFilter/SiPixelRawToDigi/plugins/siPixelRawToClusterHeterogeneousProduct.h" // TODO: we need a proper place for this header...

#include "PixelRecHits.h"

class SiPixelRecHitHeterogeneous: public HeterogeneousEDProducer<heterogeneous::HeterogeneousDevices <
                                                                   heterogeneous::GPUCuda,
                                                                   heterogeneous::CPU
                                                                   > > {

public:
  using Input = siPixelRawToClusterHeterogeneousProduct::HeterogeneousDigiCluster;

  explicit SiPixelRecHitHeterogeneous(const edm::ParameterSet& iConfig);
  ~SiPixelRecHitHeterogeneous() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  // CPU implementation
  void produceCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) override;

  // GPU implementation
  void beginStreamGPUCuda(edm::StreamID streamId, cuda::stream_t<>& cudaStream) override;
  void acquireGPUCuda(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) override;
  void produceGPUCuda(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) override;

    // Commonalities
  void initialize(const edm::EventSetup& es);

  // CPU
  void run(const edm::Handle<SiPixelClusterCollectionNew>& inputhandle, SiPixelRecHitCollectionNew &output) const;
  // GPU
  void run(const edm::Handle<SiPixelClusterCollectionNew>& inputhandle, SiPixelRecHitCollectionNew &output, const pixelgpudetails::HitsOnCPU& hoc) const;

  edm::EDGetTokenT<HeterogeneousProduct> token_;
  edm::EDGetTokenT<SiPixelClusterCollectionNew> clusterToken_;
  std::string cpeName_;

  const TrackerGeometry *geom_ = nullptr;
  const PixelClusterParameterEstimator *cpe_ = nullptr;

  std::unique_ptr<pixelgpudetails::PixelRecHitGPUKernel> gpuAlgo_;
};

SiPixelRecHitHeterogeneous::SiPixelRecHitHeterogeneous(const edm::ParameterSet& iConfig):
  HeterogeneousEDProducer(iConfig),
  token_(consumesHeterogeneous(iConfig.getParameter<edm::InputTag>("heterogeneousSrc"))),
  clusterToken_(consumes<SiPixelClusterCollectionNew>(iConfig.getParameter<edm::InputTag>("src"))),
  cpeName_(iConfig.getParameter<std::string>("CPE"))
{
  produces<SiPixelRecHitCollectionNew>();
}

void SiPixelRecHitHeterogeneous::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;

  desc.add<edm::InputTag>("heterogeneousSrc", edm::InputTag("siPixelDigisHeterogeneous"));
  desc.add<edm::InputTag>("src", edm::InputTag("siPixelClusters"));
  desc.add<std::string>("CPE", "PixelCPEFast");

  HeterogeneousEDProducer::fillPSetDescription(desc);

  descriptions.add("siPixelRecHitHeterogeneous",desc);
}

void SiPixelRecHitHeterogeneous::initialize(const edm::EventSetup& es) {
  edm::ESHandle<TrackerGeometry> geom;
  es.get<TrackerDigiGeometryRecord>().get( geom );
  geom_ = geom.product();

  edm::ESHandle<PixelClusterParameterEstimator> hCPE;
  es.get<TkPixelCPERecord>().get(cpeName_, hCPE);
  cpe_ = dynamic_cast< const PixelCPEBase* >(hCPE.product());
}

void SiPixelRecHitHeterogeneous::produceCPU(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup) {
  initialize(iSetup);

  edm::Handle<SiPixelClusterCollectionNew> hclusters;
  iEvent.getByToken(clusterToken_, hclusters);

  auto output = std::make_unique<SiPixelRecHitCollectionNew>();
  run(hclusters, *output);

  output->shrink_to_fit();
  iEvent.put(std::move(output));
}

void SiPixelRecHitHeterogeneous::run(const edm::Handle<SiPixelClusterCollectionNew>& inputhandle, SiPixelRecHitCollectionNew &output) const {
  const auto& input = *inputhandle;

  edmNew::DetSetVector<SiPixelCluster>::const_iterator DSViter=input.begin();
  for ( ; DSViter != input.end() ; DSViter++) {
    unsigned int detid = DSViter->detId();
    DetId detIdObject( detid );
    const GeomDetUnit * genericDet = geom_->idToDetUnit( detIdObject );
    const PixelGeomDetUnit * pixDet = dynamic_cast<const PixelGeomDetUnit*>(genericDet);
    assert(pixDet); 
    SiPixelRecHitCollectionNew::FastFiller recHitsOnDetUnit(output,detid);

    edmNew::DetSet<SiPixelCluster>::const_iterator clustIt = DSViter->begin(), clustEnd = DSViter->end();

    for ( ; clustIt != clustEnd; clustIt++) {
      std::tuple<LocalPoint, LocalError,SiPixelRecHitQuality::QualWordType> tuple = cpe_->getParameters( *clustIt, *genericDet );
      LocalPoint lp( std::get<0>(tuple) );
      LocalError le( std::get<1>(tuple) );
      SiPixelRecHitQuality::QualWordType rqw( std::get<2>(tuple) );
      // Create a persistent edm::Ref to the cluster
      edm::Ref< edmNew::DetSetVector<SiPixelCluster>, SiPixelCluster > cluster = edmNew::makeRefTo( inputhandle, clustIt);
      // Make a RecHit and add it to the DetSet
      // old : recHitsOnDetUnit.push_back( new SiPixelRecHit( lp, le, detIdObject, &*clustIt) );
      SiPixelRecHit hit( lp, le, rqw, *genericDet, cluster);
      //
      // Now save it =================
      recHitsOnDetUnit.push_back(hit);
    } //  <-- End loop on Clusters
  } //    <-- End loop on DetUnits
}


void SiPixelRecHitHeterogeneous::beginStreamGPUCuda(edm::StreamID streamId, cuda::stream_t<>& cudaStream) {
  gpuAlgo_ = std::make_unique<pixelgpudetails::PixelRecHitGPUKernel>();
}

void SiPixelRecHitHeterogeneous::acquireGPUCuda(const edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) {
  initialize(iSetup);

  PixelCPEFast const * fcpe = dynamic_cast<const PixelCPEFast *>(cpe_);
  if (!fcpe) {
    throw cms::Exception("Configuration") << "too bad, not a fast cpe gpu processing not possible....";
  }

  edm::Handle<siPixelRawToClusterHeterogeneousProduct::GPUProduct> hinput;
  iEvent.getByToken<Input>(token_, hinput);

  gpuAlgo_->makeHitsAsync(*hinput, fcpe->getGPUProductAsync(cudaStream), cudaStream);
}

void SiPixelRecHitHeterogeneous::produceGPUCuda(edm::HeterogeneousEvent& iEvent, const edm::EventSetup& iSetup, cuda::stream_t<>& cudaStream) {
  const auto& hits = gpuAlgo_->getOutput(cudaStream);

  // Need the CPU clusters to
  // - properly fill the output DetSetVector of hits
  // - to set up edm::Refs to the clusters
  edm::Handle<SiPixelClusterCollectionNew> hclusters;
  iEvent.getByToken(clusterToken_, hclusters);

  auto output = std::make_unique<SiPixelRecHitCollectionNew>();
  run(hclusters, *output, hits);

  iEvent.put(std::move(output));
}

void SiPixelRecHitHeterogeneous::run(const edm::Handle<SiPixelClusterCollectionNew>& inputhandle, SiPixelRecHitCollectionNew &output, const pixelgpudetails::HitsOnCPU& hoc) const {
  auto const & input = *inputhandle;

  int numberOfDetUnits = 0;
  int numberOfClusters = 0;
  for (auto DSViter=input.begin(); DSViter != input.end() ; DSViter++) {
    numberOfDetUnits++;
    unsigned int detid = DSViter->detId();
    DetId detIdObject( detid );
    const GeomDetUnit * genericDet = geom_->idToDetUnit( detIdObject );
    auto gind = genericDet->index();
    const PixelGeomDetUnit * pixDet = dynamic_cast<const PixelGeomDetUnit*>(genericDet);
    assert(pixDet);
    SiPixelRecHitCollectionNew::FastFiller recHitsOnDetUnit(output, detid);
    auto fc = hoc.hitsModuleStart[gind];
    auto lc = hoc.hitsModuleStart[gind+1];
    auto nhits = lc-fc;
    int32_t ind[nhits];
    auto mrp = &hoc.mr[fc];
    uint32_t ngh=0;
    for (uint32_t i=0; i<nhits;++i) {
      if( hoc.charge[fc+i]<2000 || (gind>=96 && hoc.charge[fc+i]<4000) ) continue;
      ind[ngh]=i;std::push_heap(ind, ind+ngh+1,[&](auto a, auto b) { return mrp[a]<mrp[b];});
      ++ngh;
    }
    std::sort_heap(ind, ind+ngh,[&](auto a, auto b) { return mrp[a]<mrp[b];});
    uint32_t ic=0;
    assert(ngh==DSViter->size());
    for (auto const & clust : *DSViter) {
      assert(ic<ngh);
      // order is not stable... assume charge to be unique...
      auto ij = fc+ind[ic];
      // assert( clust.minPixelRow()==hoc.mr[ij] );
      if( clust.minPixelRow()!=hoc.mr[ij] )
        edm::LogWarning("GPUHits2CPU") <<"IMPOSSIBLE "
                                       << gind <<'/'<<fc<<'/'<<ic<<'/'<<ij << ' ' << clust.charge()<<"/"<<hoc.charge[ij]
                                       << ' ' << clust.minPixelRow()<<"!="<< mrp[ij] << std::endl;

      if(clust.charge()!=hoc.charge[ij]) {
        auto fd=false;
        auto k = ij;
        while (clust.minPixelRow()==hoc.mr[++k]) if(clust.charge()==hoc.charge[k]) {fd=true; break;}
        if (!fd) {
          k = ij;
          while (clust.minPixelRow()==hoc.mr[--k])  if(clust.charge()==hoc.charge[k]) {fd=true; break;}
        }
        // assert(fd && k!=ij);
        if(fd) ij=k;
      }
      if(clust.charge()!=hoc.charge[ij])
        edm::LogWarning("GPUHits2CPU") << "perfect Match not found "
                                       << gind <<'/'<<fc<<'/'<<ic<<'/'<<ij << ' ' << clust.charge()<<"!="<<hoc.charge[ij]
                                       << ' ' << clust.minPixelRow()<<'/'<< mrp[ij] <<'/'<< mrp[fc+ind[ic]] << std::endl;

      LocalPoint lp(hoc.xl[ij], hoc.yl[ij]);
      LocalError le(hoc.xe[ij]*hoc.xe[ij], 0, hoc.ye[ij]*hoc.ye[ij]);
      SiPixelRecHitQuality::QualWordType rqw=0;

      ++ic;
      numberOfClusters++;

      /*   cpu version....  (for reference)
           std::tuple<LocalPoint, LocalError, SiPixelRecHitQuality::QualWordType> tuple = cpe_->getParameters( clust, *genericDet );
           LocalPoint lp( std::get<0>(tuple) );
           LocalError le( std::get<1>(tuple) );
           SiPixelRecHitQuality::QualWordType rqw( std::get<2>(tuple) );
      */

      // Create a persistent edm::Ref to the cluster
      edm::Ref< edmNew::DetSetVector<SiPixelCluster>, SiPixelCluster > cluster = edmNew::makeRefTo( inputhandle, &clust);
      // Make a RecHit and add it to the DetSet
      SiPixelRecHit hit( lp, le, rqw, *genericDet, cluster);
      //
      // Now save it =================
      recHitsOnDetUnit.push_back(hit);
      // =============================

      // std::cout << "SiPixelRecHitGPUVI " << numberOfClusters << ' '<< lp << " " << le << std::endl;

    } //  <-- End loop on Clusters


      //  LogDebug("SiPixelRecHitGPU")
      //std::cout << "SiPixelRecHitGPUVI "
      //	<< " Found " << recHitsOnDetUnit.size() << " RecHits on " << detid //;
      // << std::endl;


  } //    <-- End loop on DetUnits
}

DEFINE_FWK_MODULE(SiPixelRecHitHeterogeneous);
