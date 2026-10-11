#pragma once
#include <liburing.h>
#include <fcntl.h>
#include <unistd.h>
#include "EventHandler.h"

// A throwaway demo handler — NOT a pentest-kit tool, just the smallest thing
// that exercises the full reactor shape with zero privileges and no network:
// it reads a file block-by-block THROUGH the manager and writes it to stdout.
//
// This is the template a real policy follows:
//   has_next()    → "is there another block to read?"
//   next_op()     → "prep a READ at the current offset"
//   on_complete() → "got bytes: write them, advance; got 0/err: we're done"
class FileReader : public EventHandler {
private:
    int    fd;
    char   buf[4096];
    off_t  offset = 0;
    bool   eof    = false;

public:
    explicit FileReader(const char* path) { fd = open(path, O_RDONLY); }
    ~FileReader() override { if (fd >= 0) close(fd); }

    bool ok() const { return fd >= 0; }

    unsigned window() const override { return 1; }      // sequential: one at a time
    bool     has_next() const override { return !eof; }

    void next_op(io_uring_sqe* sqe) override {
        io_uring_prep_read(sqe, fd, buf, sizeof(buf), offset);
        io_uring_sqe_set_data64(sqe, (uint64_t)offset); // tag = where this read was
    }

    void on_complete(uint64_t /*tag*/, int res) override {
        if (res <= 0) { eof = true; return; }           // 0 = EOF, <0 = -errno
        write(STDOUT_FILENO, buf, res);
        offset += res;
    }
};
