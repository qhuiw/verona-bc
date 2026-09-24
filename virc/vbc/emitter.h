#pragma once

#include "../model/compilation.h"

namespace virc::vbc_backend
{
  void emit(
    const Compilation& compilation,
    const std::filesystem::path& output,
    bool strip);
}

namespace virc::vbc
{
  void emit(
    const Compilation& compilation,
    const std::filesystem::path& output,
    bool strip);
}

namespace virc
{
  using Bytecode = Compilation;

  // Temporary adapter for the legacy optional backend.
  bool gen_llvm(
    const Compilation& compilation, const std::filesystem::path& output);
}

namespace vbcc
{
  using namespace virc;
}