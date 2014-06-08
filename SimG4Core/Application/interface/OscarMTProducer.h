#ifndef SimG4Core_OscarMTProducer_H
#define SimG4Core_OscarMTProducer_H

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Run.h"

#include "SimG4Core/Application/interface/RunManagerMT.h"
#include "SimG4Core/Application/interface/RunManagerMTMaster.h"
#include "SimG4Core/Application/interface/CustomUIsession.h"

#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"

#include <memory>

#include <thread>
class OscarMTMasterThread {
public:
  OscarMTMasterThread(std::shared_ptr<RunManagerMT> runManager, const edm::EventSetup& iSetup);
  //OscarMTMasterThread(const edm::ParameterSet& iConfig, const edm::EventSetup& iSetup);
  ~OscarMTMasterThread();

  const RunManagerMT& runManager() const { return *m_runManager; }
  const RunManagerMTMaster& runManagerMaster() const { return *m_runManagerMaster; }
  
private:
  std::shared_ptr<RunManagerMT> m_runManager;
  std::shared_ptr<RunManagerMTMaster> m_runManagerMaster;
  std::thread m_masterThread;
};

class OscarMTProducer : public edm::stream::EDProducer<
  edm::GlobalCache<edm::ParameterSet>,
  edm::RunCache<OscarMTMasterThread>
>
{
public:
  typedef std::vector<boost::shared_ptr<SimProducer> > Producers;

  explicit OscarMTProducer(edm::ParameterSet const & p, const edm::ParameterSet *);
  virtual ~OscarMTProducer();

  static std::unique_ptr<edm::ParameterSet> initializeGlobalCache(const edm::ParameterSet& iConfig);
  static std::shared_ptr<OscarMTMasterThread> globalBeginRun(const edm::Run& iRun, const edm::EventSetup& iSetup, const edm::ParameterSet *iConfig);
  static void globalEndRun(const edm::Run& iRun, const edm::EventSetup& iSetup, const RunContext *iContext);
  static void globalEndJob(edm::ParameterSet *iConfig);

  virtual void beginRun(const edm::Run & r,const edm::EventSetup& c) override;
  virtual void endRun(const edm::Run & r,const edm::EventSetup& c) override;
  virtual void produce(edm::Event & e, const edm::EventSetup& c) override;

private:
  std::unique_ptr<RunManagerMT> m_runManager;
  Producers     m_producers;
  std::unique_ptr<CustomUIsession> m_UIsession;
  //edm::EDGetTokenT<edm::HepMCProduct> m_HepMC;
};

#endif
