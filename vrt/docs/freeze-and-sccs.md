# Freeze and SCCs

Native freeze performs a read-only preflight over the reachable graph before
publishing immutable state. Unsupported arena, stack, pending, or finalizing
objects fail without partially freezing the graph.

Reference-counted cycles are collapsed with union-find while accounting for
tree edges, back edges, and references crossing into or out of the newly frozen
set. Nested region ownership transitions are deferred until all reachable RC
regions are published, then applied deepest first.

Freezing an interior object does not remove ownership of a region entry point.