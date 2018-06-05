#ifndef EventFilter_SiPixelRawToDigi_plugins_SiPixelFedCablingMapGPU_h
#define EventFilter_SiPixelRawToDigi_plugins_SiPixelFedCablingMapGPU_h

// C++ includes
#include <set>

#include "cuda/api_wrappers.h"

class SiPixelFedCablingMap;
class SiPixelQuality;
class TrackerGeometry;

class SiPixelGainCalibrationForHLT;
class SiPixelGainForHLTonGPU;
struct SiPixelGainForHLTonGPU_DecodingStructure;

// TODO: are these still needed in this header?
// Maximum fed for phase1 is 150 but not all of them are filled
// Update the number FED based on maximum fed found in the cabling map
const unsigned int MAX_FED  = 150;
const unsigned int MAX_LINK =  48;  // maximum links/channels for Phase 1
const unsigned int MAX_ROC  =   8;
const unsigned int MAX_SIZE = MAX_FED * MAX_LINK * MAX_ROC;
const unsigned int MAX_SIZE_BYTE_INT  = MAX_SIZE * sizeof(unsigned int);
const unsigned int MAX_SIZE_BYTE_BOOL = MAX_SIZE * sizeof(unsigned char);

void processGainCalibration(SiPixelGainCalibrationForHLT const& gains,
                            TrackerGeometry const& trackerGeom,
                            SiPixelGainForHLTonGPU *gainsOnHost,
                            SiPixelGainForHLTonGPU * & gainsOnGPU,
                            SiPixelGainForHLTonGPU_DecodingStructure * & gainDataOnGPU,
                            cuda::stream_t<>& stream);

#endif // EventFilter_SiPixelRawToDigi_plugins_SiPixelFedCablingMapGPU_h
