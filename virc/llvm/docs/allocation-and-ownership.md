# Allocation and Ownership

Object and array allocation lowerers call VRT entry points for new, heap, or
region allocation. Generated class metadata supplies object layout and dynamic
method information; array allocation supplies element representation metadata.

**Current:** retain and release are implemented for VRT object and array
representations. Scalars, raw pointers, and `none` require no lifetime calls.

**Target:** retain and release lowering for references, cowns, dynamic values,
and aggregate values is not implemented. Encountering those paths reports an
emission error. Runtime ownership semantics are authoritative in
[VRT Regions and Ownership](../../../vrt/docs/regions-and-ownership.md).