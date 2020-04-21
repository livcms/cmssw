#ifndef gen_HydjetGeneratorFilter_h
#define gen_HydjetGeneratorFilter_h

#include "GeneratorInterface/HydjetInterface/interface/HydjetHadronizer.h"
#include "GeneratorInterface/Core/interface/GeneratorFilter.h"
#include "GeneratorInterface/ExternalDecays/interface/ExternalDecayDriver.h"

namespace edm {
  template<>
  GeneratorFilter<gen::HydjetHadronizer, gen::ExternalDecayDriver>::GeneratorFilter(ParameterSet const& ps)
    : hadronizer_(ps, consumesCollector()) {
    init(ps);
  }
}
namespace gen {
  typedef edm::GeneratorFilter<gen::HydjetHadronizer, gen::ExternalDecayDriver> HydjetGeneratorFilter;
}

#endif
