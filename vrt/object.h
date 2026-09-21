#pragma once

#include "../include/vrt/object.h"
#include "header.h"

#include <cstddef>
#include <cstdint>

namespace vrt
{
  struct Region;

  /** Runtime object stored immediately before its exposed fields. */
  struct Object final : Header
  {
    const Class* cls = nullptr;

  private:
    Object(
      Region* region, const Class* cls, std::byte* allocation, bool immortal);

  public:
    static constexpr size_t singleton_data_offset()
    {
      return sizeof(Object);
    }

    static constexpr size_t singleton_storage_size()
    {
      return singleton_data_offset() + 1;
    }

    static size_t size_of(const Class* cls);
    static Object* create(
      std::byte* allocation,
      const Class* cls,
      Region* region,
      bool immortal = false);

    Object& init(uintptr_t argc, const void* packed_args);
    void finalize();
    void destroy_storage();

    void* fields()
    {
      return this + 1;
    }

    const void* fields() const
    {
      return this + 1;
    }
  };

  void init_singleton(void* storage, const Class* cls);
}
