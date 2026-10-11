#pragma once
#include <liburing.h>
#include "EventHandler.h"

// The io_uring reactor — the drop-in replacement for pentest-kit's EpollManager.
// Same job (drive one handler to completion), different engine underneath:
// no epoll_wait + readiness; instead submit ops, wait for completions, refill.
class IoUringManager {
private:
    io_uring       ring;
    EventHandler*  handler    = nullptr;
    unsigned       in_flight  = 0;

    void fill();   // top the in-flight window back up to handler->window()

public:
    explicit IoUringManager(unsigned depth);
    ~IoUringManager();

    IoUringManager(const IoUringManager&)            = delete;
    IoUringManager& operator=(const IoUringManager&) = delete;

    void register_handler(EventHandler* h);
    void run();    // the event loop: fill → wait_cqe → dispatch → refill
};
