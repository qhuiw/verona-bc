# Threads and Frames

`vrt_thread_init()` binds a fresh logical Verona thread to the calling native
thread; `vrt_thread_deinit()` destroys that binding. Generated calls push
logical frames with `vrt_frame_enter()` and remove them with
`vrt_frame_leave()`.

Tail calls use `vrt_frame_reuse()` after lowering has moved arguments and
released other locals. Raise targets are stack locations associated with active
frames. Raise continuations preserve a type-erased payload while intermediate
frames are torn down.

Frame metadata references compiler-emitted `vrt_func` descriptors.