import FWCore.ParameterSet.Config as cms

from SimTracker.TrackerHitAssociation.clusterTpAssociationProducer_cfi import tpClusterProducer
from Validation.RecoTrack.TrackValidation_cff import trackAssociatorByHitsRecoDenom

selectedOfflinePrimaryVertices = cms.EDFilter("VertexSelector",
                                               src = cms.InputTag('offlinePrimaryVertices'),
                                               cut = cms.string("isValid & ndof > 4 & tracksSize > 0 & abs(z) <= 24 & abs(position.Rho) <= 2."),
                                               filter = cms.bool(False)
)

selectedOfflinePrimaryVerticesWithBS = selectedOfflinePrimaryVertices.clone()
selectedOfflinePrimaryVerticesWithBS.src = cms.InputTag('offlinePrimaryVerticesWithBS')

#selectedPixelVertices = selectedOfflinePrimaryVertices.clone()
#selectedPixelVertices.src = cms.InputTag('pixelVertices')

vertexAnalysis = cms.EDAnalyzer("PrimaryVertexAnalyzer4PUSlimmed",
                                use_only_charged_tracks = cms.untracked.bool(True),
                                use_TP_associator = cms.untracked.bool(True),
                                verbose = cms.untracked.bool(False),
                                sigma_z_match = cms.untracked.double(3.0),
                                abs_z_match = cms.untracked.double(0.1),
                                root_folder = cms.untracked.string("Vertexing/PrimaryVertexV"),
                                recoTrackProducer = cms.untracked.InputTag("generalTracks"),
                                trackingParticleCollection = cms.untracked.InputTag("mix", "MergedTrackTruth"),
                                trackingVertexCollection = cms.untracked.InputTag("mix", "MergedTrackTruth"),
                                vertexRecoCollections = cms.VInputTag("offlinePrimaryVertices",
                                                                      "offlinePrimaryVerticesWithBS",
#                                                                      "pixelVertices",
                                                                      "selectedOfflinePrimaryVertices",
                                                                      "selectedOfflinePrimaryVerticesWithBS",
#                                                                      "selectedPixelVertices"
                                ),
)

vertexAnalysisSequence = cms.Sequence(tpClusterProducer
                                      * trackAssociatorByHitsRecoDenom
                                      * cms.ignore(selectedOfflinePrimaryVertices)
                                      * cms.ignore(selectedOfflinePrimaryVerticesWithBS)
#                                      * cms.ignore(selectedPixelVertices)
                                      * vertexAnalysis
)
