import FWCore.ParameterSet.Config as cms

###############################################################
# Large impact parameter Tracking using mixed-triplet seeding #
###############################################################

#here just for backward compatibility
chargeCut2069Clusters =  cms.EDProducer("ClusterChargeMasker",
    oldClusterRemovalInfo = cms.InputTag("pixelPairStepClusters"),
    pixelClusters = cms.InputTag("siPixelClusters"),
    stripClusters = cms.InputTag("siStripClusters"),
    clusterChargeCut = cms.PSet(refToPSet_ = cms.string('SiStripClusterChargeCutTight'))
)

from RecoLocalTracker.SubCollectionProducers.trackClusterRemover_cfi import *
mixedTripletStepClusters = trackClusterRemover.clone(
    maxChi2                                  = cms.double(9.0),
    trajectories                             = cms.InputTag("pixelPairStepTracks"),
    pixelClusters                            = cms.InputTag("siPixelClusters"),
    stripClusters                            = cms.InputTag("siStripClusters"),
#    oldClusterRemovalInfo                    = cms.InputTag("pixelPairStepClusters"),
    oldClusterRemovalInfo                    = cms.InputTag("chargeCut2069Clusters"),
    trackClassifier                          = cms.InputTag('pixelPairStep',"QualityMasks"),
    TrackQuality                             = cms.string('highPurity'),
    minNumberOfLayersWithMeasBeforeFiltering = cms.int32(0),
)

# SEEDING LAYERS
from RecoLocalTracker.SiStripClusterizer.SiStripClusterChargeCut_cfi import *
from RecoTracker.IterativeTracking.DetachedTripletStep_cff import detachedTripletStepSeedLayers
mixedTripletStepSeedLayersA = cms.EDProducer("SeedingLayersEDProducer",
     layerList = cms.vstring('BPix2+FPix1_pos+FPix2_pos', 'BPix2+FPix1_neg+FPix2_neg'),
#    layerList = cms.vstring('BPix1+BPix2+BPix3', 
#        'BPix1+BPix2+FPix1_pos', 'BPix1+BPix2+FPix1_neg', 
#        'BPix1+FPix1_pos+FPix2_pos', 'BPix1+FPix1_neg+FPix2_neg', 
#        'BPix2+FPix1_pos+FPix2_pos', 'BPix2+FPix1_neg+FPix2_neg'),
    BPix = cms.PSet(
        TTRHBuilder = cms.string('WithTrackAngle'),
        HitProducer = cms.string('siPixelRecHits'),
        skipClusters = cms.InputTag('mixedTripletStepClusters')
    ),
    FPix = cms.PSet(
        TTRHBuilder = cms.string('WithTrackAngle'),
        HitProducer = cms.string('siPixelRecHits'),
        skipClusters = cms.InputTag('mixedTripletStepClusters')
    ),
    TEC = cms.PSet(
        matchedRecHits = cms.InputTag("siStripMatchedRecHits","matchedRecHit"),
        useRingSlector = cms.bool(True),
        TTRHBuilder = cms.string('WithTrackAngle'), clusterChargeCut = cms.PSet(refToPSet_ = cms.string('SiStripClusterChargeCutTight')),
        minRing = cms.int32(1),
        maxRing = cms.int32(1),
        skipClusters = cms.InputTag('mixedTripletStepClusters')
    )
)

# SEEDS
from RecoPixelVertexing.PixelTriplets.PixelTripletLargeTipGenerator_cfi import *
PixelTripletLargeTipGenerator.extraHitRZtolerance = 0.0
PixelTripletLargeTipGenerator.extraHitRPhitolerance = 0.0
import RecoTracker.TkSeedGenerator.GlobalSeedsFromTriplets_cff
mixedTripletStepSeedsA = RecoTracker.TkSeedGenerator.GlobalSeedsFromTriplets_cff.globalSeedsFromTriplets.clone()
mixedTripletStepSeedsA.OrderedHitsFactoryPSet.SeedingLayers = 'mixedTripletStepSeedLayersA'
mixedTripletStepSeedsA.OrderedHitsFactoryPSet.GeneratorPSet = cms.PSet(PixelTripletLargeTipGenerator)
mixedTripletStepSeedsA.SeedCreatorPSet.ComponentName = 'SeedFromConsecutiveHitsTripletOnlyCreator'
mixedTripletStepSeedsA.RegionFactoryPSet.RegionPSet.ptMin = 0.4
mixedTripletStepSeedsA.RegionFactoryPSet.RegionPSet.originHalfLength = 15.0
mixedTripletStepSeedsA.RegionFactoryPSet.RegionPSet.originRadius = 1.5

import RecoPixelVertexing.PixelLowPtUtilities.ClusterShapeHitFilterESProducer_cfi
mixedTripletStepClusterShapeHitFilter  = RecoPixelVertexing.PixelLowPtUtilities.ClusterShapeHitFilterESProducer_cfi.ClusterShapeHitFilterESProducer.clone(
	ComponentName = cms.string('mixedTripletStepClusterShapeHitFilter'),
        PixelShapeFile= cms.string('RecoPixelVertexing/PixelLowPtUtilities/data/pixelShape.par'),
	clusterChargeCut = cms.PSet(refToPSet_ = cms.string('SiStripClusterChargeCutTight'))
	)
	
mixedTripletStepSeedsA.SeedComparitorPSet = cms.PSet(
        ComponentName = cms.string('PixelClusterShapeSeedComparitor'),
        FilterAtHelixStage = cms.bool(False),
        FilterPixelHits = cms.bool(True),
        FilterStripHits = cms.bool(True),
        ClusterShapeHitFilterName = cms.string('mixedTripletStepClusterShapeHitFilter'),
        ClusterShapeCacheSrc = cms.InputTag('siPixelClusterShapeCache')
    )

# SEEDING LAYERS
mixedTripletStepSeedLayersB = cms.EDProducer("SeedingLayersEDProducer",
    layerList = cms.vstring('BPix2+BPix3+TIB1'),
    BPix = cms.PSet(
        TTRHBuilder = cms.string('WithTrackAngle'),
        HitProducer = cms.string('siPixelRecHits'),
        skipClusters = cms.InputTag('mixedTripletStepClusters')
    ),
    TIB = cms.PSet(
        matchedRecHits = cms.InputTag("siStripMatchedRecHits","matchedRecHit"),
        TTRHBuilder = cms.string('WithTrackAngle'), clusterChargeCut = cms.PSet(refToPSet_ = cms.string('SiStripClusterChargeCutTight')),
        skipClusters = cms.InputTag('mixedTripletStepClusters')
    )
)

# SEEDS
from RecoPixelVertexing.PixelTriplets.PixelTripletLargeTipGenerator_cfi import *
PixelTripletLargeTipGenerator.extraHitRZtolerance = 0.0
PixelTripletLargeTipGenerator.extraHitRPhitolerance = 0.0
import RecoTracker.TkSeedGenerator.GlobalSeedsFromTriplets_cff
mixedTripletStepSeedsB = RecoTracker.TkSeedGenerator.GlobalSeedsFromTriplets_cff.globalSeedsFromTriplets.clone()
mixedTripletStepSeedsB.OrderedHitsFactoryPSet.SeedingLayers = 'mixedTripletStepSeedLayersB'
mixedTripletStepSeedsB.OrderedHitsFactoryPSet.GeneratorPSet = cms.PSet(PixelTripletLargeTipGenerator)
mixedTripletStepSeedsB.SeedCreatorPSet.ComponentName = 'SeedFromConsecutiveHitsTripletOnlyCreator'
mixedTripletStepSeedsB.RegionFactoryPSet.RegionPSet.ptMin = 0.6
mixedTripletStepSeedsB.RegionFactoryPSet.RegionPSet.originHalfLength = 10.0
mixedTripletStepSeedsB.RegionFactoryPSet.RegionPSet.originRadius = 1.5

mixedTripletStepSeedsB.SeedComparitorPSet = cms.PSet(
        ComponentName = cms.string('PixelClusterShapeSeedComparitor'),
        FilterAtHelixStage = cms.bool(False),
        FilterPixelHits = cms.bool(True),
        FilterStripHits = cms.bool(True),
        ClusterShapeHitFilterName = cms.string('mixedTripletStepClusterShapeHitFilter'),
        ClusterShapeCacheSrc = cms.InputTag('siPixelClusterShapeCache')
    )

import RecoTracker.TkSeedGenerator.GlobalCombinedSeeds_cfi
mixedTripletStepSeeds = RecoTracker.TkSeedGenerator.GlobalCombinedSeeds_cfi.globalCombinedSeeds.clone()
mixedTripletStepSeeds.seedCollections = cms.VInputTag(
        cms.InputTag('mixedTripletStepSeedsA'),
        cms.InputTag('mixedTripletStepSeedsB'),
        )

# QUALITY CUTS DURING TRACK BUILDING
import TrackingTools.TrajectoryFiltering.TrajectoryFilter_cff
mixedTripletStepTrajectoryFilter = TrackingTools.TrajectoryFiltering.TrajectoryFilter_cff.CkfBaseTrajectoryFilter_block.clone(
#    maxLostHits = 0,
    constantValueForLostHitsFractionFilter = 1.4,
    minimumNumberOfHits = 3,
    minPt = 0.1
    )

# Propagator taking into account momentum uncertainty in multiple scattering calculation.
import TrackingTools.MaterialEffects.MaterialPropagatorParabolicMf_cff
import TrackingTools.MaterialEffects.MaterialPropagator_cfi
mixedTripletStepPropagator = TrackingTools.MaterialEffects.MaterialPropagator_cfi.MaterialPropagator.clone(
#mixedTripletStepPropagator = TrackingTools.MaterialEffects.MaterialPropagatorParabolicMf_cff.MaterialPropagatorParabolicMF.clone(
    ComponentName = 'mixedTripletStepPropagator',
    ptMin = 0.1
    )

import TrackingTools.MaterialEffects.OppositeMaterialPropagator_cfi
mixedTripletStepPropagatorOpposite = TrackingTools.MaterialEffects.OppositeMaterialPropagator_cfi.OppositeMaterialPropagator.clone(
#mixedTripletStepPropagatorOpposite = TrackingTools.MaterialEffects.MaterialPropagatorParabolicMf_cff.OppositeMaterialPropagatorParabolicMF.clone(
    ComponentName = 'mixedTripletStepPropagatorOpposite',
    ptMin = 0.1
    )

import RecoTracker.MeasurementDet.Chi2ChargeMeasurementEstimator_cfi
mixedTripletStepChi2Est = RecoTracker.MeasurementDet.Chi2ChargeMeasurementEstimator_cfi.Chi2ChargeMeasurementEstimator.clone(
    ComponentName = cms.string('mixedTripletStepChi2Est'),
    nSigma = cms.double(3.0),
    MaxChi2 = cms.double(16.0),
    clusterChargeCut = cms.PSet(refToPSet_ = cms.string('SiStripClusterChargeCutTight'))
)

# TRACK BUILDING
import RecoTracker.CkfPattern.GroupedCkfTrajectoryBuilder_cfi
mixedTripletStepTrajectoryBuilder = RecoTracker.CkfPattern.GroupedCkfTrajectoryBuilder_cfi.GroupedCkfTrajectoryBuilder.clone(
    MeasurementTrackerName = '',
    trajectoryFilter = cms.PSet(refToPSet_ = cms.string('mixedTripletStepTrajectoryFilter')),
    propagatorAlong = cms.string('mixedTripletStepPropagator'),
    propagatorOpposite = cms.string('mixedTripletStepPropagatorOpposite'),
    maxCand = 2,
    estimator = cms.string('mixedTripletStepChi2Est'),
    maxDPhiForLooperReconstruction = cms.double(2.0),
    maxPtForLooperReconstruction = cms.double(0.7) 
    )

# MAKING OF TRACK CANDIDATES
import RecoTracker.CkfPattern.CkfTrackCandidates_cfi
mixedTripletStepTrackCandidates = RecoTracker.CkfPattern.CkfTrackCandidates_cfi.ckfTrackCandidates.clone(
    src = cms.InputTag('mixedTripletStepSeeds'),
    clustersToSkip = cms.InputTag('mixedTripletStepClusters'),
    ### these two parameters are relevant only for the CachingSeedCleanerBySharedInput
    numHitsForSeedCleaner = cms.int32(50),
    #onlyPixelHitsForSeedCleaner = cms.bool(True),

    TrajectoryBuilderPSet = cms.PSet(refToPSet_ = cms.string('mixedTripletStepTrajectoryBuilder')),
    doSeedingRegionRebuilding = True,
    useHitsSplitting = True
)


from TrackingTools.TrajectoryCleaning.TrajectoryCleanerBySharedHits_cfi import trajectoryCleanerBySharedHits
mixedTripletStepTrajectoryCleanerBySharedHits = trajectoryCleanerBySharedHits.clone(
        ComponentName = cms.string('mixedTripletStepTrajectoryCleanerBySharedHits'),
            fractionShared = cms.double(0.11),
            allowSharedFirstHit = cms.bool(True)
            )
mixedTripletStepTrackCandidates.TrajectoryCleaner = 'mixedTripletStepTrajectoryCleanerBySharedHits'


# TRACK FITTING
import RecoTracker.TrackProducer.TrackProducer_cfi
mixedTripletStepTracks = RecoTracker.TrackProducer.TrackProducer_cfi.TrackProducer.clone(
    AlgorithmName = cms.string('mixedTripletStep'),
    src = 'mixedTripletStepTrackCandidates',
    Fitter = cms.string('FlexibleKFFittingSmoother')
)

# TRACK SELECTION AND QUALITY FLAG SETTING.
from RecoTracker.FinalTrackSelectors.TrackMVAClassifierPrompt_cfi import *
from RecoTracker.FinalTrackSelectors.TrackMVAClassifierDetached_cfi import *
mixedTripletStepClassifier1 = TrackMVAClassifierDetached.clone()
mixedTripletStepClassifier1.src = 'mixedTripletStepTracks'
mixedTripletStepClassifier1.GBRForestLabel = 'MVASelectorIter4_13TeV'
mixedTripletStepClassifier1.qualityCuts = [-0.5,0.0,0.5]
mixedTripletStepClassifier2 = TrackMVAClassifierPrompt.clone()
mixedTripletStepClassifier2.src = 'mixedTripletStepTracks'
mixedTripletStepClassifier2.GBRForestLabel = 'MVASelectorIter0_13TeV'
mixedTripletStepClassifier2.qualityCuts = [-0.2,-0.2,-0.2]

from RecoTracker.FinalTrackSelectors.ClassifierMerger_cfi import *
mixedTripletStep = ClassifierMerger.clone()
mixedTripletStep.inputClassifiers=['mixedTripletStepClassifier1','mixedTripletStepClassifier2']





MixedTripletStep = cms.Sequence(chargeCut2069Clusters*mixedTripletStepClusters*
                                mixedTripletStepSeedLayersA*
                                mixedTripletStepSeedsA*
                                mixedTripletStepSeedLayersB*
                                mixedTripletStepSeedsB*
                                mixedTripletStepSeeds*
                                mixedTripletStepTrackCandidates*
                                mixedTripletStepTracks*
                                mixedTripletStepClassifier1*mixedTripletStepClassifier2*
                                mixedTripletStep)

from Configuration.StandardSequences.Eras import eras
# Customization for phase1
def _modifyForPhase1(process):
    # Cluster mask
    mixedTripletStepClusters.trajectories = "detachedQuadStepTracks"
    mixedTripletStepClusters.oldClusterRemovalInfo ="detachedQuadStepClusters"
    del mixedTripletStepClusters.trackClassifier
    mixedTripletStepClusters.overrideTrkQuals = "detachedQuadStep"

    # Seeding layers
    mixedTripletStepSeedLayersA.layerList = ['BPix1+BPix2+FPix1_pos', 'BPix1+BPix2+FPix1_neg', 
                                             'BPix1+FPix1_pos+FPix2_pos', 'BPix1+FPix1_neg+FPix2_neg', 
                                             'FPix1_pos+FPix2_pos+FPix3_pos', 'FPix1_neg+FPix2_neg+FPix3_neg', 
                                             'BPix2+FPix1_pos+FPix2_pos', 'BPix2+FPix1_neg+FPix2_neg', 
                                             'BPix1+FPix1_pos+FPix3_pos', 'BPix1+FPix1_neg+FPix3_neg', 
                                             'FPix2_pos+FPix3_pos+TEC1_pos', 'FPix2_neg+FPix3_neg+TEC1_neg']
    mixedTripletStepSeedLayersA.TEC.clusterChargeCut.refToPSet_ = 'SiStripClusterChargeCutNone'
    mixedTripletStepSeedLayersA.TEC.maxRing = 2

    mixedTripletStepSeedLayersB.layerList = ['BPix1+BPix2+BPix3', 'BPix2+BPix3+BPix4','BPix1+BPix2+BPix4', 'BPix1+BPix3+BPix4']
    del mixedTripletStepSeedLayersB.TIB

    # Seeding
    mixedTripletStepSeedsA.RegionFactoryPSet.RegionPSet.ptMin = 0.7
    mixedTripletStepSeedsA.SeedComparitorPSet.ClusterShapeHitFilterName = 'ClusterShapeHitFilter'
    mixedTripletStepSeedsA.SeedCreatorPSet.magneticField = ''
    mixedTripletStepSeedsA.SeedCreatorPSet.propagator = 'PropagatorWithMaterial'
    mixedTripletStepSeedsA.ClusterCheckPSet.doClusterCheck = False
    mixedTripletStepSeedsA.OrderedHitsFactoryPSet.GeneratorPSet.maxElement = 0

    mixedTripletStepSeedsB.RegionFactoryPSet.RegionPSet.ptMin = 0.5
    mixedTripletStepSeedsB.RegionFactoryPSet.RegionPSet.originHalfLength = 15.0
    mixedTripletStepSeedsB.RegionFactoryPSet.RegionPSet.originRadius = 1.0
    mixedTripletStepSeedsB.SeedComparitorPSet.ClusterShapeHitFilterName = 'ClusterShapeHitFilter'
    mixedTripletStepSeedsB.SeedCreatorPSet.magneticField = ''
    mixedTripletStepSeedsB.SeedCreatorPSet.propagator = 'PropagatorWithMaterial'
    mixedTripletStepSeedsB.ClusterCheckPSet.doClusterCheck = False
    mixedTripletStepSeedsB.OrderedHitsFactoryPSet.GeneratorPSet.maxElement = 0

    # Building quality cuts
    mixedTripletStepTrajectoryFilter.maxLostHits = 0
    mixedTripletStepTrajectoryFilter.constantValueForLostHitsFractionFilter = TrackingTools.TrajectoryFiltering.TrajectoryFilter_cff.CkfBaseTrajectoryFilter_block.constantValueForLostHitsFractionFilter.value()

    import RecoTracker.MeasurementDet.Chi2ChargeMeasurementEstimator_cfi
    mixedTripletStepChi2Est.clusterChargeCut = RecoTracker.MeasurementDet.Chi2ChargeMeasurementEstimator_cfi.Chi2ChargeMeasurementEstimator.clusterChargeCut.value()
    mixedTripletStepTrajectoryCleanerBySharedHits.fractionShared = 0.095

    # Fitting
    mixedTripletStepTracks.TTRHBuilder = 'WithTrackAngle'

    # Remove modules not used in Phase1PU70
    global MixedTripletStep
    MixedTripletStep.remove(chargeCut2069Clusters)
    MixedTripletStep.remove(mixedTripletStepClassifier1)
    MixedTripletStep.remove(mixedTripletStepClassifier2)
    MixedTripletStep.remove(mixedTripletStep)

    # Then add the old-style cut-based track selector back
    import RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi
    process.mixedTripletStepSelector = RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.multiTrackSelector.clone(
        src='mixedTripletStepTracks',
        trackSelectors= cms.VPSet(
            RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.looseMTS.clone(
                name = 'mixedTripletStepVtxLoose',
                chi2n_par = 0.9,
                res_par = ( 0.003, 0.001 ),
                minNumberLayers = 3,
                maxNumberLostLayers = 1,
                minNumber3DLayers = 2,
                d0_par1 = ( 1.2, 3.0 ),
                dz_par1 = ( 1.2, 3.0 ),
                d0_par2 = ( 1.3, 3.0 ),
                dz_par2 = ( 1.3, 3.0 )
                ),
            RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.looseMTS.clone(
                name = 'mixedTripletStepTrkLoose',
                chi2n_par = 0.5,
                res_par = ( 0.003, 0.001 ),
                minNumberLayers = 4,
                maxNumberLostLayers = 1,
                minNumber3DLayers = 3,
                d0_par1 = ( 1.1, 4.0 ),
                dz_par1 = ( 1.1, 4.0 ),
                d0_par2 = ( 1.1, 4.0 ),
                dz_par2 = ( 1.1, 4.0 )
                ),
            RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.tightMTS.clone(
                name = 'mixedTripletStepVtxTight',
                preFilterName = 'mixedTripletStepVtxLoose',
                chi2n_par = 0.6,
                res_par = ( 0.003, 0.001 ),
                minNumberLayers = 3,
                maxNumberLostLayers = 1,
                minNumber3DLayers = 3,
                d0_par1 = ( 1.1, 3.0 ),
                dz_par1 = ( 1.1, 3.0 ),
                d0_par2 = ( 1.2, 3.0 ),
                dz_par2 = ( 1.2, 3.0 )
                ),
            RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.tightMTS.clone(
                name = 'mixedTripletStepTrkTight',
                preFilterName = 'mixedTripletStepTrkLoose',
                chi2n_par = 0.35,
                res_par = ( 0.003, 0.001 ),
                minNumberLayers = 5,
                maxNumberLostLayers = 1,
                minNumber3DLayers = 4,
                d0_par1 = ( 0.9, 4.0 ),
                dz_par1 = ( 0.9, 4.0 ),
                d0_par2 = ( 0.9, 4.0 ),
                dz_par2 = ( 0.9, 4.0 )
                ),
            RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.highpurityMTS.clone(
                name = 'mixedTripletStepVtx',
                preFilterName = 'mixedTripletStepVtxTight',
                chi2n_par = 0.4,
                res_par = ( 0.003, 0.001 ),
                minNumberLayers = 3,
                maxNumberLostLayers = 1,
                minNumber3DLayers = 3,
                max_minMissHitOutOrIn = 1,
                d0_par1 = ( 0.9, 3.0 ),
                dz_par1 = ( 0.9, 3.0 ),
                d0_par2 = ( 1.0, 3.0 ),
                dz_par2 = ( 1.0, 3.0 )
                ),
            RecoTracker.FinalTrackSelectors.multiTrackSelector_cfi.highpurityMTS.clone(
                name = 'mixedTripletStepTrk',
                preFilterName = 'mixedTripletStepTrkTight',
                chi2n_par = 0.2,
                res_par = ( 0.003, 0.001 ),
                minNumberLayers = 5,
                maxNumberLostLayers = 0,
                minNumber3DLayers = 4,
                max_minMissHitOutOrIn = 1,
                d0_par1 = ( 0.7, 4.0 ),
                dz_par1 = ( 0.7, 4.0 ),
                d0_par2 = ( 0.7, 4.0 ),
                dz_par2 = ( 0.7, 4.0 )
                )
            ) #end of vpset
        ) #end of clone

    import RecoTracker.FinalTrackSelectors.trackListMerger_cfi
    process.mixedTripletStep = RecoTracker.FinalTrackSelectors.trackListMerger_cfi.trackListMerger.clone(
        TrackProducers = cms.VInputTag(cms.InputTag('mixedTripletStepTracks'),
                                       cms.InputTag('mixedTripletStepTracks')),
        hasSelector=cms.vint32(1,1),
        shareFrac=cms.double(0.095),
        indivShareFrac=cms.vdouble(0.095,0.095),
        selectedTrackQuals = cms.VInputTag(cms.InputTag("mixedTripletStepSelector","mixedTripletStepVtx"),
                                           cms.InputTag("mixedTripletStepSelector","mixedTripletStepTrk")),
        setsToMerge = cms.VPSet( cms.PSet( tLists=cms.vint32(0,1), pQual=cms.bool(True) )),
        writeOnlyTrkQuals=cms.bool(True)
    )

    MixedTripletStep += (process.mixedTripletStepSelector +
                         process.mixedTripletStep)

modifyRecoTrackerIterativeTrackingMixedTripletStepPhase1Pixel_ = eras.phase1Pixel.makeProcessModifier(_modifyForPhase1)
