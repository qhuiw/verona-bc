#pragma once

#include "location.h"
#include "thread.h"

#include <csetjmp>
#include <cstdint>
#include <optional>
#include <vrt/error.h>
#include <vrt/value.h>

namespace vrt
{
  struct ErrorBoundary;
  struct Header;

  /** Language-raise target associated with one active logical frame. */
  struct Continuation
  {
    Continuation* parent = nullptr;
    Frame* frame = nullptr;
    std::jmp_buf state{};
    std::optional<uint64_t> raised_value{};
  };

  /** Runtime state bound to one native thread and native call stack. */
  struct ThreadContext
  {
    Thread thread{};
    Continuation* continuation = nullptr;
    ErrorBoundary* error_boundary = nullptr;

    /** Return the context bound to the calling native thread. */
    static ThreadContext& get();

    /** Return the bound context, or null if it is uninitialized. */
    static ThreadContext* try_get();

    /** Bind a fresh context to the calling native thread. */
    static void init();

    /** Destroy the context bound to the calling native thread. */
    static void deinit();

    /** Relocate a current-frame allocation so it survives a normal return. */
    void escape(Header* header);

    /** Raise a type-erased value through an older active stack Location. */
    [[noreturn]] void
    raise(ValueType value_type, uint64_t value, Location target);

    /** Raise a runtime Error to the innermost error boundary. */
    [[noreturn]] void raise_error(Error error);

    /** Invoke function under a nested runtime Error boundary. */
    [[nodiscard]] ErrorInfo
    try_invoke(InvocationFunction function, void* user_context);

    /** Run cleanup while preserving any caller frame and contain its Error. */
    [[nodiscard]] ErrorInfo
    run_cleanup(InvocationFunction function, void* user_context);

    /** Destroy frames through, but not including, target. */
    void unwind_frames(Frame* target);
  };
}
