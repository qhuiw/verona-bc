#pragma once

#include <cstddef>
#include <cstdint>

namespace virc
{
  inline constexpr auto MainFunctionIndex = size_t(0);
  inline constexpr auto FinalizerMethodIndex = size_t(0);
  inline constexpr auto CallbackMethodIndex = size_t(1);
  inline constexpr auto DynamicTypeId = uint32_t(-1);
}