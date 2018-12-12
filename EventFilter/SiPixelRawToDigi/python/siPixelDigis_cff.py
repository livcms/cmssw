import FWCore.ParameterSet.Config as cms

from EventFilter.SiPixelRawToDigi.SiPixelRawToDigi_cfi import siPixelDigis
from EventFilter.SiPixelRawToDigi.siPixelDigisSoAFromCUDA_cfi import siPixelDigisSoAFromCUDA as _siPixelDigisSoAFromCUDA

siPixelDigisTask = cms.Task(siPixelDigis)

siPixelDigisSoA = _siPixelDigisSoAFromCUDA.clone(
    src = "siPixelClustersCUDAPreSplitting"
)
siPixelDigisTaskCUDA = cms.Task(siPixelDigisSoA)

from Configuration.ProcessModifiers.gpu_cff import gpu
_siPixelDigisTask_gpu = siPixelDigisTask.copy()
_siPixelDigisTask_gpu.add(siPixelDigisTaskCUDA)
gpu.toReplaceWith(siPixelDigisTask, _siPixelDigisTask_gpu)
