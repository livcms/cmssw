import FWCore.ParameterSet.Config as cms

from RecoTracker.IterativeTracking.iterativeTk_cff import *
from RecoTracker.IterativeTracking.ElectronSeeds_cff import *
from SLHCUpgradeSimulations.Configuration.customise_mixing import customise_pixelMixing_PU

from Configuration.StandardSequences.Eras import eras

def customise(process):
    if hasattr(process,'DigiToRaw'):
        process=customise_DigiToRaw(process)
    if hasattr(process,'RawToDigi'):
        process=customise_RawToDigi(process)
    n=0
    if hasattr(process,'reconstruction') or hasattr(process,'dqmoffline_step'):
        if hasattr(process,'mix'): 
            if hasattr(process.mix,'input'):
                n=process.mix.input.nbPileupEvents.averageNumber.value()
        else:
            print 'phase1TkCustoms requires a --pileup option to cmsDriver to run the reconstruction/dqm'
            print 'Please provide one!'
            sys.exit(1)
    if hasattr(process,'reconstruction'):
        process=customise_Reco(process,float(n))
#        process=customise_Reco_v2(process)
#        process=customise_Reco_Run2(process)
                
    if hasattr(process,'digitisation_step'):
        process=customise_Digi(process)
    if hasattr(process,'dqmoffline_step'):
        process=customise_DQM(process,n)
    if hasattr(process,'dqmHarvesting'):
        process=customise_harvesting(process)
    if hasattr(process,'validation_step'):
        process=customise_Validation(process)
    process=customise_condOverRides(process)
    
    return process

def customise_DigiToRaw(process):
    # These were migrated in #12361
    if eras.phase1Pixel.isChosen():
        return process

    process.digi2raw_step.remove(process.siPixelRawData)
    process.digi2raw_step.remove(process.castorRawData)
    return process

def customise_RawToDigi(process):
    # These were migrated in #12361
    if eras.phase1Pixel.isChosen():
        return process

    process.raw2digi_step.remove(process.siPixelDigis)
    process.raw2digi_step.remove(process.castorDigis)
    return process

def customise_Digi(process):
    # these were migrated in #12275
    if eras.phase1Pixel.isChosen():
        return process

    process.mix.digitizers.pixel.MissCalibrate = False
    process.mix.digitizers.pixel.LorentzAngle_DB = False
    process.mix.digitizers.pixel.killModules = False
    process.mix.digitizers.pixel.useDB = False
    process.mix.digitizers.pixel.DeadModules_DB = False
    process.mix.digitizers.pixel.NumPixelBarrel = cms.int32(4)
    process.mix.digitizers.pixel.NumPixelEndcap = cms.int32(3)
    process.mix.digitizers.pixel.ThresholdInElectrons_FPix = cms.double(2000.0)
    process.mix.digitizers.pixel.ThresholdInElectrons_BPix = cms.double(2000.0)
    process.mix.digitizers.pixel.ThresholdInElectrons_BPix_L1 = cms.double(2000.0)
    process.mix.digitizers.pixel.thePixelColEfficiency_BPix1 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelColEfficiency_BPix2 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelColEfficiency_BPix3 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelColEfficiency_BPix4 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_BPix1 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_BPix2 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_BPix3 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_BPix4 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_BPix1 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_BPix2 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_BPix3 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_BPix4 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelColEfficiency_FPix1 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelColEfficiency_FPix2 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelColEfficiency_FPix3 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_FPix1 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_FPix2 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelEfficiency_FPix3 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_FPix1 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_FPix2 = cms.double(0.999)
    process.mix.digitizers.pixel.thePixelChipEfficiency_FPix3 = cms.double(0.999)
# something broken in the configs above - turn off for now
    process.mix.digitizers.pixel.AddPixelInefficiency = cms.bool(False)

    process=customise_pixelMixing_PU(process)
    return process


# DQM steps change
def customise_DQM(process,pileup):
    # FIXME
    #
    # These should be added back once somebody checks that they work,
    # and those that do not, get fixed
    #
    # The customizations are done here instead of in the central files
    # with era because they are temporary
    process.DQMOfflinePrePOG.remove(process.TrackingDQMSourceTier0)
    process.DQMOfflinePrePOG.remove(process.muonMonitors)
    process.DQMOfflinePrePOG.remove(process.jetMETDQMOfflineSource)
    process.DQMOfflinePrePOG.remove(process.egammaDQMOffline)
    process.DQMOfflinePrePOG.remove(process.triggerOfflineDQMSource)
    process.DQMOfflinePrePOG.remove(process.bTagPlotsDATA)
    process.DQMOfflinePrePOG.remove(process.alcaBeamMonitor)
    process.DQMOfflinePrePOG.remove(process.dqmPhysics)
    process.DQMOfflinePrePOG.remove(process.produceDenoms)
    process.DQMOfflinePrePOG.remove(process.pfTauRunDQMValidation)

    process.DQMOffline.remove(process.DQMOfflinePreDPG)
    process.DQMOffline.remove(process.HLTMonitoring)

    process.DQMOfflineTracking.remove(process.TrackingDQMSourceTier0Common)

    # Ok, this customization does not work currently at all
    # Need to be fixed before the tracking DQM can be enabled
    return process

    # We cut down the number of iterative tracking steps
    if not eras.phase1Pixel.isChosen(): # these were migrated in #12459
        process.dqmoffline_step.remove(process.muonAnalyzer)
        #process.dqmoffline_step.remove(process.jetMETAnalyzer)

    #put isUpgrade flag==true
    if not eras.phase1Pixel.isChosen(): # these were migrated in #12459
        process.SiPixelRawDataErrorSource.isUpgrade = cms.untracked.bool(True)
        process.SiPixelDigiSource.isUpgrade = cms.untracked.bool(True)
        process.SiPixelClusterSource.isUpgrade = cms.untracked.bool(True)
        process.SiPixelRecHitSource.isUpgrade = cms.untracked.bool(True)
        process.SiPixelTrackResidualSource.isUpgrade = cms.untracked.bool(True)
        process.SiPixelHitEfficiencySource.isUpgrade = cms.untracked.bool(True)

    from DQM.TrackingMonitor.customizeTrackingMonitorSeedNumber import customise_trackMon_IterativeTracking_PHASE1PU140
    from DQM.TrackingMonitor.customizeTrackingMonitorSeedNumber import customise_trackMon_IterativeTracking_PHASE1PU70

    if pileup>100:
        process=customise_trackMon_IterativeTracking_PHASE1PU140(process)
    else:
        process=customise_trackMon_IterativeTracking_PHASE1PU70(process)
    return process

def customise_Validation(process):
    # FIXME
    #
    # For starters, include only tracking validation
    # The rest should be added back once somebody checks that they
    # work, and those that do not, get fixed
    #
    # The customizations are done here instead of in the central files
    # with era because they are temporary
    process.globalPrevalidation.remove(process.photonPrevalidationSequence)
    process.globalPrevalidation.remove(process.produceDenoms)
    process.globalPrevalidation.remove(process.prebTagSequenceMC)
    # With era, would modify process.globalValidation
    process.validation.remove(process.trackerHitsValidation)
    process.validation.remove(process.trackerDigisValidation)
    process.validation.remove(process.trackerRecHitsValidation)
    process.validation.remove(process.trackingTruthValid)
    process.validation.remove(process.trackingRecHitsValid)
    process.validation.remove(process.ecalSimHitsValidationSequence)
    process.validation.remove(process.ecalDigisValidationSequence)
    process.validation.remove(process.ecalRecHitsValidationSequence)
    process.validation.remove(process.ecalClustersValidationSequence)
    process.validation.remove(process.hcalSimHitsValidationSequence)
    process.validation.remove(process.hcaldigisValidationSequence)
    process.validation.remove(process.hcalSimHitStudy)
    process.validation.remove(process.hcalRecHitsValidationSequence)
    process.validation.remove(process.calotowersValidationSequence)
    process.validation.remove(process.validSimHit)
    process.validation.remove(process.muondtdigianalyzer)
    process.validation.remove(process.cscDigiValidation)
    process.validation.remove(process.validationMuonRPCDigis)
    process.validation.remove(process.recoMuonValidation)
    process.validation.remove(process.muIsoVal_seq)
    process.validation.remove(process.muonIdValDQMSeq)
    process.validation.remove(process.mixCollectionValidation)
    process.validation.remove(process.JetValidation)
    process.validation.remove(process.METValidation)
    process.validation.remove(process.egammaValidation)
    process.validation.remove(process.pfJetValidationSequence)
    process.validation.remove(process.pfMETValidationSequence)
    process.validation.remove(process.pfElectronValidationSequence)
    process.validation.remove(process.pfJetResValidationSequence)
    process.validation.remove(process.pfMuonValidationSequence)
    process.validation.remove(process.rpcRecHitValidation_step)
    process.validation.remove(process.dtLocalRecoValidation_no2D)
    process.validation.remove(process.pfTauRunDQMValidation)
    process.validation.remove(process.bTagPlotsMCbcl)
    process.validation.remove(process.L1Validator)

    process.hltassociation = cms.Sequence()
    process.hltvalidation = cms.Sequence()

    # these were migrated in #12359
    if eras.phase1Pixel.isChosen():
        return process

    process.validation_step.remove(process.PixelTrackingRecHitsValid)
    # We don't run the HLT
    process.validation_step.remove(process.HLTSusyExoVal)
    process.validation_step.remove(process.hltHiggsValidator)
    process.validation_step.remove(process.relvalMuonBits)
    return process

def customise_Validation_Trackingonly(process):

    #To allow Tracking to perform special tracking only validation 
    process.trackValidator.label=cms.VInputTag(cms.InputTag("cutsRecoTracksHp"))
    process.tracksValidationSelectors = cms.Sequence(process.cutsRecoTracksHp)
    process.globalValidation.remove(process.recoMuonValidation)
    process.validation.remove(process.recoMuonValidation)
    process.validation_preprod.remove(process.recoMuonValidation)
    process.validation_step.remove(process.recoMuonValidation)
    process.validation.remove(process.globalrechitsanalyze)
    process.validation_prod.remove(process.globalrechitsanalyze)
    process.validation_step.remove(process.globalrechitsanalyze)
    process.validation.remove(process.stripRecHitsValid)
    process.validation_step.remove(process.stripRecHitsValid)
    process.validation_step.remove(process.StripTrackingRecHitsValid)
    process.globalValidation.remove(process.vertexValidation)
    process.validation.remove(process.vertexValidation)
    process.validation_step.remove(process.vertexValidation)
    process.mix.input.nbPileupEvents.averageNumber = cms.double(0.0)
    process.mix.minBunch = cms.int32(0)
    process.mix.maxBunch = cms.int32(0)
    return process

def customise_harvesting(process):
    # FIXME
    #
    # These should be added back once somebody checks that they work,
    # and those that do not, get fixed
    #
    # The customizations are done here instead of in the central files
    # with era because they are temporary
    process.DQMOffline_SecondStep.remove(process.DQMOffline_SecondStep_PreDPG)
    process.DQMOffline_SecondStep.remove(process.DQMOffline_SecondStep_PrePOG)
    process.DQMOffline_SecondStep.remove(process.HLTMonitoringClient)

    process.DQMHarvestTracking.remove(process.TrackingOfflineDQMClient)

    process.postValidation.remove(process.recoMuonPostProcessors)
    process.postValidation.remove(process.MuIsoValPostProcessor)
    process.postValidation.remove(process.calotowersPostProcessor)
    process.postValidation.remove(process.hcalSimHitsPostProcessor)
    process.postValidation.remove(process.hcaldigisPostProcessor)
    process.postValidation.remove(process.hcalrechitsPostProcessor)
    process.postValidation.remove(process.electronPostValidationSequence)
    process.postValidation.remove(process.photonPostProcessor)
    process.postValidation.remove(process.pfJetClient)
    process.postValidation.remove(process.pfMETClient)
    process.postValidation.remove(process.pfJetResClient)
    process.postValidation.remove(process.pfElectronClient)
    process.postValidation.remove(process.rpcRecHitPostValidation_step)
    process.postValidation.remove(process.runTauEff)
    process.postValidation.remove(process.makeBetterPlots)
    process.postValidation.remove(process.bTagCollectorSequenceMCbcl)
    process.postValidation.remove(process.METPostProcessor)
    process.postValidation_preprod.remove(process.recoMuonPostProcessors)
    process.postValidation_preprod.remove(process.MuIsoValPostProcessor)

    process.hltpostvalidation = cms.Sequence()

    # these were migrated in #12440
    if eras.phase1Pixel.isChosen():
        return process

    process.dqmHarvesting.remove(process.dataCertificationJetMET)
    #######process.dqmHarvesting.remove(process.sipixelEDAClient)
    process.sipixelEDAClient.isUpgrade = cms.untracked.bool(True)
    process.dqmHarvesting.remove(process.sipixelCertification)
    return (process)        

def customise_condOverRides(process):
#    process.trackerTopologyConstants.pxb_layerStartBit = cms.uint32(20)
#    process.trackerTopologyConstants.pxb_ladderStartBit = cms.uint32(12)
#    process.trackerTopologyConstants.pxb_moduleStartBit = cms.uint32(2)
#    process.trackerTopologyConstants.pxb_layerMask = cms.uint32(15)
#    process.trackerTopologyConstants.pxb_ladderMask = cms.uint32(255)
#    process.trackerTopologyConstants.pxb_moduleMask = cms.uint32(1023)
#    process.trackerTopologyConstants.pxf_diskStartBit = cms.uint32(18)
#    process.trackerTopologyConstants.pxf_bladeStartBit = cms.uint32(12)
#    process.trackerTopologyConstants.pxf_panelStartBit = cms.uint32(10)
#    process.trackerTopologyConstants.pxf_moduleMask = cms.uint32(255)
    process.load('SLHCUpgradeSimulations.Geometry.fakeConditions_Phase1_cff')
    return process

def add_detailed_pixel_dqm(process):
    #enable modOn
    process.SiPixelRawDataErrorSource.modOn = cms.untracked.bool(True)
    process.SiPixelDigiSource.modOn = cms.untracked.bool(True)
    process.SiPixelClusterSource.modOn = cms.untracked.bool(True)
    process.SiPixelRecHitSource.modOn = cms.untracked.bool(True)
    process.SiPixelTrackResidualSource.modOn = cms.untracked.bool(True)
    process.SiPixelHitEfficiencySource.modOn = cms.untracked.bool(True)

    return process


def remove_pixel_ineff(process):
    if hasattr(process,'mix'):
        process.mix.digitizers.pixel.AddPixelInefficiency = cms.bool(False) 

    return process
    

def customise_Reco(process,pileup):

    #this code supports either 70 or 140 pileup configurations - should fix as to support 0
    nPU=70
    if pileup>100: nPU=140
    
    #use with latest pixel geometry
    process.ClusterShapeHitFilterESProducer.PixelShapeFile = cms.string('RecoPixelVertexing/PixelLowPtUtilities/data/pixelShape_Phase1Tk.par')
    # Need this line to stop error about missing siPixelDigis.
    process.MeasurementTrackerEvent.inactivePixelDetectorLabels = cms.VInputTag()

    # new layer list (3/4 pixel seeding) in InitialStep and pixelTracks
    process.PixelLayerTriplets.layerList = cms.vstring( 'BPix1+BPix2+BPix3',
                                                        'BPix2+BPix3+BPix4',
                                                        'BPix1+BPix3+BPix4',
                                                        'BPix1+BPix2+BPix4',
                                                        'BPix2+BPix3+FPix1_pos',
                                                        'BPix2+BPix3+FPix1_neg',
                                                        'BPix1+BPix2+FPix1_pos',
                                                        'BPix1+BPix2+FPix1_neg',
                                                        'BPix2+FPix1_pos+FPix2_pos',
                                                        'BPix2+FPix1_neg+FPix2_neg',
                                                        'BPix1+FPix1_pos+FPix2_pos',
                                                        'BPix1+FPix1_neg+FPix2_neg',
                                                        'FPix1_pos+FPix2_pos+FPix3_pos',
                                                        'FPix1_neg+FPix2_neg+FPix3_neg' )

    # New tracking.  This is really ugly because it redefines globalreco and reconstruction.
    # It can be removed if change one line in Configuration/StandardSequences/python/Reconstruction_cff.py
    # from RecoTracker_cff.py to RecoTrackerPhase1PU140_cff.py

    # remove all the tracking first
    itIndex=process.globalreco_tracking.index(process.trackingGlobalReco)
    grIndex=process.globalreco.index(process.globalreco_tracking)

    process.globalreco.remove(process.globalreco_tracking)
    process.globalreco_tracking.remove(process.iterTracking)
    process.globalreco_tracking.remove(process.electronSeedsSeq)
    process.reconstruction_fromRECO.remove(process.trackingGlobalReco)
    process.reconstruction_fromRECO.remove(process.electronSeedsSeq)
    process.reconstruction_fromRECO.remove(process.initialStepSeedLayers)
    process.reconstruction_fromRECO.remove(process.initialStepSeeds)
    process.reconstruction_fromRECO.remove(process.initialStepClassifier1)
    process.reconstruction_fromRECO.remove(process.initialStepClassifier2)
    process.reconstruction_fromRECO.remove(process.initialStepClassifier3)
    process.reconstruction_fromRECO.remove(initialStepTrackCandidates)
    process.reconstruction_fromRECO.remove(initialStepTracks)
    process.reconstruction_fromRECO.remove(lowPtTripletStepClusters)
    process.reconstruction_fromRECO.remove(lowPtTripletStepSeedLayers)
    process.reconstruction_fromRECO.remove(lowPtTripletStepSeeds)
    process.reconstruction_fromRECO.remove(lowPtTripletStep)
    process.reconstruction_fromRECO.remove(lowPtTripletStepTrackCandidates)
    process.reconstruction_fromRECO.remove(lowPtTripletStepTracks)

    process.reconstruction_fromRECO.remove(mixedTripletStep)
    process.reconstruction_fromRECO.remove(mixedTripletStepClusters)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedLayersA)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedLayersB)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeeds)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedsA)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedsB)
    process.reconstruction_fromRECO.remove(mixedTripletStepClassifier1)
    process.reconstruction_fromRECO.remove(mixedTripletStepClassifier2)
    process.reconstruction_fromRECO.remove(mixedTripletStepTrackCandidates)
    process.reconstruction_fromRECO.remove(mixedTripletStepTracks)

    process.reconstruction_fromRECO.remove(pixelPairStepClusters)
    process.reconstruction_fromRECO.remove(pixelPairStepSeeds)
    process.reconstruction_fromRECO.remove(pixelPairStepSeedLayers)
    process.reconstruction_fromRECO.remove(pixelPairStep)
    process.reconstruction_fromRECO.remove(pixelPairStepTrackCandidates)
    process.reconstruction_fromRECO.remove(pixelPairStepTracks)
    
    process.reconstruction_fromRECO.remove(tobTecStepClusters)
    process.reconstruction_fromRECO.remove(tobTecStepSeeds)
    #process.reconstruction_fromRECO.remove(tobTecStepSeedLayers)
    process.reconstruction_fromRECO.remove(tobTecStepClassifier1)
    process.reconstruction_fromRECO.remove(tobTecStepClassifier2)
    process.reconstruction_fromRECO.remove(tobTecStep)
    process.reconstruction_fromRECO.remove(tobTecStepTrackCandidates)
    process.reconstruction_fromRECO.remove(tobTecStepTracks)

    # Yes, needs to be done twice for InOut...
    process.reconstruction_fromRECO.remove(process.muonSeededSeedsInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededSeedsInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTrackCandidatesInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTrackCandidatesInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTracksInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTracksInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededSeedsOutIn)
    process.reconstruction_fromRECO.remove(process.muonSeededTrackCandidatesOutIn)
    process.reconstruction_fromRECO.remove(process.muonSeededTracksOutIn)
    # Why are these modules in this sequence (isn't iterTracking enough)?
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededSeedsInOut)
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededTrackCandidatesInOut)
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededTracksInOut)
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededSeedsOutIn)
    process.muonSeededStepExtraDisplaced.remove(process.muonSeededTracksInOutClassifier)

    process.reconstruction_fromRECO.remove(process.convClusters)
    process.reconstruction_fromRECO.remove(process.convLayerPairs)
    process.reconstruction_fromRECO.remove(process.convStepSelector)
    process.reconstruction_fromRECO.remove(process.convTrackCandidates)
    process.reconstruction_fromRECO.remove(process.convStepTracks)
    process.reconstruction_fromRECO.remove(process.photonConvTrajSeedFromSingleLeg)

    process.reconstruction_fromRECO.remove(process.preDuplicateMergingGeneralTracks)

    # Needed to make the loading of recoFromSimDigis_cff below to work
    process.InitialStepPreSplitting.remove(siPixelClusters)

    del process.iterTracking
    del process.ckftracks
    del process.ckftracks_woBH
    del process.ckftracks_wodEdX
    del process.ckftracks_plus_pixelless
    del process.trackingGlobalReco
    del process.electronSeedsSeq
    del process.InitialStep
    del process.LowPtTripletStep
    del process.PixelPairStep
    del process.DetachedTripletStep
    del process.MixedTripletStep
    del process.PixelLessStep
    del process.TobTecStep
    del process.earlyGeneralTracks
    del process.muonSeededStep
    del process.muonSeededStepCore
    del process.muonSeededStepDebug
    del process.muonSeededStepDebugDisplaced
    del process.ConvStep
    # add the correct tracking back in
    process.load("RecoTracker.Configuration.RecoTrackerPhase1PU"+str(nPU)+"_cff")

    process.globalreco_tracking.insert(itIndex,process.trackingGlobalReco)
    process.globalreco.insert(grIndex,process.globalreco_tracking)
    #Note process.reconstruction_fromRECO is broken
    
    # End of new tracking configuration which can be removed if new Reconstruction is used.


    process.reconstruction.remove(process.castorreco)
    process.reconstruction.remove(process.CastorTowerReco)
    process.reconstruction.remove(process.ak5CastorJets)
    process.reconstruction.remove(process.ak5CastorJetID)
    process.reconstruction.remove(process.ak7CastorJets)
    #process.reconstruction.remove(process.ak7BasicJets)
    process.reconstruction.remove(process.ak7CastorJetID)

    #the quadruplet merger configuration     
    process.load("RecoPixelVertexing.PixelTriplets.quadrupletseedmerging_cff")
    process.PixelSeedMergerQuadruplets.BPix.TTRHBuilder = cms.string("PixelTTRHBuilderWithoutAngle" )
    process.PixelSeedMergerQuadruplets.BPix.HitProducer = cms.string("siPixelRecHits" )
    process.PixelSeedMergerQuadruplets.FPix.TTRHBuilder = cms.string("PixelTTRHBuilderWithoutAngle" )
    process.PixelSeedMergerQuadruplets.FPix.HitProducer = cms.string("siPixelRecHits" )    
    
    # Need these until pixel templates are used
    process.load("SLHCUpgradeSimulations.Geometry.recoFromSimDigis_cff")
    # PixelCPEGeneric #
    process.PixelCPEGenericESProducer.Upgrade = cms.bool(True)
    process.PixelCPEGenericESProducer.UseErrorsFromTemplates = cms.bool(False)
    process.PixelCPEGenericESProducer.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.TruncatePixelCharge = cms.bool(False)
    process.PixelCPEGenericESProducer.IrradiationBiasCorrection = False
    process.PixelCPEGenericESProducer.DoCosmics = False
    # CPE for other steps
    process.siPixelRecHits.CPE = cms.string('PixelCPEGeneric')
    # Turn of template use in tracking (iterative steps handled inside their configs)
    process.duplicateTrackCandidates.ttrhBuilderName = 'WithTrackAngle'
    process.mergedDuplicateTracks.TTRHBuilder = 'WithTrackAngle'
    process.ctfWithMaterialTracks.TTRHBuilder = 'WithTrackAngle'
    process.muonSeededSeedsInOut.TrackerRecHitBuilder=cms.string('WithTrackAngle')
    process.muonSeededTracksInOut.TTRHBuilder=cms.string('WithTrackAngle')
    process.muonSeededTracksOutIn.TTRHBuilder=cms.string('WithTrackAngle')
    process.muons1stStep.TrackerKinkFinderParameters.TrackerRecHitBuilder=cms.string('WithTrackAngle')
    process.regionalCosmicTracks.TTRHBuilder=cms.string('WithTrackAngle')
    process.cosmicsVetoTracksRaw.TTRHBuilder=cms.string('WithTrackAngle')
    process.trackerDrivenElectronSeeds.TTRHBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.tevMuons.RefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.tevMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.muonSeededTracksOutInDisplaced.TTRHBuilder = 'WithTrackAngle'
    process.duplicateDisplacedTrackCandidates.ttrhBuilderName = 'WithTrackAngle'
    process.mergedDuplicateDisplacedTracks.TTRHBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.glbTrackQual.RefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    # End of pixel template needed section

    # Remove, for now, the pre-cluster-splitting clustering step
    # To be enabled later together with or after the jet core step is enabled
    # This snippet must be after the loading of recoFromSimDigis_cff
    process.pixeltrackerlocalreco = cms.Sequence(
        process.siPixelClusters +
        process.siPixelRecHits
    )
    process.clusterSummaryProducer.pixelClusters = "siPixelClusters"
    process.globalreco_tracking.replace(process.MeasurementTrackerEventPreSplitting, process.MeasurementTrackerEvent)
    process.globalreco_tracking.replace(process.siPixelClusterShapeCachePreSplitting, process.siPixelClusterShapeCache)

    # Enable, for now, pixel tracks and vertices
    # To be removed later together with the cluster splitting
    process.globalreco_tracking.replace(process.standalonemuontracking,
                                        process.standalonemuontracking+process.recopixelvertexing)
    process.initialStepSelector.vertices = "pixelVertices"
    process.highPtTripletStepSelector.vertices = "pixelVertices"
    process.lowPtQuadStepSelector.vertices = "pixelVertices"
    process.lowPtTripletStepSelector.vertices = "pixelVertices"
    process.detachedQuadStepSelector.vertices = "pixelVertices"
    process.mixedTripletStepSelector.vertices = "pixelVertices"
    process.pixelPairStepSeeds.RegionFactoryPSet.RegionPSet.VertexCollection = "pixelVertices"
    process.pixelPairStepSelector.vertices = "pixelVertices"
    process.tobTecStepSelector.vertices = "pixelVertices"
    process.muonSeededTracksInOutSelector.vertices = "pixelVertices"
    process.muonSeededTracksOutInSelector.vertices = "pixelVertices"
    process.duplicateTrackClassifier.vertices = "pixelVertices"
    process.convStepSelector.vertices = "pixelVertices"
    process.ak4CaloJetsForTrk.srcPVs = "pixelVertices"
    process.muonSeededTracksOutInDisplacedClassifier.vertices = "pixelVertices"
    process.duplicateDisplacedTrackClassifier.vertices = "pixelVertices"

    # Make pixelTracks use quadruplets
    process.pixelTracks.SeedMergerPSet = cms.PSet(
        layerList = cms.PSet(refToPSet_ = cms.string('PixelSeedMergerQuadruplets')),
        addRemainingTriplets = cms.bool(False),
        mergeTriplets = cms.bool(True),
        ttrhBuilderLabel = cms.string('PixelTTRHBuilderWithoutAngle')
        )
    process.pixelTracks.FilterPSet.chi2 = cms.double(50.0)
    process.pixelTracks.FilterPSet.tipMax = cms.double(0.05)
    process.pixelTracks.RegionFactoryPSet.RegionPSet.originRadius =  cms.double(0.02)
    process.templates.DoLorentz=False
    process.templates.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.useLAWidthFromDB = cms.bool(False)

    # This probably breaks badly the "displaced muon" reconstruction,
    # but let's do it for now, until the upgrade tracking sequences
    # are modernized
    process.preDuplicateMergingDisplacedTracks.inputClassifiers.remove("muonSeededTracksInOutClassifier")
    process.preDuplicateMergingDisplacedTracks.trackProducers.remove("muonSeededTracksInOut")

    return process



def customise_Reco_v2(process):
    #use with latest pixel geometry
    process.ClusterShapeHitFilterESProducer.PixelShapeFile = cms.string('RecoPixelVertexing/PixelLowPtUtilities/data/pixelShape_Phase1Tk.par')
    # Need this line to stop error about missing siPixelDigis.
    process.MeasurementTrackerEvent.inactivePixelDetectorLabels = cms.VInputTag()

    # new layer list (3/4 pixel seeding) in InitialStep and pixelTracks
    process.PixelLayerTriplets.layerList = cms.vstring( 'BPix1+BPix2+BPix3',
                                                        'BPix2+BPix3+BPix4',
                                                        'BPix1+BPix3+BPix4',
                                                        'BPix1+BPix2+BPix4',
                                                        'BPix2+BPix3+FPix1_pos',
                                                        'BPix2+BPix3+FPix1_neg',
                                                        'BPix1+BPix2+FPix1_pos',
                                                        'BPix1+BPix2+FPix1_neg',
                                                        'BPix2+FPix1_pos+FPix2_pos',
                                                        'BPix2+FPix1_neg+FPix2_neg',
                                                        'BPix1+FPix1_pos+FPix2_pos',
                                                        'BPix1+FPix1_neg+FPix2_neg',
                                                        'FPix1_pos+FPix2_pos+FPix3_pos',
                                                        'FPix1_neg+FPix2_neg+FPix3_neg' )

    # New tracking.  This is really ugly because it redefines globalreco and reconstruction.
    # It can be removed if change one line in Configuration/StandardSequences/python/Reconstruction_cff.py
    # from RecoTracker_cff.py to RecoTrackerPhase1PU140_cff.py

    # remove all the tracking first
    itIndex=process.globalreco.index(process.trackingGlobalReco)
    grIndex=process.reconstruction.index(process.globalreco)

    process.reconstruction.remove(process.globalreco)
    process.globalreco.remove(process.iterTracking)
    process.globalreco.remove(process.electronSeedsSeq)
    process.reconstruction_fromRECO.remove(process.trackingGlobalReco)
    process.reconstruction_fromRECO.remove(process.electronSeedsSeq)
    process.reconstruction_fromRECO.remove(process.initialStep)
    process.reconstruction_fromRECO.remove(process.initialStepSeedLayers)
    process.reconstruction_fromRECO.remove(process.initialStepSeeds)
    process.reconstruction_fromRECO.remove(process.initialStepClassifier1)
    process.reconstruction_fromRECO.remove(process.initialStepClassifier2)
    process.reconstruction_fromRECO.remove(process.initialStepClassifier3)
    process.reconstruction_fromRECO.remove(initialStepTrackCandidates)
    process.reconstruction_fromRECO.remove(initialStepTracks)
    process.reconstruction_fromRECO.remove(process.initialStepTrackRefsForJets)
    process.reconstruction_fromRECO.remove(process.firstStepPrimaryVertices)
    process.reconstruction_fromRECO.remove(process.firstStepGoodPrimaryVertices)
    process.reconstruction_fromRECO.remove(lowPtTripletStepClusters)
    process.reconstruction_fromRECO.remove(lowPtTripletStepSeedLayers)
    process.reconstruction_fromRECO.remove(lowPtTripletStepSeeds)
    process.reconstruction_fromRECO.remove(lowPtTripletStep)
    process.reconstruction_fromRECO.remove(lowPtTripletStepTrackCandidates)
    process.reconstruction_fromRECO.remove(lowPtTripletStepTracks)

    process.reconstruction_fromRECO.remove(process.detachedTripletStep)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepClusters)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepSeedLayers)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepSeeds)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepTrackCandidates)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepTracks)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepClassifier1)
    process.reconstruction_fromRECO.remove(process.detachedTripletStepClassifier2)

    process.reconstruction_fromRECO.remove(mixedTripletStep)
    process.reconstruction_fromRECO.remove(mixedTripletStepClusters)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedLayersA)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedLayersB)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeeds)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedsA)
    process.reconstruction_fromRECO.remove(mixedTripletStepSeedsB)
    process.reconstruction_fromRECO.remove(mixedTripletStepClassifier1)
    process.reconstruction_fromRECO.remove(mixedTripletStepClassifier2)
    process.reconstruction_fromRECO.remove(mixedTripletStepTrackCandidates)
    process.reconstruction_fromRECO.remove(mixedTripletStepTracks)

    process.reconstruction_fromRECO.remove(pixelPairStepClusters)
    process.reconstruction_fromRECO.remove(pixelPairStepSeeds)
    process.reconstruction_fromRECO.remove(pixelPairStepSeedLayers)
    process.reconstruction_fromRECO.remove(pixelPairStep)
    process.reconstruction_fromRECO.remove(pixelPairStepTrackCandidates)
    process.reconstruction_fromRECO.remove(pixelPairStepTracks)

    process.reconstruction_fromRECO.remove(process.pixelLessStep)
    process.reconstruction_fromRECO.remove(process.pixelLessStepClusters)
    process.reconstruction_fromRECO.remove(process.pixelLessStepSeeds)
    process.reconstruction_fromRECO.remove(process.pixelLessStepSeedLayers)
    process.reconstruction_fromRECO.remove(process.pixelLessStepTrackCandidates)
    process.reconstruction_fromRECO.remove(process.pixelLessStepTracks)
    process.reconstruction_fromRECO.remove(process.pixelLessStepClassifier1)
    process.reconstruction_fromRECO.remove(process.pixelLessStepClassifier2)
    
    process.reconstruction_fromRECO.remove(tobTecStepClusters)
    process.reconstruction_fromRECO.remove(tobTecStepSeeds)
    process.reconstruction_fromRECO.remove(process.tobTecStepSeedLayersPair)
    process.reconstruction_fromRECO.remove(process.tobTecStepSeedLayersTripl)
    process.reconstruction_fromRECO.remove(process.tobTecStepSeedsPair)
    process.reconstruction_fromRECO.remove(process.tobTecStepSeedsTripl)
    #process.reconstruction_fromRECO.remove(tobTecStepSeedLayers)
    process.reconstruction_fromRECO.remove(tobTecStepClassifier1)
    process.reconstruction_fromRECO.remove(tobTecStepClassifier2)
    process.reconstruction_fromRECO.remove(tobTecStep)
    process.reconstruction_fromRECO.remove(tobTecStepTrackCandidates)
    process.reconstruction_fromRECO.remove(tobTecStepTracks)

    process.reconstruction_fromRECO.remove(process.jetCoreRegionalStep)
    process.reconstruction_fromRECO.remove(process.jetCoreRegionalStepSeedLayers)
    process.reconstruction_fromRECO.remove(process.jetCoreRegionalStepSeeds)
    process.reconstruction_fromRECO.remove(process.jetCoreRegionalStepTrackCandidates)
    process.reconstruction_fromRECO.remove(process.jetCoreRegionalStepTracks)
    process.reconstruction_fromRECO.remove(process.jetsForCoreTracking)

    # Yes, needs to be done twice for InOut...
    process.reconstruction_fromRECO.remove(process.muonSeededSeedsInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededSeedsInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTrackCandidatesInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTrackCandidatesInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTracksInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededTracksInOut)
    process.reconstruction_fromRECO.remove(process.muonSeededSeedsOutIn)
    process.reconstruction_fromRECO.remove(process.muonSeededTrackCandidatesOutIn)
    process.reconstruction_fromRECO.remove(process.muonSeededTracksOutIn)
    # Why are these modules in this sequence (isn't iterTracking enough)?
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededSeedsInOut)
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededTrackCandidatesInOut)
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededTracksInOut)
    process.muonSeededStepCoreDisplaced.remove(process.muonSeededSeedsOutIn)
    process.muonSeededStepExtraDisplaced.remove(process.muonSeededTracksInOutClassifier)

    process.reconstruction_fromRECO.remove(process.convClusters)
    process.reconstruction_fromRECO.remove(process.convLayerPairs)
    process.reconstruction_fromRECO.remove(process.convStepSelector)
    process.reconstruction_fromRECO.remove(process.convTrackCandidates)
    process.reconstruction_fromRECO.remove(process.convStepTracks)
    process.reconstruction_fromRECO.remove(process.photonConvTrajSeedFromSingleLeg)

    process.reconstruction_fromRECO.remove(process.preDuplicateMergingGeneralTracks)

    process.reconstruction_fromRECO.remove(process.chargeCut2069Clusters)

    # Needed to make the loading of recoFromSimDigis_cff below to work
    process.InitialStepPreSplitting.remove(siPixelClusters)

    del process.iterTracking
    del process.ckftracks
    del process.ckftracks_woBH
    del process.ckftracks_wodEdX
    del process.ckftracks_plus_pixelless
    del process.trackingGlobalReco
    del process.electronSeedsSeq
    del process.InitialStepPreSplitting
    del process.InitialStep
    del process.LowPtTripletStep
    del process.PixelPairStep
    del process.DetachedTripletStep
    del process.MixedTripletStep
    del process.PixelLessStep
    del process.TobTecStep
    del process.JetCoreRegionalStep
    del process.earlyGeneralTracks
    del process.muonSeededStep
    del process.muonSeededStepCore
    del process.muonSeededStepDebug
    del process.muonSeededStepDebugDisplaced
    del process.ConvStep

    # Need these until pixel templates are used, and need to load this
    # before the tracking configuration
    process.load("SLHCUpgradeSimulations.Geometry.recoFromSimDigis_cff")

    # add the correct tracking back in
    process.load("RecoTracker.Configuration.RecoTrackerPhase1_cff")

    process.globalreco.insert(itIndex,process.trackingGlobalReco)
    process.reconstruction.insert(grIndex,process.globalreco)
    #Note process.reconstruction_fromRECO is broken
    
    # End of new tracking configuration which can be removed if new Reconstruction is used.


    process.reconstruction.remove(process.castorreco)
    process.reconstruction.remove(process.CastorTowerReco)
    process.reconstruction.remove(process.ak5CastorJets)
    process.reconstruction.remove(process.ak5CastorJetID)
    process.reconstruction.remove(process.ak7CastorJets)
    #process.reconstruction.remove(process.ak7BasicJets)
    process.reconstruction.remove(process.ak7CastorJetID)

    #the quadruplet merger configuration     
    process.load("RecoPixelVertexing.PixelTriplets.quadrupletseedmerging_cff")
    process.PixelSeedMergerQuadruplets.BPix.TTRHBuilder = cms.string("PixelTTRHBuilderWithoutAngle" )
    process.PixelSeedMergerQuadruplets.BPix.HitProducer = cms.string("siPixelRecHits" )
    process.PixelSeedMergerQuadruplets.FPix.TTRHBuilder = cms.string("PixelTTRHBuilderWithoutAngle" )
    process.PixelSeedMergerQuadruplets.FPix.HitProducer = cms.string("siPixelRecHits" )    
    
    # Need these until pixel templates are used
    # PixelCPEGeneric #
    process.PixelCPEGenericESProducer.Upgrade = cms.bool(True)
    process.PixelCPEGenericESProducer.UseErrorsFromTemplates = cms.bool(False)
    process.PixelCPEGenericESProducer.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.TruncatePixelCharge = cms.bool(False)
    process.PixelCPEGenericESProducer.IrradiationBiasCorrection = False
    process.PixelCPEGenericESProducer.DoCosmics = False
    # CPE for other steps
    process.siPixelRecHits.CPE = cms.string('PixelCPEGeneric')
    # Turn of template use in tracking (iterative steps handled inside their configs)
    process.duplicateTrackCandidates.ttrhBuilderName = 'WithTrackAngle'
    process.convStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.mergedDuplicateTracks.TTRHBuilder = 'WithTrackAngle'
    process.ctfWithMaterialTracks.TTRHBuilder = 'WithTrackAngle'
    process.muonSeededSeedsInOut.TrackerRecHitBuilder=cms.string('WithTrackAngle')
    process.muonSeededTracksInOut.TTRHBuilder=cms.string('WithTrackAngle')
    process.muonSeededTracksOutIn.TTRHBuilder=cms.string('WithTrackAngle')
    process.muons1stStep.TrackerKinkFinderParameters.TrackerRecHitBuilder=cms.string('WithTrackAngle')
    process.regionalCosmicTracks.TTRHBuilder=cms.string('WithTrackAngle')
    process.cosmicsVetoTracksRaw.TTRHBuilder=cms.string('WithTrackAngle')
    process.trackerDrivenElectronSeeds.TTRHBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.tevMuons.RefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.tevMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.muonSeededTracksOutInDisplaced.TTRHBuilder = 'WithTrackAngle'
    process.duplicateDisplacedTrackCandidates.ttrhBuilderName = 'WithTrackAngle'
    process.mergedDuplicateDisplacedTracks.TTRHBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.glbTrackQual.RefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    # End of pixel template needed section

    process.templates.DoLorentz=False
    process.templates.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.useLAWidthFromDB = cms.bool(False)

    # This probably breaks badly the "displaced muon" reconstruction,
    # but let's do it for now, until the upgrade tracking sequences
    # are modernized
    process.preDuplicateMergingDisplacedTracks.inputClassifiers.remove("muonSeededTracksInOutClassifier")
    process.preDuplicateMergingDisplacedTracks.trackProducers.remove("muonSeededTracksInOut")

    # Disable CCC
    process.SiStripClusterChargeCutLoose.value = -1
    process.SiStripClusterChargeCutTight.value = -1

    # Seed creator
    process.initialStepSeeds.SeedCreatorPSet.magneticField = ''
    process.initialStepSeeds.SeedCreatorPSet.propagator = 'PropagatorWithMaterial'
    process.highPtTripletStepSeeds.SeedCreatorPSet.magneticField = ''
    process.highPtTripletStepSeeds.SeedCreatorPSet.propagator = 'PropagatorWithMaterial'
    
    # maxCand
    process.initialStepTrajectoryBuilder.maxCand = 6
    process.highPtTripletStepTrajectoryBuilder.maxCand = 4

    # trajectory cleaner
    from TrackingTools.TrajectoryCleaning.TrajectoryCleanerBySharedHits_cfi import trajectoryCleanerBySharedHits
    process.highPtTripletStepTrajectoryCleanerBySharedHits = trajectoryCleanerBySharedHits.clone(
        ComponentName = cms.string('highPtTripletStepTrajectoryCleanerBySharedHits'),
        fractionShared = cms.double(0.16),
        allowSharedFirstHit = cms.bool(True)
    )
    process.highPtTripletStepTrackCandidates.TrajectoryCleaner = 'highPtTripletStepTrajectoryCleanerBySharedHits'

    # pixel cluster splitting
    process.iterTracking.remove(process.InitialStepPreSplitting)
    process.siPixelClusters = process.siPixelClustersPreSplitting.clone()
    process.pixeltrackerlocalreco.replace(process.siPixelClustersPreSplitting, process.siPixelClusters)
    process.pixeltrackerlocalreco.replace(process.siPixelRecHitsPreSplitting, process.siPixelRecHits)
    process.clusterSummaryProducer.pixelClusters = 'siPixelClusters'
    process.reconstruction.replace(process.MeasurementTrackerEventPreSplitting, process.MeasurementTrackerEvent)
    process.reconstruction.replace(process.siPixelClusterShapeCachePreSplitting, process.siPixelClusterShapeCache)

    ## pixel vertices
    # Make pixelTracks use quadruplets
    process.pixelTracks.SeedMergerPSet = cms.PSet(
        layerList = cms.PSet(refToPSet_ = cms.string('PixelSeedMergerQuadruplets')),
        addRemainingTriplets = cms.bool(False),
        mergeTriplets = cms.bool(True),
        ttrhBuilderLabel = cms.string('PixelTTRHBuilderWithoutAngle')
        )
    process.pixelTracks.FilterPSet.chi2 = cms.double(50.0)
    process.pixelTracks.FilterPSet.tipMax = cms.double(0.05)
    process.pixelTracks.RegionFactoryPSet.RegionPSet.originRadius =  cms.double(0.02)
    process.templates.DoLorentz=False
    process.templates.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.useLAWidthFromDB = cms.bool(False)

    # Enable, for now, pixel tracks and vertices
    # To be removed later together with the cluster splitting
    process.reconstruction.replace(process.standalonemuontracking,
                                   process.standalonemuontracking+process.recopixelvertexing)
    process.initialStepClassifier1.vertices = "pixelVertices"
    process.initialStepClassifier2.vertices = "pixelVertices"
    process.initialStepClassifier3.vertices = "pixelVertices"
    process.highPtTripletStepClassifier1.vertices = "pixelVertices"
    process.highPtTripletStepClassifier2.vertices = "pixelVertices"
    process.highPtTripletStepClassifier3.vertices = "pixelVertices"
    process.detachedQuadStepClassifier1.vertices = "pixelVertices"
    process.detachedQuadStepClassifier2.vertices = "pixelVertices"
    process.detachedTripletStepClassifier1.vertices = "pixelVertices"
    process.detachedTripletStepClassifier2.vertices = "pixelVertices"
    process.lowPtQuadStep.vertices = "pixelVertices"
    process.lowPtTripletStep.vertices = "pixelVertices"
    process.mixedTripletStepClassifier1.vertices = "pixelVertices"
    process.mixedTripletStepClassifier2.vertices = "pixelVertices"
    process.pixelLessStepClassifier1.vertices = "pixelVertices"
    process.pixelLessStepClassifier2.vertices = "pixelVertices"
    process.tobTecStepClassifier1.vertices = "pixelVertices"
    process.tobTecStepClassifier2.vertices = "pixelVertices"
    process.jetCoreRegionalStep.vertices = "pixelVertices"
    process.muonSeededTracksInOutClassifier.vertices = "pixelVertices"
    process.muonSeededTracksOutInClassifier.vertices = "pixelVertices"
    process.duplicateTrackClassifier.vertices = "pixelVertices"
    process.convStepSelector.vertices = "pixelVertices"
    process.ak4CaloJetsForTrk.srcPVs = "pixelVertices"
    process.muonSeededTracksOutInDisplacedClassifier.vertices = "pixelVertices"
    process.duplicateDisplacedTrackClassifier.vertices = "pixelVertices"
    ## end pixel vertices

    # cut-based track selector, initialStep
    process.InitialStep.remove(process.initialStepClassifier1)
    process.InitialStep.remove(process.initialStepClassifier2)
    process.InitialStep.remove(process.initialStepClassifier3)
    process.InitialStep.remove(process.initialStep)

    from RecoTracker.FinalTrackSelectors.TrackCutClassifier_cfi import TrackCutClassifier
    process.initialStep = TrackCutClassifier.clone(
        src = "initialStepTracks",
        vertices = "pixelVertices",
    )
    process.initialStep.mva.minPixelHits = [1,1,1]
    process.initialStep.mva.maxChi2 = [9999.,9999.,9999.]
    process.initialStep.mva.maxChi2n = [1.6,1.0,0.7]
    process.initialStep.mva.minLayers = [3,3,3]
    process.initialStep.mva.min3DLayers = [3,3,3]
    process.initialStep.mva.maxLostLayers = [3,2,2]
    process.initialStep.mva.maxDz = [0.7,0.6,0.55];
    process.initialStep.mva.maxDr = [0.4,0.3,0.2];
    process.InitialStep += process.initialStep

    # cut-based track selector, highPtTripletStep
    process.HighPtTripletStep.remove(process.highPtTripletStepClassifier1)
    process.HighPtTripletStep.remove(process.highPtTripletStepClassifier2)
    process.HighPtTripletStep.remove(process.highPtTripletStepClassifier3)
    process.HighPtTripletStep.remove(process.highPtTripletStep)

    from RecoTracker.FinalTrackSelectors.TrackCutClassifier_cfi import TrackCutClassifier
    process.highPtTripletStep = TrackCutClassifier.clone(
        src = "highPtTripletStepTracks",
        vertices = "pixelVertices",
    )
    process.highPtTripletStep.mva.minPixelHits = [1,1,1]
    process.highPtTripletStep.mva.maxChi2 = [9999.,9999.,9999.]
    process.highPtTripletStep.mva.maxChi2n = [1.6,1.0,0.7]
    process.highPtTripletStep.mva.minLayers = [3,3,3]
    process.highPtTripletStep.mva.min3DLayers = [3,3,3]
    process.highPtTripletStep.mva.maxLostLayers = [3,2,2]
    process.highPtTripletStep.mva.maxDz = [0.7,0.6,0.55];
    process.highPtTripletStep.mva.maxDr = [0.4,0.3,0.2];
    process.HighPtTripletStep += process.highPtTripletStep

    return process

def customise_Reco_Run2(process):
    #use with latest pixel geometry
    process.ClusterShapeHitFilterESProducer.PixelShapeFile = cms.string('RecoPixelVertexing/PixelLowPtUtilities/data/pixelShape_Phase1Tk.par')
    # Need this line to stop error about missing siPixelDigis.
    process.MeasurementTrackerEvent.inactivePixelDetectorLabels = cms.VInputTag()

    # Needed to make the loading of recoFromSimDigis_cff below to work
    pixelClusterIndex = process.InitialStepPreSplitting.index(process.siPixelClusters)
    process.InitialStepPreSplitting.remove(siPixelClusters)

    # Need these until pixel templates are used, and need to load this
    # before the tracking configuration
    process.load("SLHCUpgradeSimulations.Geometry.recoFromSimDigis_cff")

    process.InitialStepPreSplitting.insert(pixelClusterIndex, process.siPixelClusters)

    # End of new tracking configuration which can be removed if new Reconstruction is used.


    process.reconstruction.remove(process.castorreco)
    process.reconstruction.remove(process.CastorTowerReco)
    process.reconstruction.remove(process.ak5CastorJets)
    process.reconstruction.remove(process.ak5CastorJetID)
    process.reconstruction.remove(process.ak7CastorJets)
    #process.reconstruction.remove(process.ak7BasicJets)
    process.reconstruction.remove(process.ak7CastorJetID)

    # Need these until pixel templates are used
    # PixelCPEGeneric #
    process.PixelCPEGenericESProducer.Upgrade = cms.bool(True)
    process.PixelCPEGenericESProducer.UseErrorsFromTemplates = cms.bool(False)
    process.PixelCPEGenericESProducer.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.TruncatePixelCharge = cms.bool(False)
    process.PixelCPEGenericESProducer.IrradiationBiasCorrection = False
    process.PixelCPEGenericESProducer.DoCosmics = False
    # CPE for other steps
    process.siPixelRecHits.CPE = cms.string('PixelCPEGeneric')
    # Turn of template use in tracking
    process.initialStepTracksPreSplitting.TTRHBuilder = 'WithTrackAngle'
    process.initialStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.detachedTripletStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.lowPtTripletStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.pixelPairStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.mixedTripletStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.pixelLessStepTracks.TTRHBuilder = 'WithTrackAngle' 
    process.tobTecStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.jetCoreRegionalStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.duplicateTrackCandidates.ttrhBuilderName = 'WithTrackAngle'
    process.convStepTracks.TTRHBuilder = 'WithTrackAngle'
    process.mergedDuplicateTracks.TTRHBuilder = 'WithTrackAngle'
    process.ctfWithMaterialTracks.TTRHBuilder = 'WithTrackAngle'
    process.muonSeededSeedsInOut.TrackerRecHitBuilder=cms.string('WithTrackAngle')
    process.muonSeededTracksInOut.TTRHBuilder=cms.string('WithTrackAngle')
    process.muonSeededTracksOutIn.TTRHBuilder=cms.string('WithTrackAngle')
    process.muons1stStep.TrackerKinkFinderParameters.TrackerRecHitBuilder=cms.string('WithTrackAngle')
    process.regionalCosmicTracks.TTRHBuilder=cms.string('WithTrackAngle')
    process.cosmicsVetoTracksRaw.TTRHBuilder=cms.string('WithTrackAngle')
    process.trackerDrivenElectronSeeds.TTRHBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.tevMuons.RefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.tevMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.muonSeededTracksOutInDisplaced.TTRHBuilder = 'WithTrackAngle'
    process.duplicateDisplacedTrackCandidates.ttrhBuilderName = 'WithTrackAngle'
    process.mergedDuplicateDisplacedTracks.TTRHBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.displacedGlobalMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    process.glbTrackQual.RefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.GlbRefitterParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.TrackTransformer.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.GLBTrajBuilderParameters.TrackerRecHitBuilder = 'WithTrackAngle'
    process.globalSETMuons.TrackLoaderParameters.TTRHBuilder = 'WithTrackAngle'
    # End of pixel template needed section

    process.templates.DoLorentz=False
    process.templates.LoadTemplatesFromDB = cms.bool(False)
    process.PixelCPEGenericESProducer.useLAWidthFromDB = cms.bool(False)

    return process
