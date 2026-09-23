#include "freeze.h"

#include "failure.h"
#include "header.h"
#include "region.h"
#include "region_ext.h"

#include <limits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vrt
{
  namespace
  {
    struct RegionTransition
    {
      Region* region;
      bool clear_parent;
      RC stack_adjustment;
    };

    Header* post_order_mark(Header* header)
    {
      return reinterpret_cast<Header*>(
        reinterpret_cast<uintptr_t>(header) | uintptr_t{1});
    }

    Header* remove_post_order_mark(Header* header)
    {
      return reinterpret_cast<Header*>(
        reinterpret_cast<uintptr_t>(header) & ~uintptr_t{1});
    }

    bool is_post_order(Header* header)
    {
      return (reinterpret_cast<uintptr_t>(header) & uintptr_t{1}) != 0;
    }

    void scc_union(Header* child, Header* representative)
    {
      auto* child_root = Header::find(child);
      auto* representative_root = Header::find(representative);
      internal_check(
        child_root != representative_root, Failure::invalid_header_state);

      if (
        reinterpret_cast<uintptr_t>(child_root) >
        reinterpret_cast<uintptr_t>(representative_root))
        std::swap(child_root, representative_root);

      const auto child_count = child_root->get_rc();
      const auto representative_count = representative_root->get_rc();
      internal_check(
        (child_count != 0) &&
          ((child_count - 1) <=
           (std::numeric_limits<RC>::max() - representative_count)),
        Failure::invalid_header_state);

      representative_root->set_rc(representative_count + child_count - 1);
      child_root->set_location(Location::scc_ptr(representative_root));
    }

    void trace_fields(Header* header, std::vector<Header*>& dfs)
    {
      header->trace_fn([&](Header* child) {
        const auto location = child->location();

        if (location.is_pending())
        {
          dfs.push_back(child);
          return;
        }

        if (location.is_scc_ptr())
        {
          dfs.push_back(child);
          return;
        }

        if (location.is_immutable() || location.is_immortal())
          return;

        if (location.is_region())
          dfs.push_back(child);
      });
    }

    void canonicalize(const std::unordered_set<Header*>& frozen_set)
    {
      for (auto* member : frozen_set)
      {
        auto* representative = Header::find(member);
        internal_check(
          representative->location() == Location::immutable(),
          Failure::invalid_header_state);

        if (member != representative)
          member->set_location(Location::scc_ptr(representative));
      }
    }

    bool can_freeze(Header* root)
    {
      std::vector<Header*> work{root};
      std::unordered_set<Header*> visited;

      while (!work.empty())
      {
        auto* header = work.back();
        work.pop_back();

        if (!visited.emplace(header).second)
          continue;

        const auto location = header->location();
        if (location.is_immutable() || location.is_immortal())
          continue;

        if (location.is_stack() || location.is_pending())
          return false;

        auto* region = location.to_region();
        if (
          header->finalizing || region->destroying || region->is_finalizing() ||
          region->is_arena())
          return false;

        header->trace_fn([&](Header* child) { work.push_back(child); });
      }

      return true;
    }

    void freeze_local(Header* root)
    {
      internal_check(
        (root != nullptr) && root->location().is_region() &&
          root->region()->is_frame_local(),
        Failure::invalid_header_state);

      std::vector<Header*> dfs{root};
      std::vector<Header*> pending;
      std::vector<Header*> heap_roots;
      std::unordered_set<Header*> frozen_set;

      while (!dfs.empty())
      {
        auto* marked = dfs.back();
        dfs.pop_back();

        if (is_post_order(marked))
        {
          auto* header = remove_post_order_mark(marked);
          if (!pending.empty() && (pending.back() == header))
          {
            pending.pop_back();
            auto* representative = Header::find(header);
            representative->set_location(Location::immutable());
            representative->set_arc(representative->get_rc());
          }
          continue;
        }

        auto* header = marked;
        auto* representative = Header::find(header);
        const auto representative_location = representative->location();

        if (representative_location.is_pending())
        {
          const auto count = representative->get_rc();
          internal_check(count != 0, Failure::invalid_header_state);
          representative->set_rc(count - 1);

          while (!pending.empty() && (pending.back() != representative))
          {
            scc_union(pending.back(), representative);
            pending.pop_back();
          }
        }
        else if (representative_location.is_immutable())
        {
        }
        else if (representative_location.is_region())
        {
          auto* object_region = representative_location.to_region();
          if (!object_region->is_frame_local())
          {
            heap_roots.push_back(header);
            continue;
          }

          const bool removed = object_region->remove(header);
          internal_check(removed, Failure::invalid_region_state);
          header->set_location(Location::from_raw(Location::Pending));
          frozen_set.emplace(header);

          pending.push_back(header);
          dfs.push_back(post_order_mark(header));
          trace_fields(header, dfs);
        }
      }

      canonicalize(frozen_set);
      for (auto* heap_root : heap_roots)
      {
        const bool frozen = freeze(heap_root);
        internal_check(frozen, Failure::invalid_header_state);
      }
    }
  }

  bool freeze(Header* root)
  {
    internal_check(root != nullptr, Failure::invalid_header_state);
    const auto root_location = root->location();

    if (root_location.is_immutable() || root_location.is_immortal())
      return true;

    if (root_location.is_stack() || root_location.is_pending())
      return false;

    if (!can_freeze(root))
      return false;

    auto* root_region = root_location.to_region();
    if (root_region->is_frame_local())
    {
      freeze_local(root);
      return true;
    }

    std::vector<Header*> worklist{root};
    std::vector<Header*> dfs;
    std::vector<Header*> pending;
    std::unordered_set<Header*> frozen_set;
    std::vector<RegionTransition> region_transitions;

    while (!worklist.empty())
    {
      root = worklist.back();
      worklist.pop_back();

      if (root->location().is_immutable())
        continue;

      auto* region = root->region();
      internal_check(
        (region != nullptr) && !region->destroying &&
          !region->is_finalizing() && !region->is_arena(),
        Failure::invalid_region_state);

      const bool is_subregion = region->has_parent();
      RC arc_sum = 0;
      RC frozen_cross = 0;
      frozen_set.clear();
      dfs.push_back(root);

      while (!dfs.empty())
      {
        auto* marked = dfs.back();
        dfs.pop_back();

        if (is_post_order(marked))
        {
          auto* header = remove_post_order_mark(marked);
          if (!pending.empty() && (pending.back() == header))
          {
            pending.pop_back();
            auto* representative = Header::find(header);
            internal_check(
              representative->get_rc() <=
                (std::numeric_limits<RC>::max() - arc_sum),
              Failure::invalid_header_state);
            arc_sum += representative->get_rc();
            representative->set_location(Location::immutable());
            representative->set_arc(representative->get_rc());
          }

          continue;
        }

        auto* header = marked;
        auto* representative = Header::find(header);
        const auto representative_location = representative->location();

        if (representative_location.is_pending())
        {
          const auto count = representative->get_rc();
          internal_check(count != 0, Failure::invalid_header_state);
          representative->set_rc(count - 1);

          while (!pending.empty() && (pending.back() != representative))
          {
            scc_union(pending.back(), representative);
            pending.pop_back();
          }
        }
        else if (representative_location.is_immutable())
        {
        }
        else if (representative_location.is_region())
        {
          if (representative_location.to_region() == region)
          {
            const bool removed = region->remove(header);
            internal_check(removed, Failure::invalid_region_state);
            header->set_location(Location::from_raw(Location::Pending));
            frozen_set.emplace(header);

            pending.push_back(header);
            dfs.push_back(post_order_mark(header));
            trace_fields(header, dfs);
          }
          else if (!representative_location.to_region()->is_frame_local())
          {
            worklist.push_back(header);
          }
        }
      }

      canonicalize(frozen_set);

      for (auto* header : frozen_set)
      {
        auto* source_representative = header->representative();
        header->trace_fn([&](Header* target) {
          if (
            frozen_set.contains(target) &&
            (target->representative() != source_representative))
            frozen_cross++;
        });
      }

      RC unfrozen_to_frozen = 0;
      region->for_each_header([&](Header* header) {
        header->trace_fn([&](Header* target) {
          if (frozen_set.contains(target))
            unfrozen_to_frozen++;
        });
      });

      const bool freezes_entry_point =
        is_subregion && frozen_set.contains(region->entry_point);
      const RC parent_reference = freezes_entry_point ? 1 : 0;
      internal_check(
        arc_sum >= (frozen_cross + unfrozen_to_frozen + parent_reference),
        Failure::invalid_region_state);
      const RC stack_adjustment =
        arc_sum - frozen_cross - unfrozen_to_frozen - parent_reference;

      region_transitions.push_back(
        {region, freezes_entry_point, stack_adjustment});
    }

    for (auto transition = region_transitions.rbegin();
         transition != region_transitions.rend();
         ++transition)
    {
      if (transition->clear_parent)
        transition->region->clear_parent();

      if (transition->stack_adjustment != 0)
        transition->region->stack_dec(transition->stack_adjustment);
    }

    return true;
  }
}
