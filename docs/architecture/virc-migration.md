# VBCC to VIRC Migration

## Scope

The shared compiler is moving from `vbcc/` to `virc/` so its analyzed state
and passes can serve more than one output backend. VIR names the in-memory
representation; VIRC names the compiler that validates and analyzes it.

This document records repository-facing path, symbol, and compatibility
changes. It intentionally excludes branch-rewrite procedures and local
backend work that is not part of the shared VIRC/VBC migration.

## Migration Phases

| Phase | Status | Result |
| --- | --- | --- |
| Mechanical source move | Complete | Shared sources retain one-to-one history under `virc/`. |
| Concept rename | Complete | `vir` owns VIR tokens and `virc` owns compiler code. |
| Responsibility extraction | Pending | Analysis, support, reader, model, and VBC encoding gain dedicated homes. |
| Build target split | Pending | Core, reader, and VBC emitter targets become independently consumable. |
| Compatibility removal | Pending | Legacy names remain until downstream users migrate. |

## Current Source Map

| Previous owner | Current owner | Responsibility |
| --- | --- | --- |
| `include/vbcc.h` | `include/vir.h` | Public VIR tokens and well-formedness contracts |
| `vbcc/*.h`, `vbcc/*.cc` | `virc/*.h`, `virc/*.cc` | Shared compiler implementation |
| `vbcc/passes/**` | `virc/passes/**` | VIR validation, analysis, and optimization passes |
| `virc/bitset.{h,cc}` | `virc/analysis/bitset.{h,cc}` | Reusable liveness bitset |
| `virc/irsubtype.h` | `virc/analysis/ir_subtype.h` | VIR subtype analysis |
| `virc/sequent.h` | `virc/analysis/sequent.h` | Generic sequent-calculus framework |
| `virc/lang.cc` diagnostics | `virc/support/diagnostics.{h,cc}` | Shared error construction and source locations |
| `virc/lang.{h,cc}` literals | `virc/support/literals.{h,cc}` | Literal conversion, escaping, and parser helpers |
| `virc/from_chars.h` | `virc/support/literals.h` | Portable literal number parsing |
| `virc/lang.h` statement patterns | `virc/passes/patterns.h` | Shared patterns consumed by compiler passes |
| `virc/passes/parser.cc` | `virc/reader/parser.cc` | Textual VIR tokenization and parsing |
| `virc/passes/statements.cc` | `virc/reader/statements.cc` | Textual statement construction |
| `virc/passes/labels.cc` | `virc/reader/labels.cc` | Textual body-to-label grouping |
| `virc/lang.h` reader declarations | `virc/reader/reader.h` | Textual reader API |
| `virc/bytecode.{h,cc}` | `virc/vbc/emitter.{h,cc}` | Primary descendant of the VBC serialization monolith |
| `virc/vbc/emitter.h` compilation state | `virc/model/compilation.h` | Output-neutral analyzed compilation state |
| `virc/vbc/emitter.cc` state methods | `virc/model/compilation.cc` | Compilation indexing and liveness state |
| `virc/stringtable.{h,cc}` | `virc/model/name_table.{h,cc}` | Interned execution and debug names |
| Encoded complex type byte vectors | `virc/model/type_table.{h,cc}` | Structured backend-neutral type records |
| Reserved semantic IDs | `virc/model/ids.h` | Backend-neutral compiler indices |
| `include/vbci.h` wire declarations | `include/vbc/format.h` | Neutral VBC constants, enums, and instruction contract |
| `include/vbci.h` live value tags | `vbci/value_type.h` | Interpreter-private value representation categories |
| `virc/vbc/emitter.cc` byte helpers | `virc/vbc/encoder.{h,cc}` | LEB128, floating-point, debug-op, and string encoding |
| Inline emitter string-table loops | `virc/vbc/string_table.{h,cc}` | Execution and debug string-table serialization |
| Inline emitter type encoding | `virc/vbc/type_encoding.{h,cc}` | Region operands and structured type-table serialization |
| Inline emitter instruction chain | `virc/vbc/instruction_encoder.{h,cc}` | VIR statement and terminator lowering to VBC operations |
| Inline emitter debug state | `virc/vbc/debug_info.{h,cc}` | Source mapping, debug operations, and compressed debug payloads |
| `vbcc/llvm/**` | `vbcc/llvm/**` | Legacy optional backend pending its separate migration |

The extraction phases extend this table with symbol-level destinations. Line
numbers belong in the corresponding commit bodies because they are tied to a
specific parent snapshot.

## Renamed Concepts

| Previous name | Current name |
| --- | --- |
| `vbcc` token namespace | `vir` |
| `vbcc` compiler namespace | `virc` |
| `assignids` C++ API | `assign_ids` |
| `validids` C++ API | `validate_ids` |

Pass display names remain `assignids` and `validids` so command-line pass
selection and existing golden output stay stable during migration.

## Compatibility

- `include/vbcc.h` forwards the former token namespace to `vir`.
- `include/vbci.h` forwards neutral wire names from `vbc` into `vbci`.
- The `vbcc` executable and existing CMake target names remain available
  during the transition.
- Compatibility layers forward to one implementation; they do not duplicate
  compiler or emitter code.

Compatibility can be removed after downstream source includes, CMake target
references, scripts, and pass invocations have migrated and the deprecation
has crossed one documented transition period.