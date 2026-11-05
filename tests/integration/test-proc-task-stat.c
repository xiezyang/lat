/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static pthread_barrier_t barrier;
static void *worker(void *unused)
{
    (void)unused;
    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);
    return NULL;
}

static int check(unsigned count)
{
    struct stat st;
    struct statx sx;
    int fd = open("/proc/self/task", O_RDONLY | O_DIRECTORY);
    if (fd < 0) return 1;
    if (stat("/proc/self/task", &st) || !S_ISDIR(st.st_mode) ||
        st.st_nlink != count + 2 || fstat(fd, &st) ||
        st.st_nlink != count + 2 ||
        fstatat(fd, ".", &st, 0) || st.st_nlink != count + 2 ||
        statx(fd, ".", 0, STATX_BASIC_STATS, &sx) ||
        !S_ISDIR(sx.stx_mode) || sx.stx_nlink != count + 2 ||
        statx(fd, ".", 0, STATX_NLINK, &sx) ||
        sx.stx_nlink != count + 2) return 2;
    close(fd);
    if (stat("/proc/self/exe", &st) || !S_ISREG(st.st_mode) ||
        statx(AT_FDCWD, "/proc/self/exe", 0, STATX_BASIC_STATS, &sx) ||
        !S_ISREG(sx.stx_mode) || st.st_ino != sx.stx_ino ||
        st.st_nlink != sx.stx_nlink) return 3;
    return 0;
}

int main(void)
{
    int rc = check(1);
    if (rc) return rc;
    pthread_t thread;
    if (pthread_barrier_init(&barrier, NULL, 2) ||
        pthread_create(&thread, NULL, worker, NULL)) return 4;
    pthread_barrier_wait(&barrier);
    rc = check(2);
    pthread_barrier_wait(&barrier);
    pthread_join(thread, NULL);
    pthread_barrier_destroy(&barrier);
    if (rc) return rc;
    if ((rc = check(1))) return rc;
    puts("PASS: stat/fstat/fstatat/statx task-directory and regular-file results");
    return 0;
}
