//--------------------------------------------------------------------------------------------------
// AntiElectronIDMVA6
//
// Helper Class for applying MVA anti-electron discrimination
//
// Authors: F.Colombo, C.Veelken
//          M. Bluj (template version)
//--------------------------------------------------------------------------------------------------

#ifndef RECOTAUTAG_RECOTAU_AntiElectronIDMVA6_H
#define RECOTAUTAG_RECOTAU_AntiElectronIDMVA6_H

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"

#include "DataFormats/TauReco/interface/PFTau.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrackFwd.h"
#include "CondFormats/EgammaObjects/interface/GBRForest.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "RecoTauTag/RecoTau/interface/PositionAtECalEntranceComputer.h"

#include "TMVA/Tools.h"
#include "TMVA/Reader.h"

#include "DataFormats/Math/interface/deltaR.h"

#include <vector>

namespace antiElecIDMVA6_blocks {
  struct TauVars {
    float pt = 0;
    float etaAtEcalEntrance = 0;
    float phi = 0;
    float leadChargedPFCandPt = 0;
    float leadChargedPFCandEtaAtEcalEntrance = 0;
    float emFraction = 0;
    float leadPFChargedHadrHoP = 0;
    float leadPFChargedHadrEoP = 0;
    float visMassIn = 0;
    float dCrackEta = 0;
    float dCrackPhi = 0;
    float hasGsf = 0;
  };
  struct TauGammaVecs {
    std::vector<float> gammasdEtaInSigCone;
    std::vector<float> gammasdPhiInSigCone;
    std::vector<float> gammasPtInSigCone;
    std::vector<float> gammasdEtaOutSigCone;
    std::vector<float> gammasdPhiOutSigCone;
    std::vector<float> gammasPtOutSigCone;
  };
  struct TauGammaMoms {
    int signalPFGammaCandsIn = 0;
    int signalPFGammaCandsOut = 0;
    float gammaEtaMomIn = 0;
    float gammaEtaMomOut = 0;
    float gammaPhiMomIn = 0;
    float gammaPhiMomOut = 0;
    float gammaEnFracIn = 0;
    float gammaEnFracOut = 0;
  };
  struct ElecVars {
    float eta = 0;
    float phi = 0;
    float eTotOverPin = 0;
    float chi2NormGSF = 0;
    float chi2NormKF = 0;
    float gsfNumHits = 0;
    float kfNumHits = 0;
    float gsfTrackResol = 0;
    float gsfTracklnPt = 0;
    float pIn = 0;
    float pOut = 0;
    float eEcal = 0;
    float deltaEta = 0;
    float deltaPhi = 0;
    float mvaInSigmaEtaEta = 0;
    float mvaInHadEnergy = 0;
    float mvaInDeltaEta = 0;
    float eSeedClusterOverPout = 0;
    float superClusterEtaWidth = 0;
    float superClusterPhiWidth = 0;
    float sigmaIEtaIEta5x5 = 0;
    float sigmaIPhiIPhi5x5 = 0;
    float showerCircularity = 0;
    float r9 = 0;
    float hgcalSigmaUU = 0;
    float hgcalSigmaVV = 0;
    float hgcalSigmaEE = 0;
    float hgcalSigmaPP = 0;
    float hgcalNLayers = 0;
    float hgcalFirstLayer = 0;
    float hgcalLastLayer = 0;
    float hgcalLayerEfrac10 = 0;
    float hgcalLayerEfrac90 = 0;
    float hgcalEcEnergyEE = 0;
    float hgcalEcEnergyFH = 0;
    float hgcalMeasuredDepth = 0;
    float hgcalExpectedDepth = 0;
    float hgcalExpectedSigma = 0;
    float hgcalDepthCompatibility = 0;
  };
}  // namespace antiElecIDMVA6_blocks

template <class TauType, class ElectronType>
class AntiElectronIDMVA6 {
public:
  AntiElectronIDMVA6(const edm::ParameterSet&);
  ~AntiElectronIDMVA6();

  void beginEvent(const edm::Event&, const edm::EventSetup&);

  double MVAValue(const antiElecIDMVA6_blocks::TauVars& tauVars,
                  const antiElecIDMVA6_blocks::TauGammaVecs& tauGammaVecs,
                  const antiElecIDMVA6_blocks::ElecVars& elecVars);

  double MVAValue(const antiElecIDMVA6_blocks::TauVars& tauVars,
                  const antiElecIDMVA6_blocks::TauGammaMoms& tauGammaMoms,
                  const antiElecIDMVA6_blocks::ElecVars& elecVars);

  double MVAValuePhase2(const antiElecIDMVA6_blocks::TauVars& tauVars,
                        const antiElecIDMVA6_blocks::TauGammaMoms& tauGammaMoms,
                        const antiElecIDMVA6_blocks::ElecVars& elecVars);

  // this function can be called for all categories
  double MVAValue(const TauType& theTau, const ElectronType& theEle);
  // this function can be called for category 1 only !!
  double MVAValue(const TauType& theTau);

  // overloaded method with explicit tau type to avoid partial imlementation of full class
  antiElecIDMVA6_blocks::TauVars getTauVarsTypeSpecific(const reco::PFTau& theTau);
  antiElecIDMVA6_blocks::TauVars getTauVarsTypeSpecific(const pat::Tau& theTau);
  antiElecIDMVA6_blocks::TauVars getTauVars(const TauType& theTau);
  antiElecIDMVA6_blocks::TauGammaVecs getTauGammaVecs(const TauType& theTau);
  antiElecIDMVA6_blocks::ElecVars getElecVars(const ElectronType& theEle);
  // overloaded method with explicit electron type to avoid partial imlementation of full class
  void getElecVarsHGCalTypeSpecific(const reco::GsfElectron& theEle, antiElecIDMVA6_blocks::ElecVars& elecVars);
  void getElecVarsHGCalTypeSpecific(const pat::Electron& theEle, antiElecIDMVA6_blocks::ElecVars& elecVars);

private:
  double dCrackEta(double eta);
  double minimum(double a, double b);
  double dCrackPhi(double phi, double eta);

  static constexpr float ecalBarrelEndcapEtaBorder_ = 1.479;
  static constexpr float ecalEndcapVFEndcapEtaBorder_ = 2.4;

  bool isInitialized_;
  bool loadMVAfromDB_;
  edm::FileInPath inputFileName_;

  std::string mvaName_NoEleMatch_woGwoGSF_BL_;
  std::string mvaName_NoEleMatch_wGwoGSF_BL_;
  std::string mvaName_woGwGSF_BL_;
  std::string mvaName_wGwGSF_BL_;
  std::string mvaName_NoEleMatch_woGwoGSF_EC_;
  std::string mvaName_NoEleMatch_wGwoGSF_EC_;
  std::string mvaName_woGwGSF_EC_;
  std::string mvaName_wGwGSF_EC_;
  std::string mvaName_NoEleMatch_woGwoGSF_VFEC_;
  std::string mvaName_NoEleMatch_wGwoGSF_VFEC_;
  std::string mvaName_woGwGSF_VFEC_;
  std::string mvaName_wGwGSF_VFEC_;

  bool usePhiAtEcalEntranceExtrapolation_;

  float* Var_NoEleMatch_woGwoGSF_Barrel_;
  float* Var_NoEleMatch_wGwoGSF_Barrel_;
  float* Var_woGwGSF_Barrel_;
  float* Var_wGwGSF_Barrel_;
  float* Var_NoEleMatch_woGwoGSF_Endcap_;
  float* Var_NoEleMatch_wGwoGSF_Endcap_;
  float* Var_woGwGSF_Endcap_;
  float* Var_wGwGSF_Endcap_;
  float* Var_NoEleMatch_woGwoGSF_VFEndcap_;
  float* Var_NoEleMatch_wGwoGSF_VFEndcap_;
  float* Var_woGwGSF_VFEndcap_;
  float* Var_wGwGSF_VFEndcap_;

  const GBRForest* mva_NoEleMatch_woGwoGSF_BL_;
  const GBRForest* mva_NoEleMatch_wGwoGSF_BL_;
  const GBRForest* mva_woGwGSF_BL_;
  const GBRForest* mva_wGwGSF_BL_;
  const GBRForest* mva_NoEleMatch_woGwoGSF_EC_;
  const GBRForest* mva_NoEleMatch_wGwoGSF_EC_;
  const GBRForest* mva_woGwGSF_EC_;
  const GBRForest* mva_wGwGSF_EC_;
  const GBRForest* mva_NoEleMatch_woGwoGSF_VFEC_;
  const GBRForest* mva_NoEleMatch_wGwoGSF_VFEC_;
  const GBRForest* mva_woGwGSF_VFEC_;
  const GBRForest* mva_wGwGSF_VFEC_;

  std::vector<TFile*> inputFilesToDelete_;

  PositionAtECalEntranceComputer positionAtECalEntrance_;

  const bool isPhase2_;

  const int verbosity_;
};

#endif
