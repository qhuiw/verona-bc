#pragma once

#include "location.h"

#include <vrt/value.h>

namespace vrt
{
  struct Header;
  struct Region;

  /** Non-owning, type-erased view over one runtime value. */
  class Value final
  {
  private:
    ValueType value_type;
    const void* data_address;

  public:
    Value(ValueType value_type, const void* data_address)
    : value_type(value_type), data_address(data_address)
    {}

    ValueType type() const
    {
      return value_type;
    }

    Header* header() const;
    Location location() const;
    Region* region() const;

    void reg_inc() const;
    void reg_dec() const;
    void field_inc() const;
    void field_dec() const;
    void escape() const;
  };
}
