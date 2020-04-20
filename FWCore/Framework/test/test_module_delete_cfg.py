import FWCore.ParameterSet.Config as cms

process = cms.Process("TESTMODULEDELETE")

process.maxEvents.input = 8
process.source = cms.Source("EmptySource")

#process.Tracer = cms.Service("Tracer")

# Cases to test
# - event/lumi/run product consumed, module kept
# - event/lumi/run product not consumed, module deleted
# - event product not consumed but module in Path, module kept
# - event/lumi/run product with the same module label but different instance name, module deleted
# - event/lumi/run product with the same module label and instance name but with skipCurrentProcess, module deleted
# - DAG of event/lumi/run producers that are not consumed by an always-running module, whole DAG deleted
# - DAG of event(/lumi/run) producers that are partly consumed (kept) and partly non-consumed (delete)

intEventProducer = cms.EDProducer("IntProducer", ivalue = cms.int32(1))
intNonEventProducer = cms.EDProducer("NonEventIntProducer", ivalue = cms.int32(1))
intEventProducerMustRun = cms.EDProducer("edmtest::MustRunIntProducer", ivalue = cms.int32(1))
intEventConsumer = cms.EDAnalyzer("IntTestAnalyzer",
    moduleLabel = cms.untracked.string("producerEventConsumed"),
    valueMustMatch = cms.untracked.int32(1)
)
def nonEventConsumer(transition, sourcePattern, expected):
    transCap = transition[0].upper() + transition[1:]
    runOrLumiBlock = transCap
    if "Lumi" in runOrLumiBlock:
        runOrLumiBlock = runOrLumiBlock+"nosityBlock"
    ret = intNonEventProducer.clone()
    setattr(ret, "consumes%s"%runOrLumiBlock, cms.InputTag(sourcePattern%transCap, transition))
    setattr(ret, "expect%s"%runOrLumiBlock, cms.untracked.int32(expected))
    return ret

process.producerEventConsumed = intEventProducer.clone(ivalue = 1)
process.producerBeginLumiConsumed = intNonEventProducer.clone(ivalue = 2)
process.producerEndLumiConsumed = intNonEventProducer.clone(ivalue = 3)
process.producerBeginRunConsumed = intNonEventProducer.clone(ivalue = 4)
process.producerEndRunConsumed = intNonEventProducer.clone(ivalue = 5)

process.producerEventNotConsumedInPath = intEventProducerMustRun.clone()

process.consumerEvent = intEventConsumer.clone(moduleLabel = "producerEventConsumed", valueMustMatch = 1)
process.consumerBeginLumi = nonEventConsumer("beginLumi", "producer%sConsumed", 2)
process.consumerEndLumi = nonEventConsumer("endLumi", "producer%sConsumed", 3)
process.consumerBeginRun = nonEventConsumer("beginRun", "producer%sConsumed", 4)
process.consumerEndRun = nonEventConsumer("endRun", "producer%sConsumed", 5)

process.producerEventNotConsumed = cms.EDProducer("edmtest::TestModuleDeleteProducer")
process.producerBeginLumiNotConsumed = cms.EDProducer("edmtest::TestModuleDeleteInLumiProducer")
process.producerBeginRunNotConsumed = cms.EDProducer("edmtest::TestModuleDeleteInRunProducer")

process.producerEventNotConsumedChain1 = cms.EDProducer("edmtest::TestModuleDeleteProducer")
process.producerEventNotConsumedChain2 = cms.EDProducer("edmtest::TestModuleDeleteProducer",
    srcEvent = cms.untracked.VInputTag("producerEventNotConsumedChain1")
)
process.producerEventNotConsumedChain3 = cms.EDProducer("edmtest::TestModuleDeleteProducer",
    srcEvent = cms.untracked.VInputTag("producerEventNotConsumedChain1")
)
process.producerEventNotConsumedChain4 = cms.EDProducer("edmtest::TestModuleDeleteProducer",
    srcEvent = cms.untracked.VInputTag("producerEventNotConsumedChain2", "producerEventNotConsumedChain3")
)
process.producerEventNotConsumedChain5 = cms.EDProducer("edmtest::TestModuleDeleteInLumiProducer",
    srcEvent = cms.untracked.VInputTag("producerEventNotConsumedChain1")
)
process.producerEventNotConsumedChain6 = cms.EDProducer("edmtest::TestModuleDeleteInRunProducer",
    srcBeginLumi = cms.untracked.VInputTag("producerEventNotConsumedChain5")
)
process.producerEventNotConsumedChain7 = cms.EDProducer("edmtest::TestModuleDeleteInLumiProducer",
    srcBeginRun = cms.untracked.VInputTag("producerEventNotConsumedChain6")
)
process.producerEventNotConsumedChain8 = cms.EDProducer("edmtest::TestModuleDeleteProducer",
    srcBeginLumi = cms.untracked.VInputTag("producerEventNotConsumedChain7")
)

process.producerEventPartiallyConsumedChain1 = intEventProducerMustRun.clone()
process.producerEventPartiallyConsumedChain2 = cms.EDProducer("AddIntsProducer", labels = cms.vstring("producerEventPartiallyConsumedChain1"))
process.producerEventPartiallyConsumedChain3 = cms.EDProducer("edmtest::TestModuleDeleteInLumiProducer",
    srcEvent = cms.untracked.VInputTag("producerEventPartiallyConsumedChain1")
)
process.producerEventPartiallyConsumedChain4 = cms.EDProducer("edmtest::TestModuleDeleteInLumiProducer",
    srcEvent = cms.untracked.VInputTag("producerEventPartiallyConsumedChain2", "producerEventPartiallyConsumedChain4")
)

process.consumerNotExist = cms.EDAnalyzer("edmtest::GenericIntsAnalyzer",
    inputShouldBeMissing = cms.untracked.bool(True),
    srcBeginRun = cms.untracked.VInputTag(
        "producerBeginRunNotConsumed:doesNotExist",
        cms.InputTag("producerBeginRunNotConsumed", "", cms.InputTag.skipCurrentProcess())
    ),
    srcBeginLumi = cms.untracked.VInputTag(
        "producerBeginLumiNotConsumed:doesNotExist",
        cms.InputTag("producerBeginLumiNotConsumed", "", cms.InputTag.skipCurrentProcess())
    ),
    srcEvent = cms.untracked.VInputTag(
        "producerEventNotConsumed:doesNotExist",
        cms.InputTag("producerEventNotConsumed", "", cms.InputTag.skipCurrentProcess())
    ),
)

process.intAnalyzerDelete = cms.EDAnalyzer("edmtest::TestModuleDeleteAnalyzer")

process.t = cms.Task(
    process.producerEventConsumed,
    process.producerBeginLumiConsumed,
    process.producerEndLumiConsumed,
    process.producerBeginRunConsumed,
    process.producerEndRunConsumed,
    #
    process.producerEventNotConsumed,
    process.producerBeginLumiNotConsumed,
    process.producerBeginRunNotConsumed,
    #
    process.producerEventNotConsumedChain1,
    process.producerEventNotConsumedChain2,
    process.producerEventNotConsumedChain3,
    process.producerEventNotConsumedChain4,
    process.producerEventNotConsumedChain5,
    process.producerEventNotConsumedChain6,
    process.producerEventNotConsumedChain7,
    process.producerEventNotConsumedChain8,
    #
    process.producerEventPartiallyConsumedChain1,
    process.producerEventPartiallyConsumedChain3,
    process.producerEventPartiallyConsumedChain4,
)

process.p = cms.Path(
    process.producerEventNotConsumedInPath+
    #
    process.consumerEvent+
    process.consumerBeginLumi+
    process.consumerEndLumi+
    process.consumerBeginRun+
    process.consumerEndRun+
    #
    process.consumerNotExist+
    #
    process.producerEventPartiallyConsumedChain2+
    #
    process.intAnalyzerDelete
    ,
    process.t
)
