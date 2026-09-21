#include "value.h"

#include "error.h"
#include "failure.h"
#include "header.h"
#include "thread_context.h"

namespace vrt
{
  namespace
  {
    bool is_unmanaged(ValueType value_type)
    {
      switch (value_type)
      {
        case ValueType::none:
        case ValueType::scalar:
        case ValueType::raw_pointer:
          return true;

        default:
          return false;
      }
    }
  }

  Header* Value::header() const
  {
    return Header::from_data(value_type, data_address);
  }

  Location Value::location() const
  {
    if (is_unmanaged(value_type))
      return Location::immortal();

    return header()->location();
  }

  Region* Value::region() const
  {
    if ((value_type == ValueType::object) || (value_type == ValueType::array))
    {
      auto* result = header()->region();
      if (result != nullptr)
        return result;
    }

    raise_error(Error::bad_alloc_target);
  }

  void Value::reg_inc() const
  {
    if (!is_unmanaged(value_type))
      header()->reg_inc();
  }

  void Value::reg_dec() const
  {
    if (!is_unmanaged(value_type))
      header()->reg_dec();
  }

  void Value::field_inc() const
  {
    if (!is_unmanaged(value_type))
      header()->field_inc();
  }

  void Value::field_dec() const
  {
    if (!is_unmanaged(value_type))
      header()->field_dec();
  }

  void Value::escape() const
  {
    auto* value_header = header();
    if (value_header->location().is_immortal())
      return;

    auto* context = ThreadContext::try_get();
    internal_check(context != nullptr, Failure::invalid_header_state);

    context->escape(value_header);
  }
}
