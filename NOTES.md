# io_uring playground — notes

## The mantra
**epoll tells you when you CAN do I/O; io_uring tells you when the I/O is DONE.**
Readiness interface vs. completion interface. That's the whole leap.

## The lifecycle (every io_uring op is these 6 beats)
1. `io_uring_get_sqe`  — grab a free Submission Queue Entry
2. `io_uring_prep_read/write/recv/...` — describe the operation
3. `io_uring_sqe_set_data` — stamp a correlation tag (who is this?)
4. `io_uring_submit` — publish SQEs (the one hot syscall: io_uring_enter)
5. `io_uring_wait_cqe` — block for a Completion Queue Entry
6. `cqe->res` = result (like the syscall's return) → `io_uring_cqe_seen` to free it

## Build & run
    sudo apt-get install -y liburing-dev   # one-time
    make
    ./mycat mycat.c

## Things to poke (this is how it sticks)
- Change `BLOCK` to 8 bytes — watch it loop many more times (ring reused each pass).
- `strace -e io_uring_enter ./mycat mycat.c` — count how FEW syscalls happen vs.
  how many read()s a normal cat would make. That's the efficiency story, live.
- Add a second file: submit TWO reads before waiting, tell them apart by the
  `user_data` tag. That's batching — the thing epoll can't do.
- Try `io_uring_prep_openat` instead of plain open() to submit the open too.

## Where this goes for PentestKit
Your epoll reactor does: EPOLLIN -> handle_read -> recv().  The io_uring version
submits the recv up front; the completion IS the data. The Policy pattern stays;
only EpollManager's guts (wait_cqe + dispatch by user_data) change.
