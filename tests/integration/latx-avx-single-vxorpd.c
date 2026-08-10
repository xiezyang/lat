/* SPDX-License-Identifier: GPL-2.0-only */
#include "latx-avx-single-common.h"

enum { SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
       PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
       MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
       SIGBUS = 7, SIGSEGV = 11, PAGE = 4096, PAGES = 16, CASES = 84,
       OUTPUT_SIZE = CASES * 64 };

struct kernel_sigaction { void (*handler)(int, void *, void *); unsigned long flags;
    void (*restorer)(void); unsigned long mask; };
struct fault_record { int32_t signal_number; int32_t signal_code; uint64_t fault_offset;
    uint8_t xmm15[16]; uint8_t ymmh15[16]; };

extern void latx_avx_single_vxorpd_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vxorpd_fault_xmm(uint8_t *);
extern void latx_avx_single_vxorpd_fault_ymm(uint8_t *);
extern void latx_avx_single_vxorpd_fault_observe(void);
extern void latx_avx_single_rt_sigreturn(void);
extern int latx_avx_single_vxorpd_flags_preserved(void);
extern const uint8_t latx_avx_single_vxorpd_fault_ymmh15[16];

static uint8_t output[OUTPUT_SIZE];
static volatile uintptr_t fault_base;

static inline long syscall4(long n, long a, long b, long c, long d)
{ register long rax __asm__("rax") = n, rdi __asm__("rdi") = a, rsi __asm__("rsi") = b,
    rdx __asm__("rdx") = c, r10 __asm__("r10") = d;
  __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10) : "rcx", "r11", "memory"); return rax; }
static inline long syscall6(long n, long a, long b, long c, long d, long e, long f)
{ register long rax __asm__("rax") = n, rdi __asm__("rdi") = a, rsi __asm__("rsi") = b,
    rdx __asm__("rdx") = c, r10 __asm__("r10") = d, r8 __asm__("r8") = e, r9 __asm__("r9") = f;
  __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory"); return rax; }
static __attribute__((noreturn)) void exit_now(int status)
{ __asm__ volatile("syscall" : : "a"((long)SYS_EXIT), "D"((long)status) : "rcx", "r11", "memory"); __builtin_unreachable(); }
static void copy(uint8_t *d, const uint8_t *s, size_t n) { while (n--) *d++ = *s++; }
static int eq(const char *a, const char *b) { while (*a && *a == *b) { ++a; ++b; } return *a == *b; }

static void handler(int sig, void *info, void *ctx)
{ const uint8_t *i = info, *c = ctx; uintptr_t addr = *(const uintptr_t *)(i + 16);
  uintptr_t fpaddr = *(const uintptr_t *)(c + 224); struct fault_record r = {0};
  latx_avx_single_vxorpd_fault_observe(); r.signal_number = sig; r.signal_code = *(const int32_t *)(i + 8);
  r.fault_offset = addr - fault_base;
  if (fpaddr) { const uint8_t *fp = (const uint8_t *)fpaddr; copy(r.xmm15, fp + 160 + 15 * 16, 16); }
  /* LATX's host sigframe has no guest YMM shadow; trace verifies this canary. */
  copy(r.ymmh15, latx_avx_single_vxorpd_fault_ymmh15, sizeof(r.ymmh15));
  latx_avx_single_write_all(&r, sizeof(r)); exit_now(128 + sig); }
static int install(int sig)
{ struct kernel_sigaction a = { handler, SA_SIGINFO | SA_RESTORER, latx_avx_single_rt_sigreturn, 0 };
  return syscall4(SYS_RT_SIGACTION, sig, (long)(uintptr_t)&a, 0, 8) < 0; }
static void fill_pattern(uint8_t *d, int p)
{ size_t i; for (i = 0; i < 32; ++i) switch (p) {
  case 0: d[i] = 0; break; case 1: d[i] = 0xff; break;
  case 2: d[i] = (i % 8 == 7) ? 0x80 : 0; break; case 3: d[i] = (uint8_t)i; break;
  case 4: d[i] = (i & 1) ? 0xaa : 0x55; break; default: d[i] = (uint8_t)(0x5d ^ (i * 37)); break; } }
static void prepare(uint8_t *p)
{ latx_avx_single_fill(p, PAGES * PAGE, UINT64_C(0x9bd4e7815260ac3f)); fill_pattern(p, 0);
  fill_pattern(p + 8192 + 13, 1); fill_pattern(p + 16384 + 120, 2); fill_pattern(p + 25600, 3);
  fill_pattern(p + 32768 + 1, 4); fill_pattern(p + 40960 + 4080, 5); fill_pattern(p + 49152 + 4080, 5); }
static int fault_case(const char *n, uint8_t *p)
{ uint8_t *a; void (*op)(uint8_t *); if (eq(n, "xmm-cross-1")) { a = p + 4081; op = latx_avx_single_vxorpd_fault_xmm; }
  else if (eq(n, "xmm-cross-15")) { a = p + 4095; op = latx_avx_single_vxorpd_fault_xmm; }
  else if (eq(n, "ymm-cross-1")) { a = p + 4065; op = latx_avx_single_vxorpd_fault_ymm; }
  else if (eq(n, "ymm-cross-15")) { a = p + 4079; op = latx_avx_single_vxorpd_fault_ymm; }
  else if (eq(n, "ymm-cross-16")) { a = p + 4080; op = latx_avx_single_vxorpd_fault_ymm; }
  else if (eq(n, "ymm-cross-31")) { a = p + 4095; op = latx_avx_single_vxorpd_fault_ymm; } else return 74;
  fault_base = (uintptr_t)p; op(a); return 90; }

int latx_avx_single_main(long argc, char **argv)
{ long map = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); uint8_t *p;
  if (map < 0) return 70;
  p = (uint8_t *)(uintptr_t)map;
  prepare(p);
  if (!latx_avx_single_vxorpd_flags_preserved()) return 75;
  if (argc == 1 || (argc == 2 && eq(argv[1], "reference"))) { latx_avx_single_vxorpd_run(output, p); return latx_avx_single_write_all(output, sizeof(output)) != 0; }
  if (argc != 2) return 72;
  if (eq(argv[1], "trace")) { latx_avx_single_vxorpd_run(0, p); return 0; }
  if (install(SIGSEGV) || install(SIGBUS)) return 73;
  if (latx_avx_single_syscall3(SYS_MPROTECT, (long)(uintptr_t)(p + PAGE), PAGE, PROT_NONE) < 0) return 71;
  return fault_case(argv[1], p); }
