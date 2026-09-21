#include "object.h"
#include "vrt.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vrt/error.h>
#include <vrt/frame.h>
#include <vrt/object.h>
#include <vrt/program.h>
#include <vrt/thread.h>

namespace
{
  constexpr uintptr_t singleton_class_id = 0x401;

  const vrt::Function root_function{1, "root", nullptr};
  const vrt::Function child_function{2, "child", nullptr};

  alignas(vrt::Object) std::byte
    singleton_storage[vrt::Object::singleton_storage_size()]{};

  const vrt::Class singleton_class{
    singleton_class_id,
    "Singleton",
    0,
    1,
    0,
    nullptr,
    0,
    nullptr,
    singleton_storage + vrt::Object::singleton_payload_offset()};

  const vrt::TypeInfo types[] = {
    {singleton_class_id, vrt::ValueType::object, sizeof(void*), 0}};
  const vrt::Singleton singletons[] = {{singleton_storage, &singleton_class}};
  const vrt::Program program{1, types, 1, singletons};

  void complete_normally(void* context)
  {
    (*static_cast<uint32_t*>(context))++;
  }

  void raise_array_index(void*)
  {
    vrt_error_raise(VRT_ERROR_BAD_ARRAY_INDEX);
  }

  void raise_after_nested_invoke(void*)
  {
    vrt_error_info nested_error{};
    if (
      vrt_try_invoke(raise_array_index, nullptr, &nested_error) ||
      (nested_error.code != VRT_ERROR_BAD_ARRAY_INDEX))
      vrt_error_raise(VRT_ERROR_BAD_STORE_TARGET);

    vrt_error_raise(VRT_ERROR_BAD_STORE);
  }

  void raise_from_invalid_target(void*)
  {
    vrt_frame_enter(&root_function);
    vrt_frame_enter(&child_function);
    vrt_frame_set_raise_target(UINT64_MAX);
    vrt_frame_raise(VRT_VALUE_TYPE_OBJECT, 1);
  }

  void allocate_region_singleton(void*)
  {
    (void)vrt_object_region(vrt::RegionType::rc, &singleton_class, 0, nullptr);
  }

  void allocate_heap_from_singleton(void*)
  {
    (void)vrt_object_heap(
      singleton_class.singleton, &singleton_class, 0, nullptr);
  }
}

int main()
{
  vrt_runtime_init();
  vrt_program_init(&program);
  vrt::init_thread();

  vrt_error_info error{};
  uint32_t calls = 0;
  if (!vrt_try_invoke(complete_normally, &calls, &error))
    return 1;
  if (
    (calls != 1) || (error.code != VRT_ERROR_NONE) || (error.func != nullptr) ||
    (error.site != 0))
    return 2;

  if (vrt_try_invoke(raise_array_index, nullptr, &error))
    return 3;
  if (
    (error.code != VRT_ERROR_BAD_ARRAY_INDEX) || (error.func != nullptr) ||
    (error.site != 0) ||
    (std::strcmp(vrt_error_message(error.code), "bad array index") != 0))
    return 4;

  if (vrt_try_invoke(raise_after_nested_invoke, nullptr, &error))
    return 5;
  if (
    (error.code != VRT_ERROR_BAD_STORE) || (error.func != nullptr) ||
    (error.site != 0))
    return 6;

  if (vrt_try_invoke(raise_from_invalid_target, nullptr, &error))
    return 7;
  if (
    (error.code != VRT_ERROR_BAD_RAISE_TARGET) ||
    (error.func != &child_function) || (error.site != 0) ||
    (vrt_thread_current_frame() != nullptr))
    return 8;

  if (vrt_try_invoke(allocate_region_singleton, nullptr, &error))
    return 9;
  if (error.code != VRT_ERROR_BAD_REGION_ENTRY_POINT)
    return 10;

  if (vrt_try_invoke(allocate_heap_from_singleton, nullptr, &error))
    return 11;
  if (error.code != VRT_ERROR_BAD_ALLOC_TARGET)
    return 12;

  if (!vrt_try_invoke(complete_normally, &calls, &error))
    return 13;
  if (
    (calls != 2) || (error.code != VRT_ERROR_NONE) || (error.func != nullptr) ||
    (error.site != 0))
    return 14;

  vrt::deinit_thread();
  return 0;
}
