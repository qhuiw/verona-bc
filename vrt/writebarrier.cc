#include "writebarrier.h"

#include "array.h"
#include "error.h"
#include "failure.h"
#include "frame.h"
#include "header.h"
#include "object.h"
#include "region.h"
#include "region_ext.h"
#include "thread_context.h"
#include "value.h"

#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>

namespace
{
  vrt::Header* load_header(vrt::ValueType value_type, const void* source)
  {
    void* data_address = nullptr;
    std::memcpy(&data_address, source, sizeof(data_address));
    if (data_address == nullptr)
      return nullptr;

    return vrt::Header::from_data(value_type, data_address);
  }

  void store_header(void* target, vrt::Header* header)
  {
    auto* data_address = header->data();
    std::memcpy(target, &data_address, sizeof(data_address));
  }

  void clear_header(void* target)
  {
    void* data_address = nullptr;
    std::memcpy(target, &data_address, sizeof(data_address));
  }

  vrt::Region* frame_region_for_stack(vrt::Location location)
  {
    internal_check(location.is_stack(), vrt::Failure::invalid_write);

    auto* context = vrt::ThreadContext::try_get();
    internal_check(context != nullptr, vrt::Failure::invalid_write);

    auto* frame = context->thread.frame;
    while ((frame != nullptr) && (frame->frame_id != location))
      frame = frame->parent;

    internal_check(frame != nullptr, vrt::Failure::invalid_write);
    return vrt::frame_region(frame);
  }

  vrt::Region* store_region(vrt::Location location)
  {
    if (location.is_region())
      return location.to_region();

    if (location.is_stack())
      return frame_region_for_stack(location);

    return nullptr;
  }

  bool is_frame_storage(vrt::Location location)
  {
    return location.is_stack() ||
      (location.is_region() && location.to_region()->is_frame_local());
  }

  void
  validate_store(vrt::Location location, const void* target, const void* source)
  {
    internal_check(
      (target != nullptr) && (source != nullptr), vrt::Failure::invalid_write);

    if (location.is_immutable() || location.is_immortal())
      vrt::raise_error(vrt::Error::bad_store_target);

    internal_check(
      location.is_stack() || location.is_region(), vrt::Failure::invalid_write);

    if (location.is_region())
    {
      auto* region = location.to_region();
      internal_check(
        !region->destroying && !region->is_finalizing(),
        vrt::Failure::invalid_write);
    }
  }

  void drop_header(vrt::Location store_location, vrt::Header* outgoing)
  {
    if (outgoing == nullptr)
      return;

    const auto outgoing_location = outgoing->location();
    if (outgoing_location.is_immortal() || outgoing_location.is_stack())
      return;

    if (outgoing_location.is_immutable())
    {
      outgoing->field_dec();
      return;
    }

    auto* outgoing_region = outgoing->region();
    internal_check(outgoing_region != nullptr, vrt::Failure::invalid_write);

    if (outgoing_region->is_frame_local())
    {
      outgoing->field_dec();
      return;
    }

    if (is_frame_storage(store_location))
    {
      outgoing_region->stack_dec();
      return;
    }

    if (store_location == outgoing_location)
    {
      outgoing->field_dec();
      return;
    }

    if (outgoing_region->has_parent())
    {
      outgoing_region->clear_parent();
      return;
    }

    outgoing->field_dec();
  }
}

namespace vrt::writebarrier
{
  void init(
    Location store_location,
    void* target,
    const Field& field,
    const void* source)
  {
    validate_store(store_location, target, source);

    if (!is_header_type(field.value_type))
    {
      std::memcpy(target, source, field.size);
      return;
    }

    auto* incoming = load_header(field.value_type, source);
    internal_check(
      (incoming != nullptr) && !incoming->finalizing &&
        (incoming->get_type_id() == field.type_id),
      Failure::invalid_write);

    const auto incoming_location = incoming->location();
    if (incoming_location.is_immortal() || incoming_location.is_immutable())
    {
      store_header(target, incoming);
      return;
    }

    auto* incoming_region = incoming->region();
    internal_check(
      (incoming_region != nullptr) && !incoming_region->destroying,
      Failure::invalid_write);

    if (incoming_region->is_frame_local())
    {
      auto* destination = store_region(store_location);
      const bool must_drag =
        (store_location.is_stack() &&
         (store_location.stack_index() < incoming_region->frame_depth)) ||
        (store_location.is_region() &&
         (!destination->is_frame_local() ||
          ((destination != incoming_region) &&
           (destination->frame_depth < incoming_region->frame_depth))));

      if (must_drag && !drag(destination, incoming))
        raise_error(Error::bad_store);

      store_header(target, incoming);
      return;
    }

    // A reference stored in a frame-local region remains an external root of
    // the incoming heap region. Moving it out of the argument register and
    // into the field therefore leaves stack_reference_count unchanged.
    if (is_frame_storage(store_location))
    {
      store_header(target, incoming);
      return;
    }

    auto* destination = store_location.to_region();
    if (destination == incoming_region)
    {
      store_header(target, incoming);
      const bool incoming_region_alive = incoming_region->stack_dec();
      internal_check(incoming_region_alive, Failure::invalid_write);

      return;
    }

    if (
      incoming_region->has_parent() ||
      incoming_region->is_ancestor_of(destination))
      raise_error(Error::bad_alloc_target);

    // Establish ownership before consuming the stack reference. A child with
    // one stack reference is allowed to transition to zero only after it has
    // a parent.
    incoming_region->set_parent(destination, incoming);
    store_header(target, incoming);
    const bool incoming_region_alive = incoming_region->stack_dec();
    internal_check(incoming_region_alive, Failure::invalid_write);
  }

  void copy(
    Location store_location,
    void* target,
    const Field& field,
    const void* source)
  {
    validate_store(store_location, target, source);

    if (!is_header_type(field.value_type))
    {
      std::memmove(target, source, field.size);
      return;
    }

    auto* incoming = load_header(field.value_type, source);
    auto* outgoing = load_header(field.value_type, target);
    internal_check(incoming != nullptr, Failure::invalid_write);

    internal_check(
      !incoming->finalizing && (incoming->get_type_id() == field.type_id) &&
        ((incoming->region() != nullptr) ||
         incoming->location().is_immutable() ||
         incoming->location().is_immortal()),
      Failure::invalid_write);

    if (incoming == outgoing)
      return;

    if (incoming->location().is_immortal())
    {
      store_header(target, incoming);
      drop_header(store_location, outgoing);
      return;
    }

    if (incoming->location().is_immutable())
    {
      incoming->field_inc();
      store_header(target, incoming);
      drop_header(store_location, outgoing);
      return;
    }

    auto* incoming_region = incoming->region();
    internal_check(!incoming_region->destroying, Failure::invalid_write);

    if (incoming_region->is_frame_local())
    {
      auto* destination = store_region(store_location);
      const bool must_drag =
        (store_location.is_stack() &&
         (store_location.stack_index() < incoming_region->frame_depth)) ||
        (store_location.is_region() &&
         (!destination->is_frame_local() ||
          ((destination != incoming_region) &&
           (destination->frame_depth < incoming_region->frame_depth))));

      if (must_drag && !drag(destination, incoming, false))
        raise_error(Error::bad_store);

      incoming->field_inc();
      store_header(target, incoming);
      drop_header(store_location, outgoing);
      return;
    }

    if (is_frame_storage(store_location))
    {
      incoming->field_inc();
      incoming_region->stack_inc();
      store_header(target, incoming);
      drop_header(store_location, outgoing);
      return;
    }

    auto* destination = store_location.to_region();
    if (destination == incoming_region)
    {
      incoming->field_inc();
      store_header(target, incoming);
      drop_header(store_location, outgoing);
      return;
    }

    if (
      incoming_region->has_parent() ||
      incoming_region->is_ancestor_of(destination))
      raise_error(Error::bad_store);

    incoming->field_inc();
    incoming_region->set_parent(destination, incoming);
    store_header(target, incoming);
    drop_header(store_location, outgoing);
  }

  void drop(Location store_location, const Field& field, void* source)
  {
    internal_check(
      (source != nullptr) && is_header_type(field.value_type),
      Failure::invalid_write);

    auto* outgoing = load_header(field.value_type, source);
    clear_header(source);
    drop_header(store_location, outgoing);
  }

  bool drag(Region* destination, Header* root, bool root_reference_is_move)
  {
    if (
      (destination == nullptr) || destination->destroying ||
      destination->is_finalizing() || (root == nullptr) ||
      root->location().is_immortal() || root->finalizing ||
      (root->region() == nullptr) || !root->region()->is_frame_local())
      return false;

    if (root->region() == destination)
      return true;

    std::vector<Header*> worklist;
    std::unordered_map<Header*, uintptr_t> internal_references;
    std::unordered_map<Region*, Header*> child_regions;
    uintptr_t destination_stack_decrements = 0;
    worklist.push_back(root);

    while (!worklist.empty())
    {
      auto* header = worklist.back();
      worklist.pop_back();

      auto existing = internal_references.find(header);
      if (existing != internal_references.end())
      {
        if (existing->second == std::numeric_limits<uintptr_t>::max())
          return false;

        existing->second++;
        continue;
      }

      if (header->location().is_immortal())
        continue;

      auto* source_region = header->region();
      if (
        (source_region == nullptr) || source_region->destroying ||
        header->finalizing)
        return false;

      if (source_region == destination)
      {
        if (!destination->is_frame_local())
        {
          if (
            destination_stack_decrements ==
            std::numeric_limits<uintptr_t>::max())
            return false;

          destination_stack_decrements++;
        }

        continue;
      }

      if (
        destination->is_frame_local() && source_region->is_frame_local() &&
        (destination->frame_depth >= source_region->frame_depth))
        continue;

      if (!source_region->is_frame_local())
      {
        if (destination->is_frame_local())
          continue;

        if (
          source_region->has_parent() && (source_region->parent == destination))
          continue;

        if (
          source_region->has_parent() ||
          source_region->is_ancestor_of(destination) ||
          !child_regions.emplace(source_region, header).second)
          return false;

        continue;
      }

      internal_references.emplace(header, 1);
      header->trace_fn([&](Header* child) { worklist.push_back(child); });
    }

    if (!root_reference_is_move)
    {
      auto root_references = internal_references.find(root);
      if (
        (root_references == internal_references.end()) ||
        (root_references->second == 0))
        return false;

      root_references->second--;
    }

    for (const auto& [header, internal_count] : internal_references)
    {
      if (
        (header->reference_count < internal_count) ||
        (header->region() == nullptr) || !header->region()->contains(header))
        return false;
    }

    destination->stack_inc();

    for (const auto& [region, entry] : child_regions)
    {
      region->set_parent(destination, entry);
      const bool child_region_alive = region->stack_dec();
      internal_check(child_region_alive, Failure::invalid_write);
    }

    for (const auto& [header, internal_count] : internal_references)
    {
      auto* source_region = header->region();
      const auto external_count = header->reference_count - internal_count;
      destination->stack_inc(external_count);

      const bool removed = source_region->remove(header);
      internal_check(removed, Failure::invalid_write);

      header->set_location(Location(destination));
      destination->insert(header);
    }

    if (destination_stack_decrements != 0)
    {
      const bool destination_alive =
        destination->stack_dec(destination_stack_decrements);
      internal_check(destination_alive, Failure::invalid_write);
    }

    const bool destination_alive = destination->stack_dec();
    internal_check(destination_alive, Failure::invalid_write);

    return true;
  }
}
