# Regions and Ownership

VRT supports reference-counted and arena regions selected by `vrt_region_type`.
Frame-local allocation belongs to the current logical frame; heap allocation
uses an object in the destination region as its locator; region allocation
creates a new entry point.

Object and array retain/release operations manage owning register references.
Escape operations relocate current-frame-local values when they must outlive
the frame. Write barriers maintain ownership when references cross region
boundaries.

Language semantics remain authoritative in the
[Memory Model](../../vc/docs/19-memory-model.md).