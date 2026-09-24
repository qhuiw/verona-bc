#pragma once

#include "passes/patterns.h"
#include "support/diagnostics.h"
#include "support/literals.h"
#include "vbc/emitter.h"

namespace virc
{
  PassDef memo();
  PassDef assign_ids(std::shared_ptr<Bytecode> state);
  PassDef validate_ids(std::shared_ptr<Bytecode> state);
  PassDef liveness(std::shared_ptr<Bytecode> state);
  PassDef typecheck(std::shared_ptr<Bytecode> state);
  PassDef optimize(std::shared_ptr<Bytecode> state);
}
