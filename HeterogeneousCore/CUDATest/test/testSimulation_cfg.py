import FWCore.ParameterSet.Config as cms
import FWCore.ParameterSet.VarParsing as VarParsing

options = VarParsing.VarParsing()
options.register('numberOfThreads',
                 1,
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.int,
                 "Number of threads.")
options.register('numberOfStreams',
                 0,
                 VarParsing.VarParsing.multiplicity.singleton,
                 VarParsing.VarParsing.varType.int,
                 "Number of streams.")
options.parseArguments()

process = cms.Process("Test")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("HeterogeneousCore.CUDAServices.CUDAService_cfi")

process.source = cms.Source("EmptySource")

interval = 1000*options.numberOfThreads
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(12*interval))
process.MessageLogger.cerr.FwkReport.reportEvery = interval

process.options = cms.untracked.PSet(
    numberOfThreads = cms.untracked.uint32(options.numberOfThreads),
    numberOfStreams = cms.untracked.uint32(options.numberOfStreams)
)

#process.overhead = cms.EDProducer("TestCUDAProducerOverhead")
#process.overhead = cms.EDProducer("TestCUDAProducerOverheadEW")
#process.overhead = cms.EDProducer("TestCUDAProducerOverheadEW", lockMutex=cms.bool(True))
#process.p_overhead = cms.Path(process.overhead)

from HeterogeneousCore.CUDATest.testCUDAProducerSimEW_cfi import testCUDAProducerSimEW
#from HeterogeneousCore.CUDATest.testCUDAProducerSimEWSerialTaskQueue_cfi import testCUDAProducerSimEWSerialTaskQueue as testCUDAProducerSimEW
process.transfer = testCUDAProducerSimEW.clone()
process.transfer.numberOfElements = 1 # 4 B
#process.transfer.numberOfElements = 8192 # 32 kB
#process.transfer.numberOfElements = 262144 # 1 MB
#process.transfer.useCachingAllocator = False
process.transfer.transferDevice = True
process.transfer.kernels = 1
process.transfer.kernelLoops = 1
#process.transfer.kernelLoops = 1024
#process.transfer.kernelLoops = 8192
process.transfer.transferHost = True
process.p_transfer = cms.Path(process.transfer)
