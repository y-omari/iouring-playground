#include "IoUringManager.h"
#include "FileReader.h"
#include <cstdio>

// Wiring mirrors pentest-kit's main.cpp: build a handler, hand it to the
// reactor, run. Swap FileReader for a real policy and this never changes.
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    FileReader reader(argv[1]);
    if (!reader.ok()) { perror("open"); return 1; }

    try {
        IoUringManager reactor(/*depth=*/8);
        reactor.register_handler(&reader);
        reactor.run();
    } catch (const std::exception& e) {
        fprintf(stderr, "reactor error: %s\n", e.what());
        return 1;
    }
    return 0;
}
