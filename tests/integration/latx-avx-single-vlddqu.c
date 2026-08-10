/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum {
    SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
    PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
    MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
    SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 3,
    RECORD_SIZE = 32, NORMAL_RECORDS = 16,
};

struct kernel_sigaction { void (*handler)(int, void *, void *); unsigned long flags;
    void (*restorer)(void); unsigned long mask; };
struct fault_record { int32_t signal_number; int32_t signal_code;
    uint64_t fault_offset; uint8_t xmm15[16]; uint8_t ymmh15[16]; };

extern void latx_avx_single_vlddqu_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vlddqu_fault_xmm(uint8_t *);
extern void latx_avx_single_vlddqu_fault_ymm(uint8_t *);
extern void latx_avx_single_vlddqu_rt_sigreturn(void);

static uint8_t output[NORMAL_RECORDS * RECORD_SIZE];
static volatile uintptr_t fault_base;

static inline long syscall3(long n, long a, long b, long c)
{ register long rax __asm__("rax") = n, rdi __asm__("rdi") = a,
    rsi __asm__("rsi") = b, rdx __asm__("rdx") = c;
  __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx)
                   : "rcx", "r11", "memory"); return rax; }
static inline long syscall4(long n, long a, long b, long c, long d)
{ register long rax __asm__("rax") = n, rdi __asm__("rdi") = a,
    rsi __asm__("rsi") = b, rdx __asm__("rdx") = c, r10 __asm__("r10") = d;
  __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10)
                   : "rcx", "r11", "memory"); return rax; }
static inline long syscall6(long n, long a, long b, long c, long d, long e, long f)
{ register long rax __asm__("rax") = n, rdi __asm__("rdi") = a,
    rsi __asm__("rsi") = b, rdx __asm__("rdx") = c, r10 __asm__("r10") = d,
    r8 __asm__("r8") = e, r9 __asm__("r9") = f;
  __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx),
                   "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
  return rax; }
static __attribute__((noreturn)) void exit_now(int status)
{ __asm__ volatile("syscall" : : "a"((long)SYS_EXIT), "D"((long)status)
                   : "rcx", "r11", "memory"); __builtin_unreachable(); }
static void copy_bytes(uint8_t *dest, const uint8_t *src, size_t size)
{ while (size-- != 0) *dest++ = *src++; }

static void handler(int signal_number, void *info, void *context)
{ const uint8_t *siginfo = info, *ucontext = context;
  uintptr_t address = *(const uintptr_t *)(siginfo + 16);
  uintptr_t fpstate = *(const uintptr_t *)(ucontext + 224);
  struct fault_record record = {0};
  record.signal_number = signal_number;
  record.signal_code = *(const int32_t *)(siginfo + 8);
  record.fault_offset = address - fault_base;
  if (fpstate != 0) { const uint8_t *state = (const uint8_t *)fpstate;
    copy_bytes(record.xmm15, state + 160 + 15 * 16, 16);
    copy_bytes(record.ymmh15, state + 576 + 15 * 16, 16); }
  latx_avx_single_write_all(&record, sizeof(record)); exit_now(128 + signal_number); }

static int install_signal(int signal_number)
{ struct kernel_sigaction action = { handler, SA_SIGINFO | SA_RESTORER,
    latx_avx_single_vlddqu_rt_sigreturn, 0 };
  return syscall4(SYS_RT_SIGACTION, signal_number, (long)(uintptr_t)&action, 0, 8) < 0; }
static uint8_t *map_pages(void)
{ long mapping = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return mapping < 0 ? 0 : (uint8_t *)(uintptr_t)mapping; }

int latx_avx_single_main(long argc, char **argv)
{ uint8_t *page = map_pages();
  if (page == 0) return 70;
  latx_avx_single_fill(page, PAGES * PAGE, UINT64_C(0x564c444451554341));
  if (argc == 1 || (argc == 2 && argv[1][0] == 'r')) {
    latx_avx_single_vlddqu_run(output, page);
    return latx_avx_single_write_all(output, sizeof(output)) != 0; }
  if (argc != 2 || (argv[1][0] != 'x' && argv[1][0] != 'y')) return 72;
  if (install_signal(SIGSEGV) || install_signal(SIGBUS)) return 73;
  if (syscall3(SYS_MPROTECT, (long)(uintptr_t)(page + PAGE), PAGE, PROT_NONE) < 0) return 71;
  fault_base = (uintptr_t)page;
  if (argv[1][0] == 'x') latx_avx_single_vlddqu_fault_xmm(page + PAGE - 15);
  else latx_avx_single_vlddqu_fault_ymm(page + PAGE - 31);
  return 90; }
