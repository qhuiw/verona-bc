# LLVM Type Representation

`LoweredType` records three related choices: an LLVM SSA type, an addressable
storage type, and a public `vrt::ValueType` runtime category. `LoweredValue`
adds the emitted LLVM value and an optional callable signature.

Primitive lowerers live under `types/`. Nominal classes use generated structure
layouts for their fields while object values use runtime-managed pointers.
Arrays carry an element runtime type ID. Dynamic and aggregate forms have local
lowering rules and are not VBCI boxed values.

The public runtime categories are defined in `include/vrt/value.h`. Private
VBCI register tags do not participate in native representation.