# Test Fixtures

Source fixtures live under `testsuite/v/`, textual VIR fixtures under
`testsuite/vir/`, LLVM-specific fixtures under `testsuite/llvm/`, and runtime
fixtures under `testsuite/vrt/`.

Compiler fixtures must be self-contained and may use only implicitly available
`_builtin` definitions. Compile-error cases belong under `v/compile_only/` and
have no run graph.

Every non-trivial LLVM or VRT fixture states the behavior it covers and its
non-goals in a short source header. Long multi-scenario fixtures may place that
detail in an adjacent README and link it from the source. Coverage text must
describe assertions the fixture actually performs, not intended future work.

Native source and VIR fixtures are explicitly allowlisted because LLVM/VRT
coverage is not yet equivalent to the general VBC/VBCI suite.