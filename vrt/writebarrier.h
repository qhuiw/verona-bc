#pragma once

#include "location.h"

#include <vrt/object.h>

namespace vrt
{
  struct Object;
}

namespace vrt::writebarrier
{
  /** Consume one field-layout argument into a newly allocated field. */
  void init(
    Location store_location,
    void* target,
    const Field& field,
    const void* source);

  /** Copy one encoded value over an existing field or array element. */
  void copy(
    Location store_location,
    void* target,
    const Field& field,
    const void* source);

  /** Drop a field while finalizing its containing object. */
  void drop(Location store_location, const Field& field, void* source);
}
