# Objects and Arrays

Object descriptors contain class identity, field layout, method dispatch
entries, singleton storage, and optional finalizer metadata. Object allocation
entry points create frame-local values, allocate into an existing region, or
create a new RC or arena region.

Arrays expose equivalent allocation, retain, release, freeze, and escape
operations plus checked copy, fill, and lexicographic comparison over encoded
elements.

Generated code passes object data addresses and array element-storage pointers;
runtime headers remain private.