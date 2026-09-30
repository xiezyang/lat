#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static void fail(const char *name)
{
    fprintf(stderr, "%s: %s\n", name, strerror(errno));
    exit(1);
}

static long raw_syscall(long nr, long a0, long a1, long a2, long a3)
{
    return syscall(nr, a0, a1, a2, a3);
}

int main(void)
{
    struct timespec ts;
    struct timeval tv;
    char path[4096];
    char dirents[4096];
    int fd;
    long ret;

    ret = raw_syscall(SYS_getpid, 0, 0, 0, 0);
    if (ret != getpid()) {
        fail("getpid");
    }
    ret = raw_syscall(SYS_getppid, 0, 0, 0, 0);
    if (ret != getppid()) {
        fail("getppid");
    }
    ret = raw_syscall(SYS_getuid, 0, 0, 0, 0);
    if (ret != getuid()) {
        fail("getuid");
    }
    ret = raw_syscall(SYS_geteuid, 0, 0, 0, 0);
    if (ret != geteuid()) {
        fail("geteuid");
    }
    ret = raw_syscall(SYS_getgid, 0, 0, 0, 0);
    if (ret != getgid()) {
        fail("getgid");
    }
    ret = raw_syscall(SYS_getegid, 0, 0, 0, 0);
    if (ret != getegid()) {
        fail("getegid");
    }
    ret = raw_syscall(SYS_gettid, 0, 0, 0, 0);
    if (ret != (long)syscall(SYS_gettid)) {
        fail("gettid");
    }
    if (raw_syscall(SYS_clock_gettime, CLOCK_MONOTONIC,
                    (long)&ts, 0, 0) != 0) {
        fail("clock_gettime");
    }
    if (raw_syscall(SYS_gettimeofday, (long)&tv, 0, 0, 0) != 0) {
        fail("gettimeofday");
    }
    if (raw_syscall(SYS_sched_yield, 0, 0, 0, 0) != 0) {
        fail("sched_yield");
    }

    ret = raw_syscall(SYS_readlinkat, AT_FDCWD, (long)"/proc/self/exe",
                      (long)path, sizeof(path));
    if (ret <= 0 || ret >= (long)sizeof(path)) {
        fail("readlinkat");
    }
    path[ret] = '\0';

    fd = open("/proc/self", O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        fail("open");
    }
    if (raw_syscall(SYS_lseek, fd, 0, SEEK_SET, 0) != 0) {
        fail("lseek");
    }
    ret = raw_syscall(SYS_getdents64, fd, (long)dirents,
                      sizeof(dirents), 0);
    if (ret <= 0) {
        fail("getdents64");
    }
    if (raw_syscall(SYS_close, fd, 0, 0, 0) != 0) {
        fail("close");
    }

    puts("direct-syscall-ok");
    return 0;
}
