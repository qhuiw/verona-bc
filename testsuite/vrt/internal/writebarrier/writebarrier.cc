// Coverage: immutable incoming copy/drop ARC accounting, rejection of writes
// to immutable or immortal storage, and conflicting region-parent stores.
// Non-goals: mutable region drag and parenting are covered by the region and
// array fixtures; SCC reclamation is covered by the SCC fixture.

#include "writebarrier.h"

#include "error.h"
#include "frame.h"
#include "object.h"
#include "region.h"
#include "thread.h"
#include "vrt.h"

#include <cstddef>
#include <cstdint>
#include <vrt/error.h>
#include <vrt/program.h>

namespace
{
  constexpr uintptr_t node_class_id = 0x101;

  struct NodeFields
  {
    void* next;
  };

  const vrt::Field node_fields[] = {
    {offsetof(NodeFields, next),
     sizeof(NodeFields::next),
     node_class_id,
     vrt::ValueType::object}};

  const vrt::Class node_class{
    node_class_id,
    "Node",
    sizeof(NodeFields),
    alignof(NodeFields),
    1,
    node_fields,
    0,
    nullptr,
    nullptr};

  const vrt::TypeInfo types[] = {
    {node_class_id, vrt::ValueType::object, sizeof(void*), 0}};
  const vrt::Program program{1, types, 0, nullptr};

  struct RejectedStore
  {
    vrt::Location location;
    void* target = nullptr;
    void* source;
  };

  void rejected_store(void* raw_context)
  {
    auto* context = static_cast<RejectedStore*>(raw_context);
    vrt::writebarrier::copy(
      context->location, &context->target, node_fields[0], &context->source);
  }
}

int main()
{
  vrt_runtime_init();
  vrt_program_init(&program);
  vrt::init_thread();

  const vrt::Function root_function{1, "root", nullptr};
  auto* frame = vrt_frame_enter(&root_function);
  auto* frame_region = frame->region;

  auto* immutable_region = vrt::Region::create(vrt::RegionType::rc);
  auto* immutable = immutable_region->object(&node_class);
  if (!immutable_region->remove(immutable))
    return 1;
  immutable->set_location(vrt::Location::immutable());
  immutable->set_arc(1);
  if (immutable_region->stack_dec())
    return 2;

  auto* holder = frame_region->object(&node_class);
  auto* holder_fields = static_cast<NodeFields*>(holder->fields());
  auto* immutable_data = immutable->data();
  vrt::writebarrier::copy(
    holder->location(), &holder_fields->next, node_fields[0], &immutable_data);
  if ((holder_fields->next != immutable_data) || (immutable->get_arc() != 2))
    return 3;

  vrt::writebarrier::drop(
    holder->location(), node_fields[0], &holder_fields->next);
  if ((holder_fields->next != nullptr) || (immutable->get_arc() != 1))
    return 4;

  holder->reg_dec();
  vrt_frame_leave();

  RejectedStore immutable_store{
    vrt::Location::immutable(), nullptr, immutable_data};
  vrt::ErrorInfo error{};
  if (vrt_try_invoke(rejected_store, &immutable_store, &error))
    return 5;
  if (
    (error.code != vrt::Error::bad_store_target) ||
    (immutable_store.target != nullptr) || (immutable->get_arc() != 1))
    return 6;

  RejectedStore immortal_store{
    vrt::Location::immortal(), nullptr, immutable_data};
  if (vrt_try_invoke(rejected_store, &immortal_store, &error))
    return 7;
  if (
    (error.code != vrt::Error::bad_store_target) ||
    (immortal_store.target != nullptr) || (immutable->get_arc() != 1))
    return 8;

  // Once a child region is owned through one parent, copying its entry point
  // into a different region would violate the single-parent invariant.
  auto* child_region = vrt::Region::create(vrt::RegionType::rc);
  auto* child = child_region->object(&node_class);
  auto* first_parent_region = vrt::Region::create(vrt::RegionType::rc);
  auto* first_parent = first_parent_region->object(&node_class);
  auto* child_data = child->data();
  auto* first_parent_fields = static_cast<NodeFields*>(first_parent->fields());
  vrt::writebarrier::init(
    first_parent->location(),
    &first_parent_fields->next,
    node_fields[0],
    &child_data);

  auto* second_parent_region = vrt::Region::create(vrt::RegionType::rc);
  auto* second_parent = second_parent_region->object(&node_class);
  RejectedStore conflicting_store{
    second_parent->location(),
    static_cast<NodeFields*>(second_parent->fields())->next,
    child_data};
  if (vrt_try_invoke(rejected_store, &conflicting_store, &error))
    return 9;
  if (
    (error.code != vrt::Error::bad_store) ||
    (conflicting_store.target != nullptr) ||
    (child_region->parent != first_parent_region))
    return 10;

  second_parent->reg_dec();
  first_parent->reg_dec();

  immutable->reg_dec();
  vrt::deinit_thread();
  return vrt_thread_current() == nullptr ? 0 : 11;
}
