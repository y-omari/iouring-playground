#pragma once
#include <cstdint>

// Forward declaration — the interface only passes a pointer, so we don't need
// to drag <liburing.h> into every file that includes this header.
struct io_uring_sqe;

// ─────────────────────────────────────────────────────────────────────────────
// The COMPLETION-model handler interface — the io_uring counterpart to
// pentest-kit's readiness-based EventHandler.
//
//   epoll EventHandler:  handle_write(fd) / handle_read(fd)
//                        → "you are READY, now go call send()/recv() yourself"
//
//   io_uring EventHandler (this one):
//                        → next_op():     "describe the operation you want"
//                        → on_complete():  "that operation is DONE, here's the result"
//
// The reactor owns the syscalls now; the handler only describes work and
// reacts to results. The business logic (parse HTTP, checksum ICMP, decide
// OPEN/CLOSED) lives in on_complete — unchanged from the epoll version.
// ─────────────────────────────────────────────────────────────────────────────
class EventHandler {
public:
    virtual ~EventHandler() = default;

    // Does the handler still have work it could submit right now?
    // (Checked BEFORE grabbing an SQE, so we never reserve a slot we won't use.)
    virtual bool has_next() const = 0;

    // Describe the next operation into `sqe` (io_uring_prep_* + set the
    // user_data tag so you can recognise its completion). Called only when
    // has_next() is true.
    virtual void next_op(io_uring_sqe* sqe) = 0;

    // A previously-submitted op finished. `tag` is the user_data you stamped;
    // `res` is exactly what the syscall would have returned (>=0, or -errno).
    virtual void on_complete(uint64_t tag, int res) = 0;

    // How many operations may be in flight at once (your bounded window —
    // the io_uring equivalent of pentest-kit's `limit`).
    virtual unsigned window() const = 0;
};
