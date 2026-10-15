/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    const unsigned char *content;
    size_t length, offset, depth;
    void *(*allocate)(size_t);
    void (*release)(void *);
} ParseBuffer;

static size_t allocated, released;

static void *allocate(size_t size)
{
    allocated = size;
    void *p = malloc(size);
    if (!p) exit(6);
    memset(p, 0xa5, size);
    return p;
}

static void release(void *p)
{
    released++;
    free(p);
}

static void record(const void *p, size_t size)
{
    if (fwrite(p, size, 1, stdout) != 1) exit(7);
}

static void check(int (*parse)(void *, ParseBuffer *), unsigned char *input,
                  size_t length, size_t offset)
{
    unsigned char item[64] = {0};
    ParseBuffer buffer = {input, length, offset, 0, allocate, release};
    allocated = released = 0;
    uint64_t result[5];
    result[0] = parse(item, &buffer);
    result[1] = buffer.offset;
    result[2] = allocated;
    result[3] = released;
    uint32_t type;
    memcpy(&type, item + 24, sizeof(type));
    result[4] = type;
    record(result, sizeof(result));
    if (result[0]) {
        void *output;
        memcpy(&output, item + 32, sizeof(output));
        record(output, allocated);
        free(output);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st)) return 3;
    void *base = (void *)UINT64_C(0x5508200000);
    if (mmap(base, st.st_size, PROT_READ | PROT_EXEC,
             MAP_PRIVATE | MAP_FIXED_NOREPLACE, fd, 0) != base) return 4;
    close(fd);
    int (*parse)(void *, ParseBuffer *) =
        (void *)UINT64_C(0x550843e1ca);
    size_t page = sysconf(_SC_PAGESIZE);
    unsigned char *mapping = mmap(NULL, page * 4, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED ||
        mprotect(mapping + page * 3, page, PROT_NONE)) return 5;
    unsigned state = 87967;
    static const char escapes[] = "\\n\\r\\t\\b\\f\\\"\\\\\\/\\u0041\\u20ac\\ud834\\udd1e\\uD800\\q";
    for (int test = 0; test < 6000; test++) {
        size_t n = test < 5900 ? test % 257 + 1 : 8192;
        unsigned char *input = mapping + page * 3 - n;
        for (size_t i = 0; i < n; i++) {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            input[i] = (test % 3) ? 'a' + state % 26 :
                       (unsigned char)escapes[state % (sizeof(escapes) - 1)];
        }
        if (test % 7) input[0] = '"';
        if (test % 5) input[n - 1] = '"';
        if (test % 11 == 0) input[n - 1] = '\\';
        check(parse, input, n, 0);
    }
    static const char *const cases[] = {
        "\"\"", "\"abc\"", "\"a\\nb\\rc\\td\\be\\ff\"",
        "\"a\\\"b\\\\c\\/d\"", "\"\\u0000abc\"", "\"\\u0041\"",
        "\"abc\\u20acdef\"", "\"abc\\ud834\\udd1edef\"",
        "\"\\ud800\"", "\"\\udc00\"", "\"\\ud800\\u0041\"",
        "\"abc\\qdef\"", "\"abc\\u00xz\"", "\"abc\\",
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        for (size_t n = 1; n <= strlen(cases[i]); n++) {
            for (size_t prefix = 0; prefix < 4; prefix++) {
                unsigned char *input = mapping + page * 3 - n - prefix;
                memset(input, ' ', prefix);
                memcpy(input + prefix, cases[i], n);
                check(parse, input, n + prefix, prefix);
            }
        }
    }
    return 0;
}
