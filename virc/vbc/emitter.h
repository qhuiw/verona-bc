#pragma once

#include "../model/compilation.h"

namespace virc::vbc_backend
{
  void emit(
    const Compilation& compilation,
    const std::filesystem::path& output,
    bool strip);
}

namespace virc
{
  using Bytecode = Compilation;
}

namespace vbcc
{
  using namespace virc;
}