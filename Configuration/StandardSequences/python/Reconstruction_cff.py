import FWCore.ParameterSet.Config as cms

from RecoLuminosity.LumiProducer.lumiProducer_cff import *
from RecoLuminosity.LumiProducer.bunchSpacingProducer_cfi import *
from RecoLocalMuon.Configuration.RecoLocalMuon_cff import *
from RecoLocalCalo.Configuration.RecoLocalCalo_cff import *
from RecoTracker.Configuration.RecoTracker_cff import *
from RecoParticleFlow.PFClusterProducer.particleFlowCluster_cff import *
from TrackingTools.Configuration.TrackingTools_cff import *
from RecoTracker.MeasurementDet.MeasurementTrackerEventProducer_cfi import *
from RecoPixelVertexing.PixelLowPtUtilities.siPixelClusterShapeCache_cfi import *
siPixelClusterShapeCachePreSplitting = siPixelClusterShapeCache.clone(
    src = 'siPixelClustersPreSplitting'
    )

# Global  reco
from RecoEcal.Configuration.RecoEcal_cff import *
from RecoJets.Configuration.CaloTowersRec_cff import *
from RecoMET.Configuration.RecoMET_cff import *
from RecoMuon.Configuration.RecoMuon_cff import *
# Higher level objects
from RecoVertex.Configuration.RecoVertex_cff import *
from RecoEgamma.Configuration.RecoEgamma_cff import *
from RecoPixelVertexing.Configuration.RecoPixelVertexing_cff import *


from RecoJets.Configuration.RecoJetsGlobal_cff import *
from RecoMET.Configuration.RecoPFMET_cff import *
from RecoBTag.Configuration.RecoBTag_cff import *
#
# please understand that division global,highlevel is completely fake !
#
#local reconstruction
from RecoLocalTracker.Configuration.RecoLocalTracker_cff import *
from RecoParticleFlow.Configuration.RecoParticleFlow_cff import *
#
# new tau configuration
#
from RecoTauTag.Configuration.RecoPFTauTag_cff import *
# Also BeamSpot
from RecoVertex.BeamSpotProducer.BeamSpot_cff import *

from RecoLocalCalo.CastorReco.CastorSimpleReconstructor_cfi import *

localreco = cms.Sequence(trackerlocalreco+muonlocalreco+calolocalreco+castorreco)
localreco_HcalNZS = cms.Sequence(trackerlocalreco+muonlocalreco+calolocalrecoNZS+castorreco)

#
# temporarily switching off recoGenJets; since this are MC and wil be moved to a proper sequence
#

from RecoLocalCalo.Castor.Castor_cff import *
from RecoLocalCalo.Configuration.hcalGlobalReco_cff import *

globalreco = cms.Sequence(offlineBeamSpot*
                          MeasurementTrackerEventPreSplitting* # unclear where to put this
                          siPixelClusterShapeCachePreSplitting* # unclear where to put this
                          standalonemuontracking*
                          trackingGlobalReco*
                          vertexreco*
                          hcalGlobalRecoSequence*
                          particleFlowCluster*
                          ecalClusters*
                          caloTowersRec*                          
                          egammaGlobalReco*
                          jetGlobalReco*
                          muonGlobalReco*
                          pfTrackingGlobalReco*
                          muoncosmicreco*
                          CastorFullReco)

globalreco_plusPL= cms.Sequence(globalreco*ctfTracksPixelLess)


reducedRecHits = cms.Sequence ( reducedEcalRecHitsSequence * reducedHcalRecHitsSequence )

highlevelreco = cms.Sequence(egammaHighLevelRecoPrePF*
                             particleFlowReco*
                             egammaHighLevelRecoPostPF*
                             muoncosmichighlevelreco*
                             muonshighlevelreco *
                             particleFlowLinks*
                             jetHighLevelReco*
                             metrecoPlusHCALNoise*
                             btagging*
                             recoPFMET*
                             PFTau*
                             reducedRecHits
                             )


from FWCore.Modules.logErrorHarvester_cfi import *

# "Export" Section
reconstruction         = cms.Sequence(bunchSpacingProducer*localreco*globalreco*highlevelreco*logErrorHarvester)


# need to "declare" these in order to be modified in _modifyRun1
reconstruction_fromRECO = cms.Sequence()
reconstruction_fromRECO_noTrackingTest = cms.Sequence()
reconstruction_fromRECO_noTracking = cms.Sequence()
reconstruction_noTracking = cms.Sequence()
# This stuff needs to done in a modifier function because the tracking
# sequences are loaded via that mechanism, and their modules are
# visible only via process object
from Configuration.StandardSequences.Eras import eras
def _modifyForRun1(process):
    if eras.phase1Pixel.isChosen():
        return

    global reconstruction_fromRECO
    global reconstruction_fromRECO_noTrackingTest
    global reconstruction_fromRECO_noTracking
    global reconstruction_noTracking

    #need a fully expanded sequence copy
    modulesToRemove = list() # copy does not work well
    noTrackingAndDependent = list()
    noTrackingAndDependent.append(siPixelClustersPreSplitting)
    noTrackingAndDependent.append(siStripZeroSuppression)
    noTrackingAndDependent.append(siStripClusters)
    noTrackingAndDependent.append(process.initialStepSeedLayersPreSplitting)
    noTrackingAndDependent.append(process.initialStepSeedsPreSplitting)
    noTrackingAndDependent.append(process.initialStepTrackCandidatesPreSplitting)
    noTrackingAndDependent.append(process.initialStepTracksPreSplitting)
    noTrackingAndDependent.append(process.firstStepPrimaryVerticesPreSplitting)
    noTrackingAndDependent.append(process.initialStepTrackRefsForJetsPreSplitting)
    noTrackingAndDependent.append(process.caloTowerForTrkPreSplitting)
    noTrackingAndDependent.append(process.ak4CaloJetsForTrkPreSplitting)
    noTrackingAndDependent.append(process.jetsForCoreTrackingPreSplitting)
    noTrackingAndDependent.append(siPixelClusterShapeCachePreSplitting)
    noTrackingAndDependent.append(process.siPixelClusters)
    noTrackingAndDependent.append(clusterSummaryProducer)
    noTrackingAndDependent.append(siPixelRecHitsPreSplitting)
    noTrackingAndDependent.append(MeasurementTrackerEventPreSplitting)
    modulesToRemove.append(dt1DRecHits)
    modulesToRemove.append(dt1DCosmicRecHits)
    modulesToRemove.append(csc2DRecHits)
    modulesToRemove.append(rpcRecHits)
    #modulesToRemove.append(ecalGlobalUncalibRecHit)
    modulesToRemove.append(ecalMultiFitUncalibRecHit)
    modulesToRemove.append(ecalDetIdToBeRecovered)
    modulesToRemove.append(ecalRecHit)
    modulesToRemove.append(ecalCompactTrigPrim)
    modulesToRemove.append(ecalTPSkim)
    modulesToRemove.append(ecalPreshowerRecHit)
    modulesToRemove.append(selectDigi)
    modulesToRemove.append(hbheprereco)
    modulesToRemove.append(hbhereco)
    modulesToRemove.append(hfreco)
    modulesToRemove.append(horeco)
    modulesToRemove.append(hcalnoise)
    modulesToRemove.append(zdcreco)
    modulesToRemove.append(castorreco)
    ##it's OK according to Ronny modulesToRemove.append(CSCHaloData)#needs digis
    reconstruction_fromRECO = reconstruction.copyAndExclude(modulesToRemove+noTrackingAndDependent)
#    noTrackingAndDependent.append(process.siPixelRecHitsPreSplitting)
    noTrackingAndDependent.append(siStripMatchedRecHits)
    noTrackingAndDependent.append(pixelTracks)
    noTrackingAndDependent.append(ckftracks)
    reconstruction_fromRECO_noTrackingTest = reconstruction.copyAndExclude(modulesToRemove+noTrackingAndDependent)
    ##requires generalTracks trajectories
    noTrackingAndDependent.append(trackerDrivenElectronSeeds)
    noTrackingAndDependent.append(ecalDrivenElectronSeeds)
    noTrackingAndDependent.append(uncleanedOnlyElectronSeeds)
    noTrackingAndDependent.append(uncleanedOnlyElectronCkfTrackCandidates)
    noTrackingAndDependent.append(uncleanedOnlyElectronGsfTracks)
    noTrackingAndDependent.append(uncleanedOnlyGeneralConversionTrackProducer)
    noTrackingAndDependent.append(uncleanedOnlyGsfConversionTrackProducer)
    noTrackingAndDependent.append(uncleanedOnlyPfTrackElec)
    noTrackingAndDependent.append(uncleanedOnlyGsfElectronCores)
    noTrackingAndDependent.append(uncleanedOnlyPfTrack)
    noTrackingAndDependent.append(uncleanedOnlyGeneralInOutOutInConversionTrackMerger)#can live with
    noTrackingAndDependent.append(uncleanedOnlyGsfGeneralInOutOutInConversionTrackMerger)#can live with
    noTrackingAndDependent.append(uncleanedOnlyAllConversions)
    noTrackingAndDependent.append(uncleanedOnlyGsfElectrons)#can live with
    noTrackingAndDependent.append(electronMergedSeeds)
    noTrackingAndDependent.append(electronCkfTrackCandidates)
    noTrackingAndDependent.append(electronGsfTracks)
    noTrackingAndDependent.append(generalConversionTrackProducer)
    noTrackingAndDependent.append(generalInOutOutInConversionTrackMerger)
    noTrackingAndDependent.append(gsfGeneralInOutOutInConversionTrackMerger)
    noTrackingAndDependent.append(ecalDrivenGsfElectrons)
    noTrackingAndDependent.append(gsfConversionTrackProducer)
    noTrackingAndDependent.append(allConversions)
    noTrackingAndDependent.append(gsfElectrons)
    reconstruction_fromRECO_noTracking = reconstruction.copyAndExclude(modulesToRemove+noTrackingAndDependent)
    reconstruction_noTracking = reconstruction.copyAndExclude(noTrackingAndDependent)

modifyConfigurationStandardSequencesReconstructionForRun1_ = eras.Run1.makeProcessModifier(_modifyForRun1, depends=modifyRecoTrackConfigurationRecoTrackerForRun1_)
modifyConfigurationStandardSequencesReconstructionForRun2_ = eras.run2_common.makeProcessModifier(_modifyForRun1, depends=modifyRecoTrackConfigurationRecoTrackerForRun2_)

#sequences with additional stuff
reconstruction_withPixellessTk  = cms.Sequence(localreco        *globalreco_plusPL*highlevelreco*logErrorHarvester)
reconstruction_HcalNZS = cms.Sequence(localreco_HcalNZS*globalreco       *highlevelreco*logErrorHarvester)

#sequences without some stuffs
#
reconstruction_woCosmicMuons = cms.Sequence(localreco*globalreco*highlevelreco*logErrorHarvester)


# define a standard candle. please note I am picking up individual
# modules instead of sequences
#
reconstruction_standard_candle = cms.Sequence(localreco*globalreco*vertexreco*recoJetAssociations*btagging*electronSequence*photonSequence)


