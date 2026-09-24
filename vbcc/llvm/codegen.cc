#include "codegen.h"

namespace vbcc
{
  namespace llvm_backend
  {
    LLVMCodegen::LLVMCodegen(const Bytecode& state)
    : state(state),
      module("verona", context),
      builder(context),
      blocks(*this),
      locals(*this)
    {}
  }
}

namespace virc
{
  bool gen_llvm(
    const Compilation& compilation, const std::filesystem::path& output)
  {
    // Destructor automatically restores the previous WFContext when this
    // function returns.
    trieste::WFContext wf_context(wfIR);
    vbcc::llvm_backend::LLVMCodegen codegen(compilation);
    return codegen.emit(output.empty() ? "out.ll" : output);
  }
}
