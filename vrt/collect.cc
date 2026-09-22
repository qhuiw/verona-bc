#include "collect.h"

#include "failure.h"
#include "header.h"
#include "region.h"
#include "region_ext.h"

#include <deque>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vrt
{
  namespace
  {
    enum class WorkType
    {
      header,
      region
    };

    struct WorkItem
    {
      WorkType type;
      void* value;
      Region* owner;
    };

    struct HeaderRelease
    {
      Header* header;
      Region* owner;
    };

    struct CollectorState
    {
      std::deque<WorkItem> work;
      std::vector<HeaderRelease> headers;
      std::vector<Region*> regions;
      bool draining = false;
    };

    thread_local CollectorState state;

    void drain()
    {
      internal_check(!state.draining, Failure::invalid_header_state);
      state.draining = true;

      while (true)
      {
        while (!state.work.empty())
        {
          const auto item = state.work.front();
          state.work.pop_front();

          switch (item.type)
          {
            case WorkType::header:
            {
              auto* header = static_cast<Header*>(item.value);
              header->finalize();
              state.headers.push_back({header, item.owner});
              break;
            }

            case WorkType::region:
            {
              auto* region = static_cast<Region*>(item.value);
              region->finalize_contents();
              state.regions.push_back(region);
              break;
            }
          }
        }

        auto headers = std::move(state.headers);
        state.headers.clear();
        for (const auto& release : headers)
        {
          release.header->destroy_storage();
          if (release.owner != nullptr)
            release.owner->stack_dec();
        }

        auto regions = std::move(state.regions);
        state.regions.clear();
        for (auto* region : regions)
          region->release_dead_objects();

        if (state.work.empty())
          break;
      }

      state.draining = false;
    }

    void start_drain()
    {
      if (!state.draining)
        drain();
    }
  }

  void collect(Header* header)
  {
    if (
      (header == nullptr) || header->location().is_immortal() ||
      header->finalizing)
      return;

    auto* region = header->region();
    if (
      (region == nullptr) || region->destroying || region->is_finalizing() ||
      region->is_arena())
      return;

    region->stack_inc();
    if (!region->remove(header))
    {
      region->stack_dec();
      return;
    }

    state.work.push_back({WorkType::header, header, region});
    start_drain();
  }

  void collect(Region* region)
  {
    if ((region == nullptr) || region->destroying)
      return;

    internal_check(region->parent == nullptr, Failure::invalid_region_state);

    region->destroying = true;
    const bool began_finalizing = region->begin_finalizing();
    internal_check(began_finalizing, Failure::invalid_region_state);

    state.work.push_back({WorkType::region, region, nullptr});
    start_drain();
  }

  void collect_scc(Header* representative)
  {
    internal_check(
      (representative != nullptr) &&
        (representative->location() == Location::immutable()) &&
        (representative->representative() == representative) &&
        (representative->get_arc() == 0),
      Failure::invalid_header_state);

    if (!representative->try_begin_scc_collection())
      return;

    std::vector<Header*> work{representative};
    std::unordered_set<Header*> members;
    members.emplace(representative);

    while (!work.empty())
    {
      auto* header = work.back();
      work.pop_back();

      header->trace_fn([&](Header* child) {
        if (
          child->location().is_immutable() &&
          (child->representative() == representative) &&
          members.emplace(child).second)
          work.push_back(child);
      });
    }

    for (auto* member : members)
      state.work.push_back({WorkType::header, member, nullptr});

    start_drain();
  }
}