# Collection and Finalization

Collection reclaims unreachable region objects according to their region kind
and ownership state. Object metadata identifies fields that must be traced or
released and provides an optional generated finalizer thunk.

Finalization runs only after the object has entered the runtime's finalizing
state. Region teardown must preserve parent and cown ownership until dependent
objects have completed their transitions.

Fixtures document the exact finalization ordering they assert; see
[VRT coverage](coverage.md).