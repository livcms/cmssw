#include "Validation/RecoTrack/interface/MultiTrackValidator.h"
#include "DQMServices/ClientConfig/interface/FitSlicesYTool.h"

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrackFwd.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"
#include "SimTracker/TrackAssociation/interface/TrackAssociatorByChi2.h"
#include "SimTracker/TrackAssociation/interface/QuickTrackAssociatorByHits.h"
#include "SimTracker/TrackerHitAssociation/interface/TrackerHitAssociator.h"
#include "SimTracker/Records/interface/TrackAssociatorRecord.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertex.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertexContainer.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/EncodedEventId/interface/EncodedEventId.h"
#include "TrackingTools/TrajectoryState/interface/FreeTrajectoryState.h"
#include "TrackingTools/PatternTools/interface/TSCBLBuilderNoMaterial.h"
#include "SimTracker/TrackAssociation/plugins/ParametersDefinerForTPESProducer.h"
#include "SimTracker/TrackAssociation/plugins/CosmicParametersDefinerForTPESProducer.h"
#include "Validation/RecoTrack/interface/MTVHistoProducerAlgoFactory.h"

#include "DataFormats/TrackReco/interface/DeDxData.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Common/interface/Ref.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "TMath.h"
#include <TF1.h>

//#include <iostream>

using namespace std;
using namespace edm;

typedef edm::Ref<edm::HepMCProduct, HepMC::GenParticle > GenParticleRef;

MultiTrackValidator::MultiTrackValidator(const edm::ParameterSet& pset):
  MultiTrackValidatorBase(pset),
  parametersDefinerIsCosmic_(parametersDefiner == "CosmicParametersDefinerForTP")
{
  //theExtractor = IsoDepositExtractorFactory::get()->create( extractorName, extractorPSet);

  ParameterSet psetForHistoProducerAlgo = pset.getParameter<ParameterSet>("histoProducerAlgoBlock");
  string histoProducerAlgoName = psetForHistoProducerAlgo.getParameter<string>("ComponentName");
  histoProducerAlgo_ = MTVHistoProducerAlgoFactory::get()->create(histoProducerAlgoName ,psetForHistoProducerAlgo);
  histoProducerAlgo_->setDQMStore(dbe_);

  dirName_ = pset.getParameter<std::string>("dirName");
  associatormap = pset.getParameter< edm::InputTag >("associatormap");
  UseAssociators = pset.getParameter< bool >("UseAssociators");

  m_dEdx1Tag = pset.getParameter< edm::InputTag >("dEdx1Tag");
  m_dEdx2Tag = pset.getParameter< edm::InputTag >("dEdx2Tag");

  tpSelector = TrackingParticleSelector(pset.getParameter<double>("ptMinTP"),
					pset.getParameter<double>("minRapidityTP"),
					pset.getParameter<double>("maxRapidityTP"),
					pset.getParameter<double>("tipTP"),
					pset.getParameter<double>("lipTP"),
					pset.getParameter<int>("minHitTP"),
					pset.getParameter<bool>("signalOnlyTP"),
					pset.getParameter<bool>("chargedOnlyTP"),
					pset.getParameter<bool>("stableOnlyTP"),
					pset.getParameter<std::vector<int> >("pdgIdTP"));

  cosmictpSelector = CosmicTrackingParticleSelector(pset.getParameter<double>("ptMinTP"),
						    pset.getParameter<double>("minRapidityTP"),
						    pset.getParameter<double>("maxRapidityTP"),
						    pset.getParameter<double>("tipTP"),
						    pset.getParameter<double>("lipTP"),
						    pset.getParameter<int>("minHitTP"),
						    pset.getParameter<bool>("chargedOnlyTP"),
						    pset.getParameter<std::vector<int> >("pdgIdTP"));

  useGsf = pset.getParameter<bool>("useGsf");
  runStandalone = pset.getParameter<bool>("runStandalone");

  _simHitTpMapTag = pset.getParameter<edm::InputTag>("simHitTpMapTag");
  vertexTag_ = pset.getParameter<edm::InputTag>("vertex");
    
  if (!UseAssociators) {
    associators.clear();
    associators.push_back(associatormap.label());
  }

}


MultiTrackValidator::~MultiTrackValidator(){delete histoProducerAlgo_;}

void MultiTrackValidator::beginRun(Run const&, EventSetup const& setup) {
  //  dbe_->showDirStructure();

  //int j=0;  //is This Necessary ???
  for (unsigned int ww=0;ww<associators.size();ww++){
    for (unsigned int www=0;www<label.size();www++){
      dbe_->cd();
      InputTag algo = label[www];
      string dirName=dirName_;
      if (algo.process()!="")
	dirName+=algo.process()+"_";
      if(algo.label()!="")
	dirName+=algo.label()+"_";
      if(algo.instance()!="")
	dirName+=algo.instance()+"_";      
      if (dirName.find("Tracks")<dirName.length()){
	dirName.replace(dirName.find("Tracks"),6,"");
      }
      string assoc= associators[ww];
      if (assoc.find("Track")<assoc.length()){
	assoc.replace(assoc.find("Track"),5,"");
      }
      dirName+=assoc;
      std::replace(dirName.begin(), dirName.end(), ':', '_');

      dbe_->setCurrentFolder(dirName.c_str());
      
      // vector of vector initialization
      histoProducerAlgo_->initialize(); //TO BE FIXED. I'D LIKE TO AVOID THIS CALL

      dbe_->goUp(); //Is this really necessary ???
      string subDirName = dirName + "/simulation";
      dbe_->setCurrentFolder(subDirName.c_str());

      //Booking histograms concerning with simulated tracks
      histoProducerAlgo_->bookSimHistos();

      dbe_->cd();
      dbe_->setCurrentFolder(dirName.c_str());

      //Booking histograms concerning with reconstructed tracks
      histoProducerAlgo_->bookRecoHistos();
      if (runStandalone) histoProducerAlgo_->bookRecoHistosForStandaloneRunning();

      if (UseAssociators) {
	edm::ESHandle<TrackAssociatorBase> theAssociator;
	for (unsigned int w=0;w<associators.size();w++) {
	  setup.get<TrackAssociatorRecord>().get(associators[w],theAssociator);
	  associator.push_back( theAssociator.product() );
	}//end loop w
      }
    }//end loop www
  }// end loop ww
}

void MultiTrackValidator::analyze(const edm::Event& event, const edm::EventSetup& setup){
  using namespace reco;
  
  LogDebug("TrackValidator") << "\n====================================================" << "\n"
				 << "Analyzing new event" << "\n"
				 << "====================================================\n" << "\n";
  edm::ESHandle<ParametersDefinerForTP> parametersDefinerTP; 
  setup.get<TrackAssociatorRecord>().get(parametersDefiner,parametersDefinerTP);    
  
  edm::Handle<TrackingParticleCollection>  TPCollectionHeff ;
  event.getByLabel(label_tp_effic,TPCollectionHeff);
  const TrackingParticleCollection& tPCeff = *(TPCollectionHeff.product());
  
  edm::Handle<TrackingParticleCollection>  TPCollectionHfake ;
  event.getByLabel(label_tp_fake,TPCollectionHfake);
  //const TrackingParticleCollection& tPCfake = *(TPCollectionHfake.product());
  
  if(parametersDefinerIsCosmic_) {
    edm::Handle<SimHitTPAssociationProducer::SimHitTPAssociationList> simHitsTPAssoc;
    //warning: make sure the TP collection used in the map is the same used in the MTV!
    event.getByLabel(_simHitTpMapTag,simHitsTPAssoc);
    parametersDefinerTP->initEvent(simHitsTPAssoc);
    cosmictpSelector.initEvent(simHitsTPAssoc);
  }

  //if (tPCeff.size()==0) {LogDebug("TrackValidator") 
  //<< "TP Collection for efficiency studies has size = 0! Skipping Event." ; return;}
  //if (tPCfake.size()==0) {LogDebug("TrackValidator") 
  //<< "TP Collection for fake rate studies has size = 0! Skipping Event." ; return;}
  
  edm::Handle<reco::BeamSpot> recoBeamSpotHandle;
  event.getByLabel(bsSrc,recoBeamSpotHandle);
  reco::BeamSpot bs = *recoBeamSpotHandle;      
  
  edm::Handle< vector<PileupSummaryInfo> > puinfoH;
  event.getByLabel(label_pileupinfo,puinfoH);
  PileupSummaryInfo puinfo;      
  
  for (unsigned int puinfo_ite=0;puinfo_ite<(*puinfoH).size();++puinfo_ite){ 
    if ((*puinfoH)[puinfo_ite].getBunchCrossing()==0){
      puinfo=(*puinfoH)[puinfo_ite];
      break;
    }
  }

  edm::Handle<TrackingVertexCollection> tvH;
  event.getByLabel(label_tv,tvH);
  const TrackingVertexCollection& tv = *tvH;      
  
  edm::Handle<reco::VertexCollection> vertexH;
  event.getByLabel(vertexTag_, vertexH);

  const reco::Vertex::Point *thePVposition = nullptr;
  //const TrackingVertex::LorentzVector *theSimPVPosition = nullptr;

  //bool isPVmatched = false;

  if(!vertexH->empty()) {
    const reco::Vertex& thePV = (*vertexH)[0];
    if(!(thePV.isFake() || thePV.ndof() < 0.)) { // skip junk
      for(size_t i=0; i<tv.size(); ++i) {
        const TrackingVertex& simV = tv[i];
        if(simV.eventId().bunchCrossing() != 0) continue; // remove OOTPU
        if(simV.eventId().event() != 0) continue; // remove ITPU

        if(std::abs(thePV.z() - simV.position().z()) < 0.1 &&
           std::abs(thePV.z() - simV.position().z())/thePV.zError() < 3) {
          //theSimPVPosition = &(simV.position());
          thePVposition = &(thePV.position());
        }
        break; // check only the hard scatter sim PV
      }
    }
  }
  /*
  if(!isPVmatched)
    return;
  */
  


  // Precalculate TP selection (for efficiency), and momentum and vertex wrt PCA
  //
  // TODO: ParametersDefinerForTP ESProduct needs to be changed to
  // EDProduct because of consumes.
  //
  // In principle, we could just precalculate the momentum and vertex
  // wrt PCA for all TPs for once and put that to the event. To avoid
  // repetitive calculations those should be calculated only once for
  // each TP. That would imply that we should access TPs via Refs
  // (i.e. View) in here, since, in general, the eff and fake TP
  // collections can be different (and at least HI seems to use that
  // feature). This would further imply that the
  // RecoToSimCollection/SimToRecoCollection should be changed to use
  // View<TP> instead of vector<TP>, and migrate everything.
  //
  // Or we could take only one input TP collection, and do another
  // TP-selection to obtain the "fake" collection like we already do
  // for "efficiency" TPs.
  std::vector<size_t> selected_tPCeff;
  std::vector<std::tuple<TrackingParticle::Vector, TrackingParticle::Point>> momVert_tPCeff;
  selected_tPCeff.reserve(tPCeff.size());
  momVert_tPCeff.reserve(tPCeff.size());
  if(parametersDefinerIsCosmic_) {
    for(size_t j=0; j<tPCeff.size(); ++j) {
      TrackingParticleRef tpr(TPCollectionHeff, j);
      if(cosmictpSelector(tpr,&bs,event,setup)) {
        selected_tPCeff.push_back(j);
        TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
        TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
        momVert_tPCeff.emplace_back(momentum, vertex);
      }
    }
  }
  else {
    size_t j=0;
    for(auto const& tp: tPCeff) {
      if(tpSelector(tp)) {
        selected_tPCeff.push_back(j);
	TrackingParticleRef tpr(TPCollectionHeff, j);
        TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
        TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
        momVert_tPCeff.emplace_back(momentum, vertex);
      }
      ++j;
    }
  }



  int w=0; //counter counting the number of sets of histograms
  for (unsigned int ww=0;ww<associators.size();ww++){
    for (unsigned int www=0;www<label.size();www++){
      //
      //get collections from the event
      //
      edm::Handle<View<Track> >  trackCollection;
      if(!event.getByLabel(label[www], trackCollection)&&ignoremissingtkcollection_)continue;
      //if (trackCollection->size()==0) 
      //LogDebug("TrackValidator") << "TrackCollection size = 0!" ; 
      //continue;
      //}
      reco::RecoToSimCollection recSimColl;
      reco::SimToRecoCollection simRecColl;
      //associate tracks
      if(UseAssociators){
	LogTrace("TrackValidator") << "Analyzing " 
					   << label[www].process()<<":"
					   << label[www].label()<<":"
					   << label[www].instance()<<" with "
					   << associators[ww].c_str() <<"\n";
	
	LogTrace("TrackValidator") << "Calling associateRecoToSim method" << "\n";
	recSimColl=associator[ww]->associateRecoToSim(trackCollection,
						      TPCollectionHfake,
						      &event,&setup);
	LogTrace("TrackValidator") << "Calling associateSimToReco method" << "\n";
	simRecColl=associator[ww]->associateSimToReco(trackCollection,
						      TPCollectionHeff, 
						      &event,&setup);
      }
      else{
	LogTrace("TrackValidator") << "Analyzing " 
					   << label[www].process()<<":"
					   << label[www].label()<<":"
					   << label[www].instance()<<" with "
					   << associatormap.process()<<":"
					   << associatormap.label()<<":"
					   << associatormap.instance()<<"\n";
	
	Handle<reco::SimToRecoCollection > simtorecoCollectionH;
	event.getByLabel(associatormap,simtorecoCollectionH);
	simRecColl= *(simtorecoCollectionH.product()); 
	
	Handle<reco::RecoToSimCollection > recotosimCollectionH;
	event.getByLabel(associatormap,recotosimCollectionH);
	recSimColl= *(recotosimCollectionH.product()); 
      }



      // ########################################################
      // fill simulation histograms (LOOP OVER TRACKINGPARTICLES)
      // ########################################################

      //compute number of tracks per eta interval
      //
      LogTrace("TrackValidator") << "\n# of TrackingParticles: " << tPCeff.size() << "\n";
      int ats(0);  	  //This counter counts the number of simTracks that are "associated" to recoTracks
      int st(0);    	  //This counter counts the number of simulated tracks passing the MTV selection (i.e. tpSelector(tp) )
      unsigned sts(0);   //This counter counts the number of simTracks surviving the bunchcrossing cut 
      unsigned asts(0);  //This counter counts the number of simTracks that are "associated" to recoTracks surviving the bunchcrossing cut

      //loop over already-selected TPs for tracking efficiency
      for(size_t i=0; i<selected_tPCeff.size(); ++i) {
        size_t iTP = selected_tPCeff[i];
	TrackingParticleRef tpr(TPCollectionHeff, iTP);
	const TrackingParticle& tp = tPCeff[iTP];

        auto const& momVert = momVert_tPCeff[i];

	TrackingParticle::Vector momentumTP; 
	TrackingParticle::Point vertexTP;
	double dxySim(0);
	double dzSim(0);
	
	//---------- THIS PART HAS TO BE CLEANED UP. THE PARAMETER DEFINER WAS NOT MEANT TO BE USED IN THIS WAY ----------
	//If the TrackingParticle is collison like, get the momentum and vertex at production state
	if(!parametersDefinerIsCosmic_)
	  {
	    momentumTP = tp.momentum();
	    vertexTP = tp.vertex();
	    //Calcualte the impact parameters w.r.t. PCA
	    //const TrackingParticle::Vector& momentum = std::get<TrackingParticle::Vector>(momVert);
	    //const TrackingParticle::Point& vertex = std::get<TrackingParticle::Point>(momVert);
	    const TrackingParticle::Vector& momentum = std::get<0>(momVert);
	    const TrackingParticle::Point& vertex = std::get<1>(momVert);
	    dxySim = (-vertex.x()*sin(momentum.phi())+vertex.y()*cos(momentum.phi()));
	    dzSim = vertex.z() - (vertex.x()*momentum.x()+vertex.y()*momentum.y())/sqrt(momentum.perp2()) 
	      * momentum.z()/sqrt(momentum.perp2());
	  }
	//If the TrackingParticle is comics, get the momentum and vertex at PCA
        else
	  {
	    //momentumTP = std::get<TrackingParticle::Vector>(momVert);
	    //vertexTP = std::get<TrackingParticle::Point>(momVert);
	    momentumTP = std::get<0>(momVert);
	    vertexTP = std::get<1>(momVert);
	    dxySim = (-vertexTP.x()*sin(momentumTP.phi())+vertexTP.y()*cos(momentumTP.phi()));
	    dzSim = vertexTP.z() - (vertexTP.x()*momentumTP.x()+vertexTP.y()*momentumTP.y())/sqrt(momentumTP.perp2()) 
	      * momentumTP.z()/sqrt(momentumTP.perp2());
	  }
	//---------- THE PART ABOVE HAS TO BE CLEANED UP. THE PARAMETER DEFINER WAS NOT MEANT TO BE USED IN THIS WAY ----------

	st++;   //This counter counts the number of simulated tracks passing the MTV selection (i.e. tpSelector(tp) )

	// in the coming lines, histos are filled using as input
	// - momentumTP 
	// - vertexTP 
	// - dxySim
	// - dzSim

	histoProducerAlgo_->fill_generic_simTrack_histos(w,momentumTP,vertexTP, tp.eventId().bunchCrossing());


	// ##############################################
	// fill RecoAssociated SimTracks' histograms
	// ##############################################
	// bool isRecoMatched(false); // UNUSED
	const reco::Track* matchedTrackPointer=0;
	std::vector<std::pair<RefToBase<Track>, double> > rt;
	if(simRecColl.find(tpr) != simRecColl.end()){
	  rt = (std::vector<std::pair<RefToBase<Track>, double> >) simRecColl[tpr];
	  if (rt.size()!=0) {
	    ats++; //This counter counts the number of simTracks that have a recoTrack associated
	    // isRecoMatched = true; // UNUSED
	    matchedTrackPointer = rt.begin()->first.get();
	    LogTrace("TrackValidator") << "TrackingParticle #" << st 
					       << " with pt=" << sqrt(momentumTP.perp2()) 
					       << " associated with quality:" << rt.begin()->second <<"\n";
	  }
	}else{
	  LogTrace("TrackValidator") 
	    << "TrackingParticle #" << st
	    << " with pt,eta,phi: " 
	    << sqrt(momentumTP.perp2()) << " , "
	    << momentumTP.eta() << " , "
	    << momentumTP.phi() << " , "
	    << " NOT associated to any reco::Track" << "\n";
	}
	

	

        int nSimHits = tp.numberOfTrackerHits();

        double vtx_z_PU = vertexTP.z();
        for (size_t j = 0; j < tv.size(); j++) {
            if (tp.eventId().event() == tv[j].eventId().event()) {
                vtx_z_PU = tv[j].position().z();
                break;
            }
        }

        histoProducerAlgo_->fill_recoAssociated_simTrack_histos(w,tp,momentumTP,vertexTP,dxySim,dzSim,nSimHits,matchedTrackPointer,puinfo.getPU_NumInteractions(), vtx_z_PU, thePVposition);
          sts++;
          if (matchedTrackPointer) asts++;




      } // End  for (TrackingParticleCollection::size_type i=0; i<tPCeff.size(); i++){

      //if (st!=0) h_tracksSIM[w]->Fill(st);  // TO BE FIXED
      
      
      // ##############################################
      // fill recoTracks histograms (LOOP OVER TRACKS)
      // ##############################################
      LogTrace("TrackValidator") << "\n# of reco::Tracks with "
					 << label[www].process()<<":"
					 << label[www].label()<<":"
					 << label[www].instance()
					 << ": " << trackCollection->size() << "\n";

      int sat(0); //This counter counts the number of recoTracks that are associated to SimTracks from Signal only
      int at(0); //This counter counts the number of recoTracks that are associated to SimTracks
      int rT(0); //This counter counts the number of recoTracks in general


      // dE/dx
      // at some point this could be generalized, with a vector of tags and a corresponding vector of Handles
      // I'm writing the interface such to take vectors of ValueMaps
      edm::Handle<edm::ValueMap<reco::DeDxData> > dEdx1Handle;
      edm::Handle<edm::ValueMap<reco::DeDxData> > dEdx2Handle;
      std::vector<edm::ValueMap<reco::DeDxData> > v_dEdx;
      v_dEdx.clear();
      //std::cout << "PIPPO: label is " << label[www] << std::endl;
      if (label[www].label()=="generalTracks") {
	try {
	  event.getByLabel(m_dEdx1Tag, dEdx1Handle);
	  const edm::ValueMap<reco::DeDxData> dEdx1 = *dEdx1Handle.product();
	  event.getByLabel(m_dEdx2Tag, dEdx2Handle);
	  const edm::ValueMap<reco::DeDxData> dEdx2 = *dEdx2Handle.product();
	  v_dEdx.push_back(dEdx1);
	  v_dEdx.push_back(dEdx2);
	} catch (cms::Exception e){
	  LogTrace("TrackValidator") << "exception found: " << e.what() << "\n";
	}
      }
      //end dE/dx

      for(View<Track>::size_type i=0; i<trackCollection->size(); ++i){

	RefToBase<Track> track(trackCollection, i);
	rT++;
	
	bool isSigSimMatched(false);
	bool isSimMatched(false);
    bool isChargeMatched(true);
    int numAssocRecoTracks = 0;
        int tpbx = 0;
	int nSimHits = 0;
	double sharedFraction = 0.;
	std::vector<std::pair<TrackingParticleRef, double> > tp;
	if(recSimColl.find(track) != recSimColl.end()){
	  tp = recSimColl[track];
	  if (tp.size()!=0) {
	    nSimHits = tp[0].first->numberOfTrackerHits();
            sharedFraction = tp[0].second;
	    isSimMatched = true;
        if (tp[0].first->charge() != track->charge()) isChargeMatched = false;
        if(simRecColl.find(tp[0].first) != simRecColl.end()) numAssocRecoTracks = simRecColl[tp[0].first].size();
        //std::cout << numAssocRecoTracks << std::endl;
	    tpbx = tp[0].first->eventId().bunchCrossing();
	    at++;
	    for (unsigned int tp_ite=0;tp_ite<tp.size();++tp_ite){ 
              TrackingParticle trackpart = *(tp[tp_ite].first);
	      if ((trackpart.eventId().event() == 0) && (trackpart.eventId().bunchCrossing() == 0)){
	      	isSigSimMatched = true;
		sat++;
		break;
	      }
            }
	    LogTrace("TrackValidator") << "reco::Track #" << rT << " with pt=" << track->pt() 
					       << " associated with quality:" << tp.begin()->second <<"\n";
	  }
	} else {
	  LogTrace("TrackValidator") << "reco::Track #" << rT << " with pt=" << track->pt()
					     << " NOT associated to any TrackingParticle" << "\n";		  
	}
	

	histoProducerAlgo_->fill_generic_recoTrack_histos(w,*track,bs.position(), thePVposition, isSimMatched,isSigSimMatched, isChargeMatched, numAssocRecoTracks, puinfo.getPU_NumInteractions(), tpbx, nSimHits, sharedFraction);

	// dE/dx
	//	reco::TrackRef track2  = reco::TrackRef( trackCollection, i );
	if (v_dEdx.size() > 0) histoProducerAlgo_->fill_dedx_recoTrack_histos(w,track, v_dEdx);
	//if (v_dEdx.size() > 0) histoProducerAlgo_->fill_dedx_recoTrack_histos(track2, v_dEdx);


	//Fill other histos
 	//try{ //Is this really necessary ????

	if (tp.size()==0) continue;	

	histoProducerAlgo_->fill_simAssociated_recoTrack_histos(w,*track);

	TrackingParticleRef tpr = tp.begin()->first;
	
	/* TO BE FIXED LATER
	if (associators[ww]=="TrackAssociatorByChi2"){
	  //association chi2
	  double assocChi2 = -tp.begin()->second;//in association map is stored -chi2
	  h_assochi2[www]->Fill(assocChi2);
	  h_assochi2_prob[www]->Fill(TMath::Prob((assocChi2)*5,5));
	}
	else if (associators[ww]=="quickTrackAssociatorByHits"){
	  double fraction = tp.begin()->second;
	  h_assocFraction[www]->Fill(fraction);
	  h_assocSharedHit[www]->Fill(fraction*track->numberOfValidHits());
	}
	*/

	  
	//Get tracking particle parameters at point of closest approach to the beamline
	TrackingParticle::Vector momentumTP = parametersDefinerTP->momentum(event,setup,tpr);
	TrackingParticle::Point vertexTP = parametersDefinerTP->vertex(event,setup,tpr);		 	 
	int chargeTP = tpr->charge();

	histoProducerAlgo_->fill_ResoAndPull_recoTrack_histos(w,momentumTP,vertexTP,chargeTP,
							     *track,bs.position());
	
	
	//TO BE FIXED
	//std::vector<PSimHit> simhits=tpr.get()->trackPSimHit(DetId::Tracker);
	//nrecHit_vs_nsimHit_rec2sim[w]->Fill(track->numberOfValidHits(), (int)(simhits.end()-simhits.begin() ));
	
	/*
	  } // End of try{
	  catch (cms::Exception e){
	  LogTrace("TrackValidator") << "exception found: " << e.what() << "\n";
	  }
	*/
	
      } // End of for(View<Track>::size_type i=0; i<trackCollection->size(); ++i){

      histoProducerAlgo_->fill_trackBased_histos(w,at,rT,st);

      LogTrace("TrackValidator") << "Total Simulated: " << st << "\n"
					 << "Total Associated (simToReco): " << ats << "\n"
					 << "Total Reconstructed: " << rT << "\n"
					 << "Total Associated (recoToSim): " << at << "\n"
					 << "Total Fakes: " << rT-at << "\n";

      w++;
    } // End of  for (unsigned int www=0;www<label.size();www++){
  } //END of for (unsigned int ww=0;ww<associators.size();ww++){

}

void MultiTrackValidator::endRun(Run const&, EventSetup const&) {
  int w=0;
  for (unsigned int ww=0;ww<associators.size();ww++){
    for (unsigned int www=0;www<label.size();www++){
      if(!skipHistoFit && runStandalone)	histoProducerAlgo_->finalHistoFits(w);
      if (runStandalone) histoProducerAlgo_->fillProfileHistosFromVectors(w);
      histoProducerAlgo_->fillHistosFromVectors(w);
      w++;
    }    
  }
  if ( out.size() != 0 && dbe_ ) dbe_->save(out);
}



