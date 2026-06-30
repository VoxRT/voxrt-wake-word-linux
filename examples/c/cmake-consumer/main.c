/*
 * VoxRT wake-word — minimum CMake-consumer smoke test.
 *
 * Doesn't run inference — its job is to prove that
 * `find_package(VoxRTWakeWord ...)` resolved the include + lib
 * correctly. Loads the .vxrt, prints SDK + ABI versions, and exits.
 *
 * Build:
 *   cmake -S . -B build && cmake --build build
 *
 * Run:
 *   ./build/cmake-consumer voxrt_wake_word.vxrt
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <voxrt_wake_word.h>

int main(int argc, char **argv) {
    printf("VoxRT wake-word SDK version: %s\n", voxrt_wake_word_version());

    uint32_t abi = voxrt_wake_word_abi_version();
    printf("                ABI version: %u.%u\n",
           (abi >> 16) & 0xffff, abi & 0xffff);

    if (argc < 2) {
        printf("\n(pass a model.vxrt path to also smoke-test the loader)\n");
        return 0;
    }

    int fd = open(argv[1], O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", argv[1], strerror(errno));
        return 2;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return 2;
    }
    void *bytes = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (bytes == MAP_FAILED) {
        fprintf(stderr, "mmap %s: %s\n", argv[1], strerror(errno));
        return 2;
    }

    voxrt_wake_word_t *engine = NULL;
    voxrt_status_t rc = voxrt_wake_word_create((const uint8_t *)bytes,
                                               (size_t)st.st_size, &engine);
    if (rc != VOXRT_OK) {
        fprintf(stderr, "voxrt_wake_word_create: %d\n", rc);
        munmap(bytes, (size_t)st.st_size);
        return 3;
    }

    printf("    Loaded model: %s (%lld bytes) — handle %p\n",
           argv[1], (long long)st.st_size, (void *)engine);
    printf("    OK.\n");

    voxrt_wake_word_destroy(engine);
    munmap(bytes, (size_t)st.st_size);
    return 0;
}
