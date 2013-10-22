// -*- c++ -*-
/*Offline DQM For Tau HLT
 Author : Michail Bachtis
 University of Wisconsin-Madison
 bachtis@hep.wisc.edu
 */

#include <FWCore/Framework/interface/EDAnalyzer.h>
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DQMServices/Core/interface/MonitorElement.h"
#include "DQMServices/Core/interface/DQMStore.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "DataFormats/Math/interface/LorentzVector.h"
#include "FWCore/Utilities/interface/Digest.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"

//Plotters
#include "DQMOffline/Trigger/interface/HLTTauDQML1Plotter.h"
#include "DQMOffline/Trigger/interface/HLTTauDQMPathPlotter.h"
#include "DQMOffline/Trigger/interface/HLTTauDQMPathSummaryPlotter.h"

#include<memory>

//
// class declaration
//

class HLTTauDQMOfflineSource : public edm::EDAnalyzer {
public:
    HLTTauDQMOfflineSource( const edm::ParameterSet& );
    ~HLTTauDQMOfflineSource();
    
protected:
    /// BeginJob
    void beginJob();
    
    /// BeginRun
    void beginRun(const edm::Run& r, const edm::EventSetup& c);
    
    /// Fake Analyze
    void analyze(const edm::Event& e, const edm::EventSetup& c) ;
    
    /// Luminosity Block 
    void beginLuminosityBlock(const edm::LuminosityBlock& lumiSeg, const edm::EventSetup& context);
    
    /// DQM Client Diagnostic
    void endLuminosityBlock(const edm::LuminosityBlock& lumiSeg, const edm::EventSetup& c);
    
    /// EndRun
    void endRun(const edm::Run& r, const edm::EventSetup& c);
    
    /// Endjob
    void endJob();
    
private:
    std::vector<edm::ParameterSet> config_;
    edm::ParameterSet matching_;
    std::string moduleName_;
    std::string hltProcessName_;
    edm::InputTag triggerResultsSrc_;
    edm::InputTag triggerEventSrc_;
    edm::ParameterSet ps_;
    std::string dqmBaseFolder_;
    bool hltMenuChanged_;
    bool verbose_;
    
    HLTConfigProvider HLTCP_;

    //Reference
    bool doRefAnalysis_;
    struct RefObject {
      int objID;
      edm::InputTag src;
    };
    std::vector<RefObject> refObjects_;

    int NPtBins_;
    int NEtaBins_;
    int NPhiBins_;
    double EtMax_;
    double L1MatchDr_;
    double HLTMatchDr_;
    
    //DQM Prescaler
    int counterEvt_;      //counter
    int prescaleEvt_;     //every n events 
    
    //Helper function to retrieve data from the parameter set
    void processPSet( const edm::ParameterSet& pset );
    
    //Count parameters
    unsigned int countParameters( const edm::ParameterSet& pset );
    
    //search event content
    void searchEventContent(std::vector<edm::InputTag>& eventContent, const edm::ParameterSet& pset);

    //Define Dummy vectors of Plotters
    std::vector<std::unique_ptr<HLTTauDQML1Plotter>> l1Plotters_;
    std::vector<std::unique_ptr<HLTTauDQMPathPlotter>> pathPlotters2_;
    std::vector<std::unique_ptr<HLTTauDQMPathSummaryPlotter>> pathSummaryPlotters_;
};
