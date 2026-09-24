# VRT ABI

**Current:** public C and C++ declarations live under `include/vrt/` and use
`VRT_EXPORT` for externally visible symbols. Generated code links against
`libvrt` and calls these functions with C calling convention.

The ABI covers runtime and program initialization, invocation boundaries,
function and class metadata, threads and frames, values, objects, arrays,
regions, and errors. Implementation-only C++ types under `vrt/` are not public
ABI.

Changes to exported structure layout, enum values, or signatures require a
coordinated LLVM emitter and runtime update.