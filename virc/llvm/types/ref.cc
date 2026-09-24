#include "../codegen.h"

namespace virc
{
  namespace llvm_backend
  {
    std::optional<LoweredType> lower_ref(llvm::LLVMContext&, const Node&)
    {
      // Lowers Ref(T) to the runtime reference representation.
      return {};
    }
  }
}
