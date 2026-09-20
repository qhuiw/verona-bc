#include "../codegen.h"

namespace vbcc
{
  namespace llvm_backend
  {
    std::optional<LoweredType> LLVMCodegen::lower_union_type(const Node& type)
    {
      std::optional<LoweredType> representation;

      for (const auto& member : *type)
      {
        auto lowered = lower_type(member);

        if (!lowered)
          return {};

        if (representation && (*representation != *lowered))
        {
          fail(type, "union members do not share one LLVM representation");
          return {};
        }

        representation = *lowered;
      }

      if (!representation)
      {
        fail(type, "empty union has no LLVM runtime representation");
        return {};
      }

      return representation;
    }
  }
}
