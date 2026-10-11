# reactor-skeleton — the io_uring reactor shape (for porting pentest-kit later)

This is a **standalone skeleton**, deliberately kept OUTSIDE pentest-kit. It shows
the completion-model reactor you'd swap in for `EpollManager`, with a throwaway
`FileReader` demo instead of a real tool.

## Files (mirrors pentest-kit's layout)
| File | pentest-kit counterpart | role |
|---|---|---|
| `EventHandler.h`   | `EventHandler.h`   | the reshaped interface (next_op / on_complete) |
| `IoUringManager.*` | `EpollManager.*`   | the reactor: submit → wait_cqe → dispatch → refill |
| `FileReader.h`     | a Policy in `Batcher.h` | throwaway demo handler (reads a file) |
| `main.cpp`         | `main.cpp`         | identical wiring: build handler, register, run |

## Build & run
    sudo apt-get install -y liburing-dev   # one-time (not yet installed on this box)
    make
    ./reactor-demo main.cpp                 # prints a file via the io_uring reactor

## The interface shift in one glance
    epoll:     handle_write(fd) / handle_read(fd)   // "you're READY, do the syscall"
    io_uring:  next_op(sqe)     / on_complete(tag,res) // "do THIS" / "it's DONE, here's the result"

## When you port a real tool
1. Make your policy inherit this `EventHandler`.
2. Move the recv/send/connect OUT of handle_read/write and into `next_op`
   (as io_uring_prep_recv / prep_send / prep_connect).
3. Move the result interpretation (bytes, errno, status) into `on_complete`.
4. Keep BroadCaster, WordListReader, Factory, Context exactly as they are.

TinyScanner first — IORING_OP_CONNECT returns the connect result directly in
`cqe->res`, so you delete the getsockopt(SO_ERROR) dance entirely.

## Note
`io_uring_sqe_set_data64` / `io_uring_cqe_get_data64` need liburing ≥ ~2.1
(fine on this modern kernel). If your liburing is older, use `set_data`/`get_data`
with a `void*` cast instead.
