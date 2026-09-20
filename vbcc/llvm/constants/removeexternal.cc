#include "../codegen.h"

namespace vbcc
{
  namespace llvm_backend
  {
    bool LLVMCodegen::emit_remove_external(const Node& statement)
    {
      // TODO: Requires VRT scheduler implementation.
      fail(statement, "removeexternal requires VRT scheduler implementation");
      return false;
    }
  }
}
