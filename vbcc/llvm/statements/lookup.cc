#include "../codegen.h"

#include <functional>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <set>

namespace vbcc
{
  namespace llvm_backend
  {
    std::optional<LoweredSignature>
    LLVMCodegen::resolve_lookup_sig(const Node& statement)
    {
      auto method_id = statement / MethodId;
      std::set<std::string> candidate_ids;

      const auto collect_class = [&](const Node& definition) {
        for (const auto& method : *(definition / Methods))
        {
          if ((method / MethodId)->location() == method_id->location())
            candidate_ids.insert(node_text(method / FunctionId));
        }
      };

      std::function<bool(const Node&)> collect_type;
      collect_type = [&](const Node& type) {
        if (!type)
          return false;

        if (type == ClassId)
        {
          for (const auto& definition : state.classes)
          {
            if ((definition / ClassId)->location() == type->location())
            {
              collect_class(definition);
              return true;
            }
          }

          return false;
        }

        if (type == TypeId)
        {
          for (const auto& alias : state.typealiases)
          {
            if ((alias / TypeId)->location() == type->location())
              return collect_type(alias / Type);
          }

          return false;
        }

        if (type == Union)
        {
          bool resolved = false;

          for (const auto& member : *type)
            resolved = collect_type(member) || resolved;

          return resolved;
        }

        return false;
      };

      bool resolved = false;
      auto function = statement->parent(Func);

      if (!function)
        function = statement->parent(FuncOnce);

      if (function)
      {
        auto lookups =
          state.func_lookups.find(node_text(function / FunctionId));

        if (lookups != state.func_lookups.end())
        {
          auto lookup = lookups->second.find(node_text(statement / LocalId));

          if (lookup != lookups->second.end())
            resolved = collect_type(lookup->second.src_type);
        }
      }

      if (!resolved)
      {
        fail(statement, "dynamic lookup type metadata is unavailable");
        return {};
      }

      std::optional<LoweredSignature> signature;

      for (const auto& candidate_id : candidate_ids)
      {
        auto candidate = functions.find(candidate_id);

        if (candidate == functions.end())
        {
          fail(
            statement,
            "dynamic method function '" + candidate_id + "' is unavailable");
          return {};
        }

        const auto& candidate_signature = candidate->second.signature;

        if (signature && (*signature != candidate_signature))
        {
          fail(
            statement,
            "dynamic method implementations have incompatible LLVM "
            "signatures");
          return {};
        }

        signature = candidate_signature;
      }

      if (!signature)
      {
        fail(
          statement,
          "dynamic method '" + node_text(method_id) +
            "' has no callable implementation");
        return {};
      }

      return signature;
    }

    bool LLVMCodegen::emit_lookup(const Node& statement)
    {
      auto dst = statement / LocalId;
      auto receiver = locals.find_value(statement / Rhs);

      if (!receiver)
      {
        fail(
          statement,
          "lookup from unknown local '" + node_text(statement / Rhs) + "'");
        return false;
      }

      if (
        (receiver->type.runtime_type != vrt::ValueType::object) ||
        (receiver->value == nullptr))
      {
        fail(statement, "dynamic lookup receiver is not an object");
        return false;
      }

      if (runtime.object_lookup_method == nullptr)
      {
        fail(statement, "LLVM dynamic lookup runtime is unavailable");
        return false;
      }

      auto method_name = ST::di().string(statement / MethodId);
      auto method = state.method_ids.find(method_name);

      if (method == state.method_ids.end())
      {
        fail(statement / MethodId, "dynamic lookup method id is unavailable");
        return false;
      }

      auto signature = resolve_lookup_sig(statement);

      if (!signature)
        return false;

      auto* pointer_type = llvm::PointerType::getUnqual(context);
      auto* word_type = module.getDataLayout().getIntPtrType(context);
      auto* method_value = llvm::ConstantInt::get(word_type, method->second);
      auto* callable = builder.CreateCall(
        runtime.object_lookup_method,
        {receiver->value, method_value},
        strip_sigil(node_text(dst)));
      LoweredType callable_type{
        IRValueType::Function,
        vrt::ValueType::raw_pointer,
        pointer_type,
        pointer_type};

      return locals.bind_value(
        statement,
        dst,
        LoweredValue{
          callable_type,
          callable,
          std::optional<LoweredSignature>{std::move(*signature)}});
    }
  }
}
