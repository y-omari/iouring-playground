#include "IoUringManager.h"
#include <stdexcept>

IoUringManager::IoUringManager(unsigned depth) {
    // io_uring_setup() under the hood: allocates the SQ + CQ rings and mmap's
    // them into both this process and the kernel (the shared-memory channel).
    if (io_uring_queue_init(depth, &ring, 0) < 0)
        throw std::runtime_error("io_uring_queue_init failed");
}

IoUringManager::~IoUringManager() {
    io_uring_queue_exit(&ring);   // tears down the rings + shared mapping
}

void IoUringManager::register_handler(EventHandler* h) {
    handler = h;
}

// Submit new operations until the in-flight window is full (or the handler
// has nothing left / the SQ ring is full). One io_uring_submit publishes them
// all in a single io_uring_enter syscall — that batching is the efficiency win.
void IoUringManager::fill() {
    while (in_flight < handler->window() && handler->has_next()) {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) break;                 // SQ ring full — submit what we have
        handler->next_op(sqe);           // handler preps op + sets user_data
        ++in_flight;
    }
    io_uring_submit(&ring);
}

void IoUringManager::run() {
    if (!handler) return;

    fill();                              // prime the pump

    while (in_flight > 0) {
        io_uring_cqe* cqe;
        if (io_uring_wait_cqe(&ring, &cqe) < 0) break;   // block for ≥1 completion

        // Drain every completion the kernel has posted so far, in one pass.
        unsigned head, count = 0;
        io_uring_for_each_cqe(&ring, head, cqe) {
            handler->on_complete(io_uring_cqe_get_data64(cqe), cqe->res);
            --in_flight;
            ++count;
        }
        io_uring_cq_advance(&ring, count);   // mark them all consumed at once

        fill();                              // refill the window and loop
    }
}
