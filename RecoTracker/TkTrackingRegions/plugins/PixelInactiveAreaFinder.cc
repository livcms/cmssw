#include "PixelInactiveAreaFinder.h"

#include "FWCore/Utilities/interface/VecArray.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "CondFormats/DataRecord/interface/SiPixelQualityRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"

#include "TrackingTools/TransientTrackingRecHit/interface/SeedingLayerSetsLooper.h"

#include <fstream>
#include <queue>
#include <algorithm>

std::ostream& operator<<(std::ostream& os, SeedingLayerSetsBuilder::SeedingLayerId layer) {
  if(std::get<0>(layer) == GeomDetEnumerators::PixelBarrel) {
    os << "BPix";
  }
  else {
    os << "FPix";
  }
  os << std::get<2>(layer);
  if(std::get<1>(layer) == TrackerDetSide::PosEndcap) {
    os << "_pos";
  }
  else if(std::get<1>(layer) == TrackerDetSide::NegEndcap) {
    os << "_neg";
  }
  return os;
}

namespace {
  using LayerPair = std::pair<SeedingLayerSetsBuilder::SeedingLayerId, SeedingLayerSetsBuilder::SeedingLayerId>;
  using ActiveLayerSetToInactiveSetsMap = std::map<LayerPair, edm::VecArray<LayerPair, 5> >;

  ActiveLayerSetToInactiveSetsMap createActiveToInactiveMap() {
    ActiveLayerSetToInactiveSetsMap map;

    auto bpix = [](int layer) {
      return SeedingLayerSetsBuilder::SeedingLayerId(GeomDetEnumerators::PixelBarrel, TrackerDetSide::Barrel, layer);
    };
    auto fpix_pos = [](int disk) {
      return SeedingLayerSetsBuilder::SeedingLayerId(GeomDetEnumerators::PixelEndcap, TrackerDetSide::PosEndcap, disk);
    };
    auto fpix_neg = [](int disk) {
      return SeedingLayerSetsBuilder::SeedingLayerId(GeomDetEnumerators::PixelEndcap, TrackerDetSide::NegEndcap, disk);
    };

    auto add_permutations = [&](std::array<SeedingLayerSetsBuilder::SeedingLayerId, 4> quads) {
      do {
        // skip permutations like BPix2+BPix1 or FPix1+BPix1
        // operator> works automatically
        if(quads[0] > quads[1] || quads[2] > quads[3]) continue;

        map[std::make_pair(quads[0], quads[1])].emplace_back(quads[2], quads[3]);
      } while(std::next_permutation(quads.begin(), quads.end()));
    };

    // 4 barrel
    add_permutations({{bpix(1), bpix(2), bpix(3), bpix(4)}});

    // 3 barrel, 1 forward
    add_permutations({{bpix(1), bpix(2), bpix(3), fpix_pos(1)}});
    add_permutations({{bpix(1), bpix(2), bpix(3), fpix_neg(1)}});

    // 2 barrel, 2 forward
    add_permutations({{bpix(1), bpix(2), fpix_pos(1), fpix_pos(2)}});
    add_permutations({{bpix(1), bpix(2), fpix_neg(1), fpix_neg(2)}});

    // 1 barrel, 3 forward
    add_permutations({{bpix(1), fpix_pos(1), fpix_pos(2), fpix_pos(3)}});
    add_permutations({{bpix(1), fpix_neg(1), fpix_neg(2), fpix_neg(3)}});

#ifdef EDM_ML_DEBUG
    LogDebug("PixelInactiveAreaFinder") << "Active to inactive mapping";
    for(const auto& elem: map) {
      std::stringstream ss;
      for(const auto& layerPair: elem.second) {
        ss << layerPair.first << "+" << layerPair.second << ",";
      }
      LogTrace("PixelInactiveAreaFinder") << " " << elem.first.first << "+" << elem.first.second << " => " << ss.str();
    }
#endif

    return map;
  }

}

PixelInactiveAreaFinder::PixelInactiveAreaFinder(const edm::ParameterSet& iConfig, const std::vector<SeedingLayerSetsBuilder::SeedingLayerId>& seedingLayers,
                                                 const SeedingLayerSetsLooper& seedingLayerSetsLooper):
  debug_(iConfig.getUntrackedParameter<bool>("debug")),
  createPlottingFiles_(iConfig.getUntrackedParameter<bool>("createPlottingFiles"))
{
#ifdef EDM_ML_DEBUG
  for(const auto& layer: seedingLayers) {
    LogTrace("PixelInactiveAreaFinder") << "Input layer subdet " << std::get<0>(layer) << " side " << std::get<1>(layer) << " layer " << std::get<2>(layer);
  }
#endif

  auto findOrAdd = [&](SeedingLayerId layer) -> unsigned short {
    auto found = std::find(layers_.cbegin(), layers_.cend(), layer);
    if(found == layers_.cend()) {
      auto ret = layers_.size();
      layers_.push_back(layer);
      return ret;
    }
    return std::distance(layers_.cbegin(), found);
  };

  // mapping from active layer pairs to inactive layer pairs
  const auto activeToInactiveMap = createActiveToInactiveMap();

  // convert input layer pairs (that are for active layers) to layer
  // pairs to look for inactive areas
  for(const auto& layerSet: seedingLayerSetsLooper.makeRange(seedingLayers)) {
    assert(layerSet.size() == 2);
    auto found = activeToInactiveMap.find(std::make_pair(layerSet[0], layerSet[1]));
    if(found == activeToInactiveMap.end()) {
      throw cms::Exception("Configuration") << "Encountered layer pair " << layerSet[0] << "+" << layerSet[1] << " not found from the internal 'active layer pairs' to 'inactive layer pairs' mapping; either fix the input or the mapping (in PixelInactiveAreaFinder.cc)";
    }

    LogTrace("PixelInactiveAreaFinder") << "Input layer set " << layerSet[0] << "+" << layerSet[1];
    for(const auto& inactiveLayerSet: found->second) {
      auto innerInd = findOrAdd(inactiveLayerSet.first);
      auto outerInd = findOrAdd(inactiveLayerSet.second);

      auto found = std::find(layerSetIndices_.cbegin(), layerSetIndices_.cend(), std::make_pair(innerInd, outerInd));
      if(found == layerSetIndices_.end()) {
        layerSetIndices_.emplace_back(innerInd, outerInd);
      }

      LogTrace("PixelInactiveAreaFinder") << " inactive layer set " << inactiveLayerSet.first << "+" << inactiveLayerSet.second;
    }
  }
  

#ifdef EDM_ML_DEBUG
  LogDebug("PixelInactiveAreaFinder") << "All inactive layer sets";
  for(const auto& idxPair: layerSetIndices_) {
    LogTrace("PixelInactiveAreaFinder") << " " << layers_[idxPair.first] << "+" << layers_[idxPair.second];
  }
#endif
}

void PixelInactiveAreaFinder::fillDescriptions(edm::ParameterSetDescription& desc) {
  desc.addUntracked<bool>("debug", false);
  desc.addUntracked<bool>("createPlottingFiles", false);
}


std::vector<PixelInactiveAreaFinder::AreaLayers>
PixelInactiveAreaFinder::inactiveAreas(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  std::vector<PixelInactiveAreaFinder::AreaLayers> ret;

  // Set data to handles
  {
    edm::ESHandle<SiPixelQuality> pixelQuality;
    iSetup.get<SiPixelQualityRcd>().get(pixelQuality);
    pixelQuality_ = pixelQuality.product();

    edm::ESHandle<TrackerGeometry> trackerGeometry;
    iSetup.get<TrackerDigiGeometryRecord>().get(trackerGeometry);
    trackerGeometry_ = trackerGeometry.product();

    edm::ESHandle<TrackerTopology> trackerTopology;
    iSetup.get<TrackerTopologyRcd>().get(trackerTopology);
    trackerTopology_ = trackerTopology.product();
  }

  // assign data to instance variables
  this->getBadPixelDets();

  //write files for plotting
  if(createPlottingFiles_) {
    updatePixelDets(iSetup);
    createPlottingFiles();
  }

  // Comparing
  if(debug_) {
    this->printOverlapSpans();
  }

  // add conversion to output

  return ret;
}

// Functions for fetching date from handles
void PixelInactiveAreaFinder::updatePixelDets(const edm::EventSetup& iSetup) {
  if(!geometryWatcher_.check(iSetup))
    return;

  pixelDetsBarrel.clear();
  pixelDetsEndcap.clear();

  for(auto const & geomDetPtr : trackerGeometry_->detsPXB() ) {
    if(geomDetPtr->geographicalId().subdetId() == PixelSubdetector::PixelBarrel){
      pixelDetsBarrel.push_back(geomDetPtr->geographicalId().rawId());
    }
  }
  for(auto const & geomDetPtr : trackerGeometry_->detsPXF() ) {
    if(geomDetPtr->geographicalId().subdetId() == PixelSubdetector::PixelEndcap){
      pixelDetsEndcap.push_back(geomDetPtr->geographicalId().rawId());
    }
  }
  std::sort(pixelDetsBarrel.begin(),pixelDetsBarrel.end());
  std::sort(pixelDetsEndcap.begin(),pixelDetsEndcap.end());
}
void PixelInactiveAreaFinder::getBadPixelDets(){
  for(auto const & disabledModule : pixelQuality_->getBadComponentList() ){
    if( DetId(disabledModule.DetID).subdetId() == PixelSubdetector::PixelBarrel ){
      badPixelDetsBarrel.push_back( disabledModule.DetID );
    } else if ( DetId(disabledModule.DetID).subdetId() == PixelSubdetector::PixelEndcap ){
      badPixelDetsEndcap.push_back( disabledModule.DetID );
    }
  }
  std::sort(badPixelDetsBarrel.begin(),badPixelDetsBarrel.end());
  std::sort(badPixelDetsEndcap.begin(),badPixelDetsEndcap.end());
  badPixelDetsBarrel.erase(
                           std::unique(badPixelDetsBarrel.begin(),badPixelDetsBarrel.end()),badPixelDetsBarrel.end());
  badPixelDetsEndcap.erase(
                           std::unique(badPixelDetsEndcap.begin(),badPixelDetsEndcap.end()),badPixelDetsEndcap.end());
}
// Printing functions
void PixelInactiveAreaFinder::detInfo(const det_t & det, Stream & ss){
  using std::tie;
  using std::setw;
  using std::showpos;
  using std::noshowpos;
  using std::fixed;
  using std::right;
  using std::left;
  using std::setprecision;
  using std::setfill;
  std::string deli = "; ";
  ss << "id:[" << det << "]" <<deli;
  ss << "subdetid:[" << DetId(det).subdetId() << "]" << deli;
  if(DetId(det).subdetId()==PixelSubdetector::PixelBarrel){
    unsigned int layer  = trackerTopology_->pxbLayer (DetId(det));
    unsigned int ladder = trackerTopology_->pxbLadder(DetId(det));
    unsigned int module = trackerTopology_->pxbModule(DetId(det));
    ss  << "layer:["                      << layer  << "]" << deli 
        << "ladder:[" << right << setw(2) << ladder << "]" << deli 
        << "module:["                     << module << "]" << deli;
  }else if(DetId(det).subdetId()==PixelSubdetector::PixelEndcap){
    unsigned int disk  = trackerTopology_->pxfDisk (DetId(det));
    unsigned int blade = trackerTopology_->pxfBlade(DetId(det));
    unsigned int panel = trackerTopology_->pxfPanel(DetId(det));
    ss  << left << setw(6) << "disk:"  << "["            << right << disk  << "]" << deli 
        << left << setw(7) << "blade:" << "[" << setw(2) << right << blade << "]" << deli 
        << left << setw(7) << "panel:" << "["            << right << panel << "]" << deli;
  }
  float phiA,phiB,zA,zB,rA,rB;
  auto detSurface = trackerGeometry_->idToDet(DetId(det))->surface();
  tie(phiA,phiB) = detSurface.phiSpan();
  tie(zA,zB) = detSurface.zSpan();
  tie(rA,rB) = detSurface.rSpan();
  ss 
    << setprecision(16) 
    << fixed 
    << showpos
    << setfill(' ')
    << "phi:[" << right << setw(12) << phiA << "," << left << setw(12) << phiB << "]" << deli
    << "z:["   << right << setw(7)  << zA   << "," << left << setw(7)  << zB   << "]" << deli << noshowpos
    << "r:["   << right << setw(10) << rA   << "," << left << setw(10) << rB   << "]" << deli;

}
void PixelInactiveAreaFinder::detGroupSpanInfo(const DetGroupSpan & cspan, Stream & ss){
  using std::showpos;
  using std::noshowpos;
  using std::fixed;
  using std::setprecision;
  using std::setw;
  using std::setfill;
  using std::left;
  using std::right;
  std::string deli = "; ";
  ss  << "subdetid:[" << cspan.subdetId << "]" << deli;
  if(cspan.subdetId == PixelSubdetector::PixelBarrel) {
    ss << "layer:[" << cspan.layer << "]" << deli;
  }
  else {
    ss << "disk:[" << cspan.disk << "]" << deli;
  }
  ss
    //<< setfill(' ') << setw(36) << " "
      << setprecision(16)
      << showpos
      << "phi:<" << right << setw(12) << cspan.phiSpan.first << "," << left << setw(12) << cspan.phiSpan.second << ">" << deli
      << "z:<" << right << setw(7) << cspan.zSpan.first << "," << left << setw(7) << cspan.zSpan.second << ">" << deli << noshowpos
      << "r:<" << right << setw(10) << cspan.rSpan.first << "," << left << setw(10) << cspan.rSpan.second << ">" << deli
    ; 
}
void PixelInactiveAreaFinder::printPixelDets(){
  edm::LogPrint("") << "Barrel detectors:";
  Stream ss;
  for(auto const & det : pixelDetsBarrel){
    detInfo(det,ss);
    edm::LogPrint("") << ss.str();ss.str(std::string());
  }
  edm::LogPrint("") << "Endcap detectors;";
  for(auto const & det : pixelDetsEndcap){
    detInfo(det,ss);
    edm::LogPrint("") << ss.str();ss.str(std::string());
  }
}
void PixelInactiveAreaFinder::printBadPixelDets(){
  edm::LogPrint("") << "Bad barrel detectors:";
  Stream ss;
  for(auto const & det : badPixelDetsBarrel){
    detInfo(det,ss);
    edm::LogPrint("") << ss.str();ss.str(std::string());
  }
  edm::LogPrint("") << "Endcap detectors;";
  for(auto const & det : badPixelDetsEndcap){
    detInfo(det,ss);
    edm::LogPrint("") << ss.str();ss.str(std::string());
  }
}
void PixelInactiveAreaFinder::printBadDetGroups(){
  DetGroupContainer badDetGroupsBar = badDetGroupsBarrel();
  DetGroupContainer badDetGroupsEnd = badDetGroupsEndcap();
  Stream ss;
  for(auto const & detGroup : badDetGroupsBar){
    ss << std::setfill(' ') << std::left << std::setw(16) << "DetGroup:";
    DetGroupSpan cspan;
    getPhiSpanBarrel(detGroup,cspan);
    getZSpan(detGroup,cspan);
    getRSpan(detGroup,cspan);
    detGroupSpanInfo(cspan,ss);
    ss<<std::endl;
    for(auto const & det : detGroup){
      detInfo(det,ss);ss<<std::endl;
    }
    ss<<std::endl;
  }
  for(auto const & detGroup : badDetGroupsEnd){
    ss << std::setfill(' ') << std::left << std::setw(16) << "DetGroup:";
    DetGroupSpan cspan;
    getPhiSpanEndcap(detGroup,cspan);
    getZSpan(detGroup,cspan);
    getRSpan(detGroup,cspan);
    detGroupSpanInfo(cspan,ss);
    ss << std::endl;
    for(auto const & det : detGroup){
      detInfo(det,ss);ss<<std::endl;
    }
    ss << std::endl;
  }
  edm::LogPrint("")<<ss.str();
}
void PixelInactiveAreaFinder::printBadDetGroupSpans(){
  DetGroupSpanContainerPair cspans = detGroupSpans();
  Stream ss;
  for(auto const & cspan : cspans.first){
    detGroupSpanInfo(cspan,ss);ss<<std::endl;
  }
  for(auto const & cspan : cspans.second){
    detGroupSpanInfo(cspan,ss);ss<<std::endl;
  }
  edm::LogPrint("") << ss.str();
}
void PixelInactiveAreaFinder::printOverlapSpans(){
  OverlapSpansContainer ospans = this->overlappingSpans();
  Stream ss;
  for(auto const & spans : ospans){
    ss << "Overlapping detGroups:\n";
    for(auto const cspan : spans){
      detGroupSpanInfo(cspan,ss);
      ss << std::endl;
    }
  }
  edm::LogPrint("") << ss.str();
}
void PixelInactiveAreaFinder::createPlottingFiles(){
  // All detectors to file DETECTORS
  Stream ss;
  std::ofstream fsDet("DETECTORS.txt");
  for(auto const & det : pixelDetsBarrel){
    detInfo(det,ss);
    ss << std::endl;
  }
  edm::LogPrint("") << "Endcap detectors;";
  for(auto const & det : pixelDetsEndcap){
    detInfo(det,ss);
    ss << std::endl;
  }
  fsDet << ss.rdbuf();
  ss.str(std::string());
  // Bad detectors
  std::ofstream fsBadDet("BADDETECTORS.txt");
  for(auto const & det : badPixelDetsBarrel){
    detInfo(det,ss);
    ss<<std::endl;
  }
  for(auto const & det : badPixelDetsEndcap){
    detInfo(det,ss);
    ss<<std::endl;
  }
  fsBadDet << ss.rdbuf();
  ss.str(std::string());
  // detgroupspans
  std::ofstream fsSpans("DETGROUPSPANS.txt");
  DetGroupSpanContainerPair cspans = detGroupSpans();
  for(auto const & cspan : cspans.first){
    detGroupSpanInfo(cspan,ss);ss<<std::endl;
  }
  for(auto const & cspan : cspans.second){
    detGroupSpanInfo(cspan,ss);ss<<std::endl;
  }
  fsSpans << ss.rdbuf();
  ss.str(std::string());

}
// Functions for finding bad detGroups
bool PixelInactiveAreaFinder::phiRangesOverlap(const float x1,const float x2, const float y1,const float y2){

  // assuming phi ranges are [x1,x2] and [y1,y2] and xi,yi in [-pi,pi]
  if(x1<=x2 && y1<=y2){
    return x1<=y2 && y1 <= x2;
  }else if (( x1>x2 && y1 <= y2) || (y1 > y2 && x1 <= x2 )){
    return y1 <= x2 || x1 <= y2;
  }else if (x1 > x2 && y1 > y2){
    return true;
  }else {
    return false;
  }
}
bool PixelInactiveAreaFinder::phiRangesOverlap(const Span_t & phiSpanA, const Span_t & phiSpanB){
  float x1,x2,y1,y2;
  std::tie(x1,x2) = phiSpanA;
  std::tie(y1,y2) = phiSpanB;
  // assuming phi ranges are [x1,x2] and [y1,y2] and xi,yi in [-pi,pi]
  if(x1<=x2 && y1<=y2){
    return x1<=y2 && y1 <= x2;
  }else if (( x1>x2 && y1 <= y2) || (y1 > y2 && x1 <= x2 )){
    return y1 <= x2 || x1 <= y2;
  }else if (x1 > x2 && y1 > y2){
    return true;
  }else {
    return false;
  }
}
bool PixelInactiveAreaFinder::detWorks(det_t det){
  return 
    std::find(badPixelDetsBarrel.begin(),badPixelDetsBarrel.end(),det)
    == badPixelDetsBarrel.end()
    &&
    std::find(badPixelDetsEndcap.begin(),badPixelDetsEndcap.end(),det)
    == badPixelDetsEndcap.end()
    ;
}
PixelInactiveAreaFinder::DetGroup PixelInactiveAreaFinder::badAdjecentDetsBarrel(const det_t & det){
  using std::remove_if;
  using std::bind1st;
  using std::mem_fun;

  DetGroup adj;
  auto const tTopo = trackerTopology_;
  auto const & detId = DetId(det);
  unsigned int layer  = tTopo->pxbLayer (detId);
  unsigned int ladder = tTopo->pxbLadder(detId);
  unsigned int module = tTopo->pxbModule(detId);
  unsigned int nLads;
  switch (layer){
  case 1:  nLads = nLayer1Ladders;break;
  case 2:  nLads = nLayer2Ladders;break;
  case 3:  nLads = nLayer3Ladders;break;
  case 4:  nLads = nLayer4Ladders;break;
  default: nLads = 0 ;break;
  }
  //add detectors from next and previous ladder
  adj.push_back( tTopo->pxbDetId( layer, ((ladder-1)+1)%nLads+1, module )() );
  adj.push_back( tTopo->pxbDetId( layer, ((ladder-1)-1+nLads)%nLads+1, module )() );
  //add adjecent detectors from same ladder
  switch (module){
  case 1:
    adj.push_back( tTopo->pxbDetId( layer, ladder, module+1 )() );
    break;
  case nModulesPerLadder:
    adj.push_back( tTopo->pxbDetId( layer, ladder, module-1 )() );
    break;
  default :
    adj.push_back( tTopo->pxbDetId( layer, ladder, module+1 )() );
    adj.push_back( tTopo->pxbDetId( layer, ladder, module-1 )() );
    break;
  }
  //remove working detectors from list
  adj.erase(remove_if(adj.begin(),adj.end(),bind1st(
                                                    mem_fun(&PixelInactiveAreaFinder::detWorks),this)),adj.end());
  return adj;
}
PixelInactiveAreaFinder::DetGroup PixelInactiveAreaFinder::badAdjecentDetsEndcap(const det_t & det){
  // this might be faster if adjecent 
  using std::tie;
  using std::ignore;
  DetGroup adj;
  Span_t  phiSpan, phiSpanComp;
  float z, zComp;
  unsigned int disk, diskComp;
  auto const & detSurf = trackerGeometry_->idToDet(DetId(det))->surface();
  phiSpan = detSurf.phiSpan();
  tie(z,ignore) = detSurf.zSpan();
  disk = trackerTopology_->pxfDisk(DetId(det));
  // add detectors from same disk whose phi ranges overlap to the adjecent list
  for(auto const & detComp : badPixelDetsEndcap){
    auto const & detIdComp = DetId(detComp);
    auto const & detSurfComp = trackerGeometry_->idToDet(detIdComp)->surface();
    diskComp = trackerTopology_->pxfDisk(detIdComp);
    phiSpanComp = detSurfComp.phiSpan();
    tie(zComp,ignore) = detSurfComp.zSpan();
    if(det != detComp && disk == diskComp && z*zComp > 0
       && phiRangesOverlap(phiSpan,phiSpanComp)){
      adj.push_back(detComp);
    }
  }
  return adj;
}
PixelInactiveAreaFinder::DetGroup PixelInactiveAreaFinder::reachableDetGroup(const det_t & initDet, DetectorSet & foundDets){
  DetGroup reachableDetGroup;
  std::queue<det_t> workQueue;
  det_t workDet;
  DetGroup badAdjDets;
  foundDets.insert(initDet);
  workQueue.push(initDet);
  reachableDetGroup.push_back(initDet);
  while(!workQueue.empty()){
    workDet = workQueue.front();workQueue.pop();
    if(DetId(workDet).subdetId() == PixelSubdetector::PixelBarrel){
      badAdjDets = this->badAdjecentDetsBarrel(workDet);
    }else if(DetId(workDet).subdetId() == PixelSubdetector::PixelEndcap){
      badAdjDets = this->badAdjecentDetsEndcap(workDet);
    }else {
      badAdjDets = {};
    }
    for(auto const & badDet : badAdjDets){
      if(foundDets.find(badDet) == foundDets.end()){
        reachableDetGroup.push_back(badDet);
        foundDets.insert(badDet);
        workQueue.push(badDet);
      }
    }
  }
  return reachableDetGroup;
}
PixelInactiveAreaFinder::DetGroupContainer PixelInactiveAreaFinder::badDetGroupsBarrel(){
  DetGroupContainer detGroups;
  DetectorSet foundDets;
  for(auto const & badDet : badPixelDetsBarrel){
    if(foundDets.find(badDet) == foundDets.end()){
      detGroups.push_back(this->reachableDetGroup(badDet,foundDets));
    } 
  }
  return detGroups;
}
PixelInactiveAreaFinder::DetGroupContainer PixelInactiveAreaFinder::badDetGroupsEndcap(){
  DetGroupContainer detGroups;
  DetectorSet foundDets;
  for(auto const & badDet : badPixelDetsEndcap){
    if(foundDets.find(badDet) == foundDets.end()){
      detGroups.push_back(this->reachableDetGroup(badDet,foundDets));
    } 
  }
  return detGroups;
}
// Functions for finding DetGroupSpans
bool PixelInactiveAreaFinder::phiMoreClockwise(float phiA, float phiB){
  // return true if a is more clockwise than b
  // assuming both angels are in same half
  float xa,ya,xb,yb;
  xa = cos(phiA);
  ya = sin(phiA);
  xb = cos(phiB);
  yb = sin(phiB);
  if(xa >= 0 && xb >= 0){
    return ya <= yb;
  }else if (ya >= 0 && yb >= 0 ){
    return xa >= xb;
  }else if (xa <= 0 && xb <= 0){
    return ya >= yb;
  }else if (ya <= 0 && yb <= 0){
    return xa <= xb;
  }else {
    return false;
  }
}
bool PixelInactiveAreaFinder::phiMoreCounterclockwise(float phiA, float phiB){
  // return true if a is more counterclockwise than b
  // assuming both ngels are in same half
  float xa,ya,xb,yb;
  xa = cos(phiA);
  ya = sin(phiA);
  xb = cos(phiB);
  yb = sin(phiB);
  if(xa >= 0 && xb >= 0){
    return ya >= yb;
  }else if (ya >= 0 && yb >= 0 ){
    return xa <= xb;
  }else if (xa <= 0 && xb <= 0){
    return ya <= yb;
  }else if (ya <= 0 && yb <= 0){
    return xa >= xb;
  }else {
    return false;
  }
}
void PixelInactiveAreaFinder::getPhiSpanBarrel(const DetGroup & detGroup, DetGroupSpan & cspan){
  // find phiSpan using ordered vector of unique ladders in detGroup
  if(detGroup.size() == 0){
    cspan = DetGroupSpan();
    return;
  } else{
    cspan.layer = trackerTopology_->pxbLayer(DetId(detGroup[0]));
    cspan.disk = 0;
  }
  using uint = unsigned int;
  using LadderSet = std::set<uint>;
  using LadVec = std::vector<uint>;
  LadderSet lads;
  for(auto const & det : detGroup){
    lads.insert(trackerTopology_->pxbLadder(DetId(det)));
  }
  LadVec ladv(lads.begin(),lads.end());
  uint nLadders = 0;
  switch(cspan.layer){
  case 1: nLadders = nLayer1Ladders;break;
  case 2: nLadders = nLayer2Ladders;break;
  case 3: nLadders = nLayer3Ladders;break;
  case 4: nLadders = nLayer4Ladders;break;
  default: nLadders = 0;
  }
  // find start ladder of detGroup
  uint i = 0;
  uint currentLadder = ladv[0];
  uint previousLadder = ladv[ (ladv.size()+i-1) % ladv.size() ];
  // loop until discontinuity is found from vector
  while ( (nLadders+currentLadder-1)%nLadders  == previousLadder  ){
    ++i;
    currentLadder = ladv[i%ladv.size()];
    previousLadder = ladv[ (ladv.size()+i-1)%ladv.size() ];
    if(i == ladv.size()){
      cspan.phiSpan.first  =  std::numeric_limits<float>::epsilon();
      cspan.phiSpan.second = -std::numeric_limits<float>::epsilon();
      return;
    }
  }
  uint startLadder = currentLadder;
  uint endLadder = previousLadder;
  auto detStart = trackerTopology_->pxbDetId(cspan.layer,startLadder,1);
  auto detEnd = trackerTopology_->pxbDetId(cspan.layer,endLadder,1);
  cspan.phiSpan.first  = trackerGeometry_->idToDet(detStart)->surface().phiSpan().first;
  cspan.phiSpan.second = trackerGeometry_->idToDet(detEnd)->surface().phiSpan().second;
}
void PixelInactiveAreaFinder::getPhiSpanEndcap(const DetGroup & detGroup, DetGroupSpan & cspan){
  // this is quite naive/bruteforce method
  // 1) it starts by taking one detector from detGroup and starts to compare it to others
  // 2) when it finds overlapping detector in clockwise direction it starts comparing 
  //    found detector to others
  // 3) search stops until no overlapping detectors in clockwise detector or all detectors
  //    have been work detector
  Stream ss;
  bool found = false;
  auto const tGeom = trackerGeometry_;
  DetGroup::const_iterator startDetIter = detGroup.begin();
  Span_t phiSpan,phiSpanComp;
  unsigned int counter = 0;
  while(!found){
    phiSpan = tGeom->idToDet(DetId(*startDetIter))->surface().phiSpan();
    for(DetGroup::const_iterator compDetIter=detGroup.begin();compDetIter!=detGroup.end();++compDetIter){
      phiSpanComp = tGeom->idToDet(DetId(*compDetIter))->surface().phiSpan();
      if(phiRangesOverlap(phiSpan,phiSpanComp)
         && phiMoreClockwise(phiSpanComp.first,phiSpan.first)
         && startDetIter != compDetIter)
        {
          ++counter;
          if(counter > detGroup.size()){
            cspan.phiSpan.first  =  std::numeric_limits<float>::epsilon();
            cspan.phiSpan.second = -std::numeric_limits<float>::epsilon();
            return;
          }
          startDetIter = compDetIter;break;
        } else if (compDetIter == detGroup.end()-1){
        found = true;
      }
    }
  }
  cspan.phiSpan.first = phiSpan.first;
  // second with same method}
  found = false;
  DetGroup::const_iterator endDetIter = detGroup.begin();
  counter = 0;
  while(!found){
    phiSpan = tGeom->idToDet(DetId(*endDetIter))->surface().phiSpan();
    for(DetGroup::const_iterator compDetIter=detGroup.begin();compDetIter!=detGroup.end();++compDetIter){
      phiSpanComp = tGeom->idToDet(DetId(*compDetIter))->surface().phiSpan();
      if(phiRangesOverlap(phiSpan,phiSpanComp)
         && phiMoreCounterclockwise(phiSpanComp.second,phiSpan.second)
         && endDetIter != compDetIter)
        {
          ++counter;
          if(counter > detGroup.size()){
            cspan.phiSpan.first  =  std::numeric_limits<float>::epsilon();
            cspan.phiSpan.second = -std::numeric_limits<float>::epsilon();
            return;
          }
          endDetIter = compDetIter;break;
        } else if (compDetIter == detGroup.end()-1){
        found = true;
      }
    }
  }
  cspan.phiSpan.second = phiSpan.second;
}
void PixelInactiveAreaFinder::getZSpan(const DetGroup & detGroup, DetGroupSpan & cspan){
  auto cmpFun = [this] (det_t detA, det_t detB){
    return
    trackerGeometry_->idToDet(DetId(detA))->surface().zSpan().first
    <
    trackerGeometry_->idToDet(DetId(detB))->surface().zSpan().first
    ;
  };
    
  auto minmaxIters = std::minmax_element(detGroup.begin(),detGroup.end(),cmpFun);
  cspan.zSpan.first = trackerGeometry_->idToDet(DetId(*(minmaxIters.first)))->surface().zSpan().first;
  cspan.zSpan.second = trackerGeometry_->idToDet(DetId(*(minmaxIters.second)))->surface().zSpan().second;
}
void PixelInactiveAreaFinder::getRSpan(const DetGroup & detGroup, DetGroupSpan & cspan){
  auto cmpFun = [this] (det_t detA, det_t detB){
    return
    trackerGeometry_->idToDet(DetId(detA))->surface().rSpan().first
    <
    trackerGeometry_->idToDet(DetId(detB))->surface().rSpan().first
    ;
  };
    
  auto minmaxIters = std::minmax_element(detGroup.begin(),detGroup.end(),cmpFun);
  cspan.rSpan.first = trackerGeometry_->idToDet(DetId(*(minmaxIters.first)))->surface().rSpan().first;
  cspan.rSpan.second = trackerGeometry_->idToDet(DetId(*(minmaxIters.second)))->surface().rSpan().second;
}
void PixelInactiveAreaFinder::getSpan(const DetGroup & detGroup, DetGroupSpan & cspan){
  auto firstDetIt = detGroup.begin();
  if(firstDetIt != detGroup.end()){
    cspan.subdetId = DetId(*firstDetIt).subdetId();
    if(cspan.subdetId == 1){
      cspan.layer = trackerTopology_->pxbLayer(DetId(*firstDetIt));
      cspan.disk = 0;
      getPhiSpanBarrel(detGroup,cspan);    
    }else if(cspan.subdetId == 2){
      cspan.disk = trackerTopology_->pxfDisk(DetId(*firstDetIt));
      cspan.layer = 0;
      getPhiSpanEndcap(detGroup,cspan);
    }
    getZSpan(detGroup,cspan);
    getRSpan(detGroup,cspan);
  }
}
PixelInactiveAreaFinder::DetGroupSpanContainerPair PixelInactiveAreaFinder::detGroupSpans(){
  DetGroupSpanContainer cspansBarrel;
  DetGroupSpanContainer cspansEndcap;
  DetGroupContainer badDetGroupsBar = badDetGroupsBarrel();
  DetGroupContainer badDetGroupsEnd = badDetGroupsEndcap();
  for(auto const & detGroup : badDetGroupsBar){
    DetGroupSpan cspan;
    getSpan(detGroup,cspan);
    cspansBarrel.push_back(cspan);
  }
  for(auto const & detGroup : badDetGroupsEnd){
    DetGroupSpan cspan;
    getSpan(detGroup,cspan);
    cspansEndcap.push_back(cspan);
  }
  return DetGroupSpanContainerPair(cspansBarrel,cspansEndcap);
}
// Functions for findind overlapping functions
float PixelInactiveAreaFinder::zAxisIntersection(const float zrPointA[2], const float zrPointB[2]){
  return (zrPointB[0]-zrPointA[0])/(zrPointB[1]-zrPointA[1])*(-zrPointA[1])+zrPointA[0];
}
bool PixelInactiveAreaFinder::getZAxisOverlapRangeBarrel(const DetGroupSpan & cspanA, const DetGroupSpan & cspanB, std::pair<float,float> & range){
  DetGroupSpan cspanUpper;
  DetGroupSpan cspanLower;
  if(cspanA.rSpan.second < cspanB.rSpan.first){
    cspanLower = cspanA;
    cspanUpper = cspanB;
  }else if(cspanA.rSpan.first > cspanB.rSpan.second){
    cspanUpper = cspanA;
    cspanLower = cspanB;
  }else{
    return false;
  }
  float lower = 0;
  float upper = 0;
  if(cspanUpper.zSpan.second < cspanLower.zSpan.first){
    // lower intersectionpoint, point = {z,r} in cylindrical coordinates
    const float pointUpperDetGroupL[2] = {cspanUpper.zSpan.second, cspanUpper.rSpan.second};
    const float pointLowerDetGroupL[2] = {cspanLower.zSpan.first,  cspanLower.rSpan.first};
    lower = zAxisIntersection(pointUpperDetGroupL,pointLowerDetGroupL);
    // upper intersectionpoint
    const float pointUpperDetGroupU[2] = {cspanUpper.zSpan.first, cspanUpper.rSpan.first};
    const float pointLowerDetGroupU[2] = {cspanLower.zSpan.second,  cspanLower.rSpan.second};
    upper = zAxisIntersection(pointUpperDetGroupU,pointLowerDetGroupU);
  }else if (cspanUpper.zSpan.first <= cspanLower.zSpan.second && cspanLower.zSpan.first <= cspanUpper.zSpan.second){
    // lower intersectionpoint, point = {z,r} in cylindrical coordinates
    const float pointUpperDetGroupL[2] = {cspanUpper.zSpan.second, cspanUpper.rSpan.first};
    const float pointLowerDetGroupL[2] = {cspanLower.zSpan.first,  cspanLower.rSpan.second};
    lower = zAxisIntersection(pointUpperDetGroupL,pointLowerDetGroupL);
    // upper intersectionpoint
    const float pointUpperDetGroupU[2] = {cspanUpper.zSpan.first, cspanUpper.rSpan.first};
    const float pointLowerDetGroupU[2] = {cspanLower.zSpan.second,  cspanLower.rSpan.second};
    upper = zAxisIntersection(pointUpperDetGroupU,pointLowerDetGroupU);
  }else if (cspanUpper.zSpan.first > cspanLower.zSpan.second){
    // lower intersectionpoint, point = {z,r} in cylindrical coordinates
    const float pointUpperDetGroupL[2] = {cspanUpper.zSpan.second, cspanUpper.rSpan.first};
    const float pointLowerDetGroupL[2] = {cspanLower.zSpan.first,  cspanLower.rSpan.second};
    lower = zAxisIntersection(pointUpperDetGroupL,pointLowerDetGroupL);
    // upper intersectionpoint
    const float pointUpperDetGroupU[2] = {cspanUpper.zSpan.first, cspanUpper.rSpan.second};
    const float pointLowerDetGroupU[2] = {cspanLower.zSpan.second,  cspanLower.rSpan.first};
    upper = zAxisIntersection(pointUpperDetGroupU,pointLowerDetGroupU);
  }else{
    //something wrong
    return false;
  }
  range = std::pair<float,float>(lower,upper);
  return true;
}
bool PixelInactiveAreaFinder::getZAxisOverlapRangeEndcap(const DetGroupSpan & cspanA, const DetGroupSpan & cspanB, std::pair<float,float> & range){
  // While on left hand side of pixel detector
  DetGroupSpan cspanNearer;
  DetGroupSpan cspanFurther;
  float lower = 0;
  float upper = 0;
  if(cspanA.zSpan.first < 0 && cspanB.zSpan.first < 0){
    if(cspanA.zSpan.second < cspanB.zSpan.first){
      cspanFurther = cspanA;
      cspanNearer = cspanB;
    }else if (cspanB.zSpan.second < cspanA.zSpan.first){
      cspanFurther = cspanB;
      cspanNearer = cspanA;
    }else {
      //edm::LogPrint("") << "No overlap, same disk propably. Spans:";
      //Stream ss;
      //detGroupSpanInfo(cspanA,ss);ss<<std::endl;detGroupSpanInfo(cspanB,ss);ss<<std::endl;
      //edm::LogPrint("") << ss.str();ss.str(std::string());
      //edm::LogPrint("") << "**";
      return false;
    }
    if(cspanFurther.rSpan.second > cspanNearer.rSpan.first){
      const float pointA[2] = {cspanFurther.zSpan.second, cspanFurther.rSpan.second};
      const float pointB[2] = {cspanNearer.zSpan.first, cspanNearer.rSpan.first};
      lower = zAxisIntersection(pointA,pointB);
      if(cspanFurther.rSpan.first > cspanNearer.rSpan.second){
        const float pointC[2] = {cspanFurther.zSpan.first, cspanFurther.rSpan.first};
        const float pointD[2] = {cspanNearer.zSpan.second, cspanFurther.rSpan.second};
        upper = zAxisIntersection(pointC,pointD);
      }else{
        upper = std::numeric_limits<float>::infinity();
      }
    }else{
      //edm::LogPrint("") << "No overlap, further detGroup is lower. Spans:";
      //Stream ss;
      //detGroupSpanInfo(cspanA,ss);ss<<std::endl;detGroupSpanInfo(cspanB,ss);ss<<std::endl;
      //edm::LogPrint("") << ss.str();ss.str(std::string());
      //edm::LogPrint("") << "**";
      return false;
    }
  }else if(cspanA.zSpan.first > 0 && cspanB.zSpan.first > 0){
    if(cspanA.zSpan.first > cspanB.zSpan.second ){
      cspanFurther = cspanA;
      cspanNearer = cspanB;
    }else if(cspanB.zSpan.first > cspanA.zSpan.second){
      cspanFurther = cspanB;
      cspanNearer = cspanA;
    }else{
      //edm::LogPrint("") << "No overlap, same disk propably. Spans:";
      //Stream ss;
      //detGroupSpanInfo(cspanA,ss);ss<<std::endl;detGroupSpanInfo(cspanB,ss);ss<<std::endl;
      //edm::LogPrint("") << ss.str();ss.str(std::string());
      //edm::LogPrint("") << "**";
      return false;
    }
    if(cspanFurther.rSpan.second > cspanNearer.rSpan.first){
      const float pointA[2] = {cspanFurther.zSpan.first, cspanFurther.rSpan.second};
      const float pointB[2] = {cspanNearer.zSpan.second, cspanNearer.rSpan.first};
      upper = zAxisIntersection(pointA,pointB);
      if(cspanFurther.rSpan.first > cspanNearer.rSpan.second){
        const float pointC[2] = {cspanFurther.zSpan.second, cspanFurther.rSpan.first};
        const float pointD[2] = {cspanNearer.zSpan.first, cspanFurther.rSpan.second};
        lower = zAxisIntersection(pointC,pointD);
      }else{
        lower = -std::numeric_limits<float>::infinity();
      }
    }else{
      //edm::LogPrint("") << "No overlap, further detGroup lower. Spans:";
      //Stream ss;
      //detGroupSpanInfo(cspanA,ss);ss<<std::endl;detGroupSpanInfo(cspanB,ss);ss<<std::endl;
      //edm::LogPrint("") << ss.str();ss.str(std::string());
      //edm::LogPrint("") << "**";
      return false;
    }
  }else{       
    //edm::LogPrint("") << "No overlap, different sides of z axis. Spans:";
    //Stream ss;
    //detGroupSpanInfo(cspanA,ss);ss<<std::endl;detGroupSpanInfo(cspanB,ss);ss<<std::endl;
    //edm::LogPrint("") << ss.str();ss.str(std::string());
    //edm::LogPrint("") << "**";
    return false;
  }
  range = std::pair<float,float>(lower,upper);
  return true;
}

bool PixelInactiveAreaFinder::getZAxisOverlapRangeBarrelEndcap(const DetGroupSpan & cspanBar, const DetGroupSpan & cspanEnd, std::pair<float,float> & range){
  float lower = 0;
  float upper = 0;
  if(cspanEnd.rSpan.second > cspanBar.rSpan.first){
    if(cspanEnd.zSpan.second < cspanBar.zSpan.first){
      // if we are on the left hand side of pixel detector
      const float pointA[2] = {cspanEnd.zSpan.second, cspanEnd.rSpan.second};
      const float pointB[2] = {cspanBar.zSpan.first, cspanBar.rSpan.first};
      lower = zAxisIntersection(pointA,pointB);
      if(cspanEnd.rSpan.first > cspanBar.rSpan.second){
        // if does not overlap, then there is also upper limit
        const float pointC[2] = {cspanEnd.zSpan.first, cspanEnd.rSpan.first};
        const float pointD[2] = {cspanBar.zSpan.second, cspanBar.rSpan.second};
        upper = zAxisIntersection(pointC,pointD);
      }else{
        upper = std::numeric_limits<float>::infinity();
      }
    }else if (cspanEnd.zSpan.first > cspanBar.zSpan.second){
      // if we are on the right hand side of pixel detector
      const float pointA[2] = {cspanEnd.zSpan.first, cspanEnd.rSpan.second};
      const float pointB[2] = {cspanBar.zSpan.second, cspanBar.rSpan.first};
      upper = zAxisIntersection(pointA,pointB);
      if(cspanEnd.rSpan.first > cspanBar.rSpan.second){
        const float pointC[2] = {cspanEnd.zSpan.second,cspanEnd.rSpan.first};
        const float pointD[2] = {cspanBar.zSpan.first, cspanBar.rSpan.second};
        lower = zAxisIntersection(pointC,pointD);
      }else{
        lower = - std::numeric_limits<float>::infinity();
      }
    }else {
      return false;
    }
  }else{
    return false;
  } 
  range =  std::pair<float,float>(lower,upper);
  return true;
}
PixelInactiveAreaFinder::OverlapSpansContainer PixelInactiveAreaFinder::overlappingSpans(float zAxisThreshold){    
  OverlapSpansContainer overlapSpansContainer;
  // find detGroupSpans ie phi,r,z limits for detector detGroups that are not working
  // returns pair where first is barrel spans and second endcap spans
  DetGroupSpanContainerPair cspans = detGroupSpans();

  // map spans to a vector with consisntent indexing with layers_ and layerSetIndices_
  // TODO: try to move the inner logic towards this direction as well
  std::vector<DetGroupSpanContainer> spans(layers_.size());

  auto doWork = [&](const DetGroupSpanContainer& container) {
    for(const auto& span: container) {
      const auto subdet = span.subdetId == PixelSubdetector::PixelBarrel ? GeomDetEnumerators::PixelBarrel : GeomDetEnumerators::PixelEndcap;
      const auto side = (subdet == GeomDetEnumerators::PixelBarrel ? TrackerDetSide::Barrel :
                         (span.zSpan.first < 0 ? TrackerDetSide::NegEndcap : TrackerDetSide::PosEndcap));
      const auto layer = subdet == GeomDetEnumerators::PixelBarrel ? span.layer : span.disk;
      auto found = std::find(layers_.begin(), layers_.end(), SeedingLayerId(subdet, side, layer));
      if(found != layers_.end()) { // it is possible that this layer is ignored by the configuration
        spans[std::distance(layers_.begin(), found)].push_back(span);
      }
    }
  };
  doWork(cspans.first);
  doWork(cspans.second);

  for(const auto& layerIdxPair: layerSetIndices_) {
    const auto& innerSpans = spans[layerIdxPair.first];
    const auto& outerSpans = spans[layerIdxPair.second];

    for(const auto& innerSpan: innerSpans) {
      for(const auto& outerSpan: outerSpans) {

        if(phiRangesOverlap(innerSpan.phiSpan, outerSpan.phiSpan)) {
          std::pair<float,float> range(0,0);

          bool zOverlap = false;
          const auto innerDet = std::get<0>(layers_[layerIdxPair.first]);
          const auto outerDet = std::get<0>(layers_[layerIdxPair.first]);
          if(innerDet == GeomDetEnumerators::PixelBarrel) {
            if(outerDet == GeomDetEnumerators::PixelBarrel)
              zOverlap = getZAxisOverlapRangeBarrel(innerSpan, outerSpan, range);
            else
              zOverlap = getZAxisOverlapRangeBarrelEndcap(innerSpan, outerSpan, range);
          }
          else {
            if(outerDet == GeomDetEnumerators::PixelEndcap)
              zOverlap = getZAxisOverlapRangeEndcap(innerSpan, outerSpan, range);
            else
              throw cms::Exception("LogicError") << "Forward->barrel transition is not supported";
          }

          if(zOverlap && -zAxisThreshold <= range.second && range.first <= zAxisThreshold) {
            overlapSpansContainer.push_back(OverlapSpans({{innerSpan, outerSpan}}));
          }
        }
      }
    }
  }

  return overlapSpansContainer;
}
