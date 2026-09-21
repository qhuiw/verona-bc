#pragma once

#include "location.h"

#include <cstddef>
#include <cstdint>
#include <vrt/value.h>

namespace vrt
{
  struct Region;

  /** State shared by every region-managed VRT allocation. */
  struct Header
  {
  private:
    Location loc;

  public:
    static constexpr uint64_t magic_value = 0x5652544845414445ULL;

    uint64_t magic = magic_value;
    uintptr_t type_id;
    uintptr_t reference_count = 1;
    std::byte* allocation = nullptr;
    bool finalizing = false;

    Header(Location location, uintptr_t type_id)
    : loc(location), type_id(type_id)
    {}

    Location location() const
    {
      return loc;
    }

    void set_location(Location location)
    {
      loc = location;
    }

    Region* region() const
    {
      if (!loc.is_region())
        return nullptr;

      return loc.to_region();
    }

    uintptr_t get_type_id() const
    {
      return type_id;
    }

    static Header* from_data(ValueType value_type, const void* data_address);

    void* data();
    const void* data() const;

    ValueType value_type() const;

    void finalize();
    void destroy_storage();
    void reg_inc();
    void reg_dec();
    void field_inc();
    void field_dec();
  };

  bool is_header_type(ValueType value_type);
}
