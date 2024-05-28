#include "CmajorStereoDSPEffect.h"
#include "../Playground.h"

bool CmajorStereoDSPEffect::Processor::updateParameters() {

  bool changed = false;
  auto params = patch->getParameterList();

  // ensureNumParameters (params.size());

  auto proc = static_cast<PlaygroundProcessor *>(mainProc);

  for (size_t i = 0; i < params.size(); ++i)

    changed = proc->applyUpdateToCmajorParams(
        [&](CmajorStereoDSPEffect::Processor::Parameters &cmajorParams) {
          auto parameters = cmajorParams.parameters;
          return parameters[i]->setPatchParam(params[i]) || changed;
        });
  //   changed = proc->updateCmajorPatchParams(i, params[i]) || changed;

  return changed;
}