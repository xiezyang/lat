/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>

int main(void)
{
    FILE *f = fopen("/proc/cpuinfo", "rb");
    unsigned char buffer[4096];
    size_t n;
    if (!f) return 1;
    while ((n = fread(buffer, 1, sizeof(buffer), f))) {
        if (fwrite(buffer, 1, n, stdout) != n) return 2;
    }
    int failed = ferror(f);
    fclose(f);
    return failed ? 3 : 0;
}
