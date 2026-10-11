// mycat.c — your very first io_uring program.
//
// It reads a file and prints it to stdout, doing the READ through io_uring
// so you can watch ONE operation's lifecycle in isolation:
//
//     get SQE  ->  describe op  ->  submit  ->  wait for CQE  ->  read result
//
// Remember the mantra: epoll tells you when you CAN read; io_uring hands you
// the bytes and tells you the read is DONE. There is no "readiness" step here.
//
// Build:  make
// Run:    ./mycat mycat.c      (prints its own source back at you)

#include <liburing.h>   // the friendly wrapper over the 3 raw syscalls
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_DEPTH 1          // how many SQEs the ring can hold in flight
#define BLOCK 4096             // read this many bytes per operation

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    // Plain open() — NOT the lesson, kept boring on purpose so the one new
    // idea (the ring) stands alone. (You could also submit an IORING_OP_OPENAT.)
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    // 1. SET UP THE RINGS.
    //    This is io_uring_setup() under the hood: it allocates the Submission
    //    Queue and Completion Queue and mmap's them into BOTH your process and
    //    the kernel. That shared memory is why submitting work needs no copy.
    struct io_uring ring;
    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        return 1;
    }

    char buf[BLOCK];
    off_t offset = 0;

    for (;;) {
        // 2. GET A FREE SQE (Submission Queue Entry) from the ring.
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);

        // 3. DESCRIBE THE OPERATION. We are not reading yet — we are filling in
        //    a request: "op = READ, fd, into buf, up to BLOCK bytes, at offset".
        io_uring_prep_read(sqe, fd, buf, BLOCK, offset);

        // 4. STAMP A CORRELATION TAG (user_data). With one op it's trivial, but
        //    this is exactly how you'd know WHICH request a completion belongs
        //    to when hundreds are in flight — your slots[fd] trick, generalised.
        io_uring_sqe_set_data(sqe, (void *)(long)fd);

        // 5. SUBMIT. This is the ONE hot-path syscall (io_uring_enter). It can
        //    publish MANY SQEs at once; here it publishes our single read.
        io_uring_submit(&ring);

        // 6. WAIT FOR A COMPLETION. Blocks until the kernel posts a CQE saying
        //    "that read you asked for is finished."
        struct io_uring_cqe *cqe;
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) { fprintf(stderr, "wait_cqe: %s\n", strerror(-ret)); break; }

        // 7. READ THE RESULT. cqe->res is EXACTLY what read() would have
        //    returned: >0 bytes, 0 = EOF, or -errno on failure.
        int bytes = cqe->res;
        void *tag = io_uring_cqe_get_data(cqe);   // our fd tag comes back
        (void)tag;

        // 8. MARK THE CQE CONSUMED so its slot in the ring can be reused.
        io_uring_cqe_seen(&ring, cqe);

        if (bytes < 0) { fprintf(stderr, "read: %s\n", strerror(-bytes)); break; }
        if (bytes == 0) break;                    // EOF — done

        write(STDOUT_FILENO, buf, bytes);         // plain write, again boring
        offset += bytes;                          // advance, loop, reuse the ring
    }

    // 9. CLEAN UP — tears down the rings (and the mmap'd shared memory).
    io_uring_queue_exit(&ring);
    close(fd);
    return 0;
}
