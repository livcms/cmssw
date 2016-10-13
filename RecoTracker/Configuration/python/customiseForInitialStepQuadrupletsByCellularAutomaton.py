import FWCore.ParameterSet.Config as cms

def customiseForInitialStepQuadrupletsByCellularAutomaton(process, regionPtMin, thetacut, phicut, hardptcut, chi2lowpt, chi2highpt):
#    for module in [process.initialStepSeeds, process.initialStepSeedsPreSplitting]:
    for module in [process.initialStepSeeds]:
        if hasattr(module, "OrderedHitsFactoryPSet"):
            pset = getattr(module, "OrderedHitsFactoryPSet")
            if (hasattr(pset, "ComponentName") and (pset.ComponentName == "CombinedHitQuadrupletGenerator")):
                # Adjust seeding layers
                seedingLayersName = module.OrderedHitsFactoryPSet.SeedingLayers.getModuleLabel()
                # Configure seed generator / pixel track producer
                quadruplets = module.OrderedHitsFactoryPSet.clone()
                from RecoPixelVertexing.PixelTriplets.CAHitQuadrupletGenerator_cfi import CAHitQuadrupletGenerator as _CAHitQuadrupletGenerator
    
                module.OrderedHitsFactoryPSet  = _CAHitQuadrupletGenerator.clone(
                    ComponentName = cms.string("CAHitQuadrupletGenerator"),
                    extraHitRPhitolerance = quadruplets.GeneratorPSet.extraHitRPhitolerance,
                    maxChi2 = dict(
                        pt1    = cms.double(0.8), pt2    = cms.double(2),
                        value1 = cms.double(chi2lowpt), value2 = cms.double(chi2highpt),
                        enabled = cms.bool(True),
                    ),
                    useBendingCorrection = True,
                    fitFastCircle = True,
                    fitFastCircleChi2Cut = True,
                    SeedingLayers = cms.InputTag(seedingLayersName),
                    CAThetaCut = cms.double(thetacut),
                    CAPhiCut = cms.double(phicut),
                    CAHardPtCut = cms.double(hardptcut),
                )
                
                
                module.RegionFactoryPSet.RegionPSet.ptMin = cms.double(regionPtMin)
    
                if hasattr(quadruplets.GeneratorPSet, "SeedComparitorPSet"):
                    module.SeedComparitorPSet = quadruplets.GeneratorPSet.SeedComparitorPSet
    return process
