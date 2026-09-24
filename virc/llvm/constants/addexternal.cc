#include "../codegen.h"

namespace vbcc
{
  namespace llvm_backend
  {
    bool LLVMCodegen::emit_add_external(const Node& statement)
    {
      // TODO: Requires VRT scheduler implementation.
      fail(statement, "addexternal requires VRT scheduler implementation");
      return false;
    }
  }
}
