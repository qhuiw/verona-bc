#include "../codegen.h"

namespace vbcc
{
  namespace llvm_backend
  {
    std::optional<LoweredType> lower_cown(llvm::LLVMContext&, const Node&)
    {
      // Lowers Cown(T) to the runtime cown representation.
      return {};
    }
  }
}
