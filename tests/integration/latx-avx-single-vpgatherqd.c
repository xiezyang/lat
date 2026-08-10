/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"
extern void latx_avx_single_vpgatherqd_run(uint8_t *, uint8_t *);
static long syscall6(long n,long a,long b,long c,long d,long e,long f) {
    register long r10 __asm__("r10")=d, r8 __asm__("r8")=e, r9 __asm__("r9")=f, rax __asm__("rax")=n;
    register long rdi __asm__("rdi")=a, rsi __asm__("rsi")=b, rdx __asm__("rdx")=c;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi),"S"(rsi),"d"(rdx),"r"(r10),"r"(r8),"r"(r9) : "rcx","r11","memory");
    return rax;
}
static uint8_t output[128];
int latx_avx_single_main(long argc, char **argv) {
    uint8_t *fault_ptr = 0;
    if (argc > 1 && argv[1][0] == 'f') {
        long map = syscall6(9,0,8192,3,0x22,-1,0);
        if (map < 0 || syscall6(10,map+4096,4096,0,0,0,0) < 0) return 2;
        fault_ptr = (uint8_t *)(map + 4095);
    }
    latx_avx_single_vpgatherqd_run(output, fault_ptr);
    if (fault_ptr) return 1;
    return latx_avx_single_write_all(output, sizeof(output)) != 0;
}
