import FWCore.ParameterSet.Config as cms

process = cms.Process("Test")
process.load("FWCore.MessageService.MessageLogger_cfi")

process.source = cms.Source("EmptySource")

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(10) )

process.options = cms.untracked.PSet(
#    numberOfThreads = cms.untracked.uint32(4),
    numberOfStreams = cms.untracked.uint32(0)
)


#process.Tracer = cms.Service("Tracer")
process.CUDAService = cms.Service("CUDAService")
process.prod1 = cms.EDProducer('TestHeterogeneousEDProducerGPU')
process.prod2 = cms.EDProducer('TestHeterogeneousEDProducerGPU',
    src = cms.InputTag("prod1"),
)
process.prod3 = cms.EDProducer('TestHeterogeneousEDProducerGPU',
    src = cms.InputTag("prod1"),
)
process.prod4 = cms.EDProducer('TestHeterogeneousEDProducerGPU')
process.ana = cms.EDAnalyzer("TestHeterogeneousEDProducerAnalyzer",
    src = cms.VInputTag("prod2", "prod3", "prod4")
)

process.t = cms.Task(process.prod1, process.prod2, process.prod3, process.prod4)
process.p = cms.Path(process.ana)
process.p.associate(process.t)
