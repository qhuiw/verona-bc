# Values and Layouts

`vrt::ValueType` classifies native representation and lifetime behavior as
none, scalar, raw pointer, object, array, reference, cown, dynamic, or aggregate.
It is not a source type enumeration and is not the VBC primitive wire enum.

Compiler-emitted `TypeInfo` supplies storage size and element metadata needed
by generic runtime operations. Class `Field` metadata records offset, size,
type ID, and runtime value category for object fields.

VBCI's boxed `Value` remains interpreter-private and does not cross this ABI.