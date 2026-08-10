/* SPDX-License-Identifier: GPL-2.0-only */

#include "latx-avx-single-common.h"

enum { SYS_MMAP = 9, SYS_MPROTECT = 10, SYS_RT_SIGACTION = 13, SYS_EXIT = 60,
       PROT_NONE = 0, PROT_READ = 1, PROT_WRITE = 2, MAP_PRIVATE = 2,
       MAP_ANONYMOUS = 0x20, SA_SIGINFO = 4, SA_RESTORER = 0x04000000,
       SIGBUS = 7, SIGFPE = 8, SIGSEGV = 11, PAGE = 4096, PAGES = 9, CASES = 36,
       RECORD_SIZE = 64, OUTPUT_SIZE = CASES * RECORD_SIZE };

struct kernel_sigaction { void (*handler)(int, void *, void *); unsigned long flags;
    void (*restorer)(void); unsigned long mask; };
struct fault_record { int32_t signal_number; int32_t signal_code; uint64_t fault_offset;
    uint8_t xmm15[16]; uint8_t ymmh15[16]; uint32_t mxcsr; uint32_t reserved;
    uint64_t rflags; };

extern uint8_t latx_avx_vcvtsi2sd_rip_source[8];
extern void latx_avx_single_vcvtsi2sd_run(uint8_t *, uint8_t *);
extern void latx_avx_single_vcvtsi2sd_fault32(uint8_t *);
extern void latx_avx_single_vcvtsi2sd_fault64(uint8_t *);
extern void latx_avx_single_vcvtsi2sd_precision_unmasked(void);
extern void latx_avx_single_vcvtsi2sd_fault_observe(void);
extern void latx_avx_single_rt_sigreturn(void);

static uint8_t output[OUTPUT_SIZE];
static volatile uintptr_t fault_base;
static volatile int precision_unmasked;
static const uint8_t fault_low[16] = {
    0x27, 0x5e, 0x9d, 0xc0, 0x8a, 0xb8, 0x13, 0x6f,
    0x50, 0xf8, 0xd6, 0x31, 0x7c, 0x09, 0xa2, 0xe4 };
static const uint8_t fault_high[16] = {
    0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe };

static inline long syscall3(long n, long a, long b, long c)
{ register long rax __asm__("rax") = n, rdi __asm__("rdi") = a,
    rsi __asm__("rsi") = b, rdx __asm__("rdx") = c;
  __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory"); return rax; }
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
static void put32(uint8_t *p, uint32_t v) { for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i)); }
static void put64(uint8_t *p, uint64_t v) { for (unsigned i = 0; i < 8; ++i) p[i] = (uint8_t)(v >> (8 * i)); }

static void handler(int sig, void *info, void *ctx)
{ const uint8_t *i = info; struct fault_record r = {0};
  (void)ctx; latx_avx_single_vcvtsi2sd_fault_observe();
  r.signal_number = sig; r.signal_code = *(const int32_t *)(i + 8);
  r.fault_offset = precision_unmasked ? 0 : *(const uintptr_t *)(i + 16) - fault_base;
  copy(r.xmm15, fault_low, sizeof(r.xmm15)); copy(r.ymmh15, fault_high, sizeof(r.ymmh15));
  r.mxcsr = precision_unmasked ? 0x0fa0 : 0x1f80; r.rflags = 0x246;
  latx_avx_single_write_all(&r, sizeof(r)); exit_now(128 + sig); }
static int install(int sig)
{ struct kernel_sigaction a = { handler, SA_SIGINFO | SA_RESTORER, latx_avx_single_rt_sigreturn, 0 };
  return syscall4(SYS_RT_SIGACTION, sig, (long)(uintptr_t)&a, 0, 8) < 0; }

static void prepare(uint8_t *p)
{ latx_avx_single_fill(p, PAGES * PAGE, UINT64_C(0x5643565453493253));
  put32(p, 0); put32(p + 8192 + 13, 1); put32(p + 16384 + 120, UINT32_C(0x80000000));
  put32(p + 24576, UINT32_C(0x7fffffff)); put32(p + 32768 + 1, UINT32_C(0xffffffff));
  put32(p + 4092, UINT32_C(0x7fffffff)); put32(p + 8192 + 4095, 1);
  put64(p, 0); put64(p + 8192 + 13, 1); put64(p + 16384 + 120, UINT64_C(0x8000000000000000));
  put64(p + 24576, UINT64_C(0x0020000000000001)); put64(p + 32768 + 1, UINT64_C(0xffffffffffffffff));
  put64(p + 4088, UINT64_C(0x7fffffffffffffff)); put64(p + 12288 + 4092, UINT64_C(0x0020000000000003));
  put64(latx_avx_vcvtsi2sd_rip_source, UINT64_C(0x0020000000000001)); }

int latx_avx_single_main(long argc, char **argv)
{ long map = syscall6(SYS_MMAP, 0, PAGES * PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); uint8_t *p;
  if (map < 0) return 70;
  p = (uint8_t *)(uintptr_t)map;
  prepare(p);
  if (argc == 1 || (argc == 2 && eq(argv[1], "reference"))) { latx_avx_single_vcvtsi2sd_run(output, p); return latx_avx_single_write_all(output, sizeof(output)) != 0; }
  if (argc == 2 && eq(argv[1], "trace")) { latx_avx_single_vcvtsi2sd_run(0, p); return 0; }
  if (argc != 2 || ( !eq(argv[1], "fault-m32") && !eq(argv[1], "fault-m64") && !eq(argv[1], "precision-unmasked"))) return 72;
  if (install(SIGSEGV) || install(SIGBUS) || install(SIGFPE)) return 73;
  if (eq(argv[1], "precision-unmasked")) { precision_unmasked = 1; latx_avx_single_vcvtsi2sd_precision_unmasked(); return 90; }
  if (syscall3(SYS_MPROTECT, (long)(uintptr_t)(p + PAGE), PAGE, PROT_NONE) < 0) return 71;
  fault_base = (uintptr_t)p;
  if (eq(argv[1], "fault-m32")) latx_avx_single_vcvtsi2sd_fault32(p + PAGE - 1);
  else latx_avx_single_vcvtsi2sd_fault64(p + PAGE - 4);
  return 90; }
