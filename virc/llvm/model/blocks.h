#pragma once

#include "../../../virc/lang.h"

#include <string>
#include <unordered_map>

namespace llvm
{
  class BasicBlock;
  class Function;
}

namespace vbcc
{
  using namespace virc;

  namespace llvm_backend
  {
    class LLVMCodegen;

    class BasicBlockState
    {
    private:
      LLVMCodegen& codegen;
      std::unordered_map<std::string, llvm::BasicBlock*> blocks;

    public:
      explicit BasicBlockState(LLVMCodegen& codegen);

      void reset();
      bool declare(const Node& label_id, llvm::Function* function);
      llvm::BasicBlock* get(const Node& label_id) const;
      llvm::BasicBlock* find(const Node& branch, const Node& label_id);
    };
  }
}