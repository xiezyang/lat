/* SPDX-License-Identifier: GPL-2.0-only */
/* Freestanding x86 Linux guest. Expected flags come from integer arithmetic,
 * not a second execution of the same instruction through the translator.
 */
typedef __UINT64_TYPE__ u64;
typedef __UINT32_TYPE__ u32;
typedef __UINT16_TYPE__ u16;
typedef __UINT8_TYPE__ u8;
typedef __UINTPTR_TYPE__ uptr;
#define FLAGS 0x8d5u
#define CF 1u
#define PF 4u
#define AF 16u
#define ZF 64u
#define SF 128u
#define OF 2048u

#ifdef __x86_64__
#define PUSHF "pushfq\n\t"
#define POPF "popfq\n\t"
__asm__(".global _start\n_start:\n andq $-16,%rsp\n call main\n movl %eax,%edi\n movl $60,%eax\n syscall\n");
static long sys(long n, long a, long b, long c, long d)
{
    register long r10 __asm__("r10") = d;
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10) : "rcx", "r11", "memory");
    return ret;
}
static void *mapping(int shared)
{
    register long r10 __asm__("r10") = (shared ? 1 : 2) | 32;
    register long r8 __asm__("r8") = -1;
    register long r9 __asm__("r9") = 0;
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(9), "D"(0L), "S"(8192L), "d"(3L), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return (void *)ret;
}
#define NR_FORK 57
#define NR_WAIT 61
#define NR_MPROTECT 10
#define NR_EXIT 60
#else
#define PUSHF "pushfl\n\t"
#define POPF "popfl\n\t"
__asm__(".global _start\n_start:\n andl $-16,%esp\n call main\n movl %eax,%ebx\n movl $1,%eax\n int $0x80\n");
static long sys(long n, long a, long b, long c, long d)
{
    long ret;
    (void)d;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return ret;
}
static void *mapping(int shared)
{
    unsigned long args[] = {0, 8192, 3, (shared ? 1u : 2u) | 32, (unsigned long)-1, 0};
    return (void *)sys(90, (long)args, 0, 0, 0);
}
#define NR_FORK 2
#define NR_WAIT 7
#define NR_MPROTECT 125
#define NR_EXIT 1
#endif
static void message(const char *s)
{
    long size = 0;
    while (s[size]) { size++; }
#ifdef __x86_64__
    sys(1, 1, (long)s, size, 0);
#else
    sys(4, 1, (long)s, size, 0);
#endif
}
static int active_size, active_op, active_offset, active_carry;
static u64 actual_flags, wanted_flags;
static void hex(u64 value)
{
    char text[17];
    for (int n = 0; n < 16; n++) {
        text[15-n] = "0123456789abcdef"[value & 15]; value >>= 4;
    }
    text[16] = 0; message(text);
}
static __attribute__((noreturn)) void fail(int code)
{
    message("FAIL: code="); hex(code);
    message(" bytes="); hex(active_size);
    message(" op="); hex(active_op);
    message(" offset="); hex(active_offset);
    message(" carry="); hex(active_carry);
    message(" flags="); hex(actual_flags);
    message(" expected_flags="); hex(wanted_flags); message("\n");
    sys(NR_EXIT, code, 0, 0, 0);
    __builtin_unreachable();
}
static u64 mask(int size)
{
    return size == 8 ? ~(u64)0 : ((u64)1 << (size * 8)) - 1;
}
static void put(void *ptr, int size, u64 v)
{
    volatile u8 *p = ptr;
    for (int n = 0; n < size; n++) { p[n] = v >> (n * 8); }
}
static u64 get(const void *ptr, int size)
{
    const volatile u8 *p = ptr;
    u64 value = 0;
    for (int n = 0; n < size; n++) { value |= (u64)p[n] << (n * 8); }
    return value;
}
enum { ADD, ADC, SUB, SBB, INC, DEC, NEG, NOT, AND, OR, XOR, XADD, XCHG, CMPXCHG };
static unsigned char arena[192] __attribute__((aligned(64)));

/* Set arithmetic flags to a known input. Memory operands use a register base,
 * never an SP-relative object across the temporary PUSH/POP instructions.
 */
#define INITIAL0 "push $0x202\n\t" POPF
#define INITIAL1 "push $0x203\n\t" POPF
#define EXEC_TEXT(init, text, type) \
    __asm__ volatile(init text "\n\t" PUSHF "pop %0" \
                     : "=&r"(flags), "+q"(source), "+a"(expected) \
                     : "r"(address) : "cc", "memory")
#define CASES(init, suffix, type, reg) \
    switch (op) { \
    case ADD: EXEC_TEXT(init,"lock add" suffix " %" reg "1,(%3)",type);break; \
    case ADC: EXEC_TEXT(init,"lock adc" suffix " %" reg "1,(%3)",type);break; \
    case SUB: EXEC_TEXT(init,"lock sub" suffix " %" reg "1,(%3)",type);break; \
    case SBB: EXEC_TEXT(init,"lock sbb" suffix " %" reg "1,(%3)",type);break; \
    case INC: EXEC_TEXT(init,"lock inc" suffix " (%3)",type);break; \
    case DEC: EXEC_TEXT(init,"lock dec" suffix " (%3)",type);break; \
    case NEG: EXEC_TEXT(init,"lock neg" suffix " (%3)",type);break; \
    case NOT: EXEC_TEXT(init,"lock not" suffix " (%3)",type);break; \
    case AND: EXEC_TEXT(init,"lock and" suffix " %" reg "1,(%3)",type);break; \
    case OR: EXEC_TEXT(init,"lock or" suffix " %" reg "1,(%3)",type);break; \
    case XOR: EXEC_TEXT(init,"lock xor" suffix " %" reg "1,(%3)",type);break; \
    case XADD: EXEC_TEXT(init,"lock xadd" suffix " %" reg "1,(%3)",type);break; \
    case XCHG: EXEC_TEXT(init,"xchg" suffix " %" reg "1,(%3)",type);break; \
    case CMPXCHG: EXEC_TEXT(init,"lock cmpxchg" suffix " %" reg "1,(%3)",type);break; \
    }
#define WIDTH(fn, suffix, type, reg) \
static uptr fn(int op, void *address, u64 input, int carry, u64 compare, u64 *out_src, u64 *out_acc) \
{ \
    uptr flags; \
    /* q constrains the byte source to AL/BL/CL/DL on i386. */ \
    type source = input; \
    type expected = compare; \
    if (carry) { CASES(INITIAL1,suffix,type,reg) } \
    else { CASES(INITIAL0,suffix,type,reg) } \
    *out_src = source; *out_acc = expected; return flags; \
}
WIDTH(exec_b,"b",u8,"b")
WIDTH(exec_h,"w",u16,"w")
WIDTH(exec_w,"l",u32,"k")
#ifdef __x86_64__
WIDTH(exec_d,"q",u64,"")
#endif
static uptr execute(int size,int op,void *p,u64 src,int cf,u64 compare,u64 *out_src,u64 *out_acc)
{
    switch (size) {
    case 1:return exec_b(op,p,src,cf,compare,out_src,out_acc);
    case 2:return exec_h(op,p,src,cf,compare,out_src,out_acc);
    case 4:return exec_w(op,p,src,cf,compare,out_src,out_acc);
#ifdef __x86_64__
    case 8:return exec_d(op,p,src,cf,compare,out_src,out_acc);
#endif
    }
    fail(30);
}
static u32 result_flags(u64 result,int size)
{
    u32 flags = 0;
    u8 low = result;
    unsigned ones = 0;
    for (int n = 0; n < 8; n++) { ones += (low >> n) & 1; }
    if (!(ones & 1)) { flags |= PF; }
    if (!result) { flags |= ZF; }
    if (result & ((u64)1 << (size * 8 - 1))) { flags |= SF; }
    return flags;
}
static u64 reference(int size,int op,u64 a,u64 b,int carry,u64 compare,u32 *flags,u32 *defined)
{
    u64 m = mask(size), sign = (u64)1 << (size * 8 - 1), r;
    int sub = op == SUB || op == SBB || op == DEC || op == NEG || op == CMPXCHG;
    int cin = (op == ADC || op == SBB) ? carry : 0;
    *defined = FLAGS;
    if (op == NOT || op == XCHG) {
        *flags = carry;
        return op == NOT ? (~a & m) : b;
    }
    if (op == AND || op == OR || op == XOR) {
        r = op == AND ? a & b : op == OR ? a | b : a ^ b;
        *flags = result_flags(r,size); *defined &= ~AF; return r;
    }
    if (op == NEG) { b = a; a = 0; }
    if (op == INC || op == DEC) { b = 1; }
    u64 memory_result = 0;
    if (op == CMPXCHG) { memory_result = a == compare ? b : a; b = a; a = compare; }
    r = (sub ? a - b - cin : a + b + cin) & m;
    *flags = result_flags(r,size);
    if (sub ? (a < b || (a == b && cin)) : (a > m - b || ((a + b) & m) > m - cin)) { *flags |= CF; }
    if ((a ^ b ^ r) & 16) { *flags |= AF; }
    if ((sub ? (a ^ b) : ~(a ^ b)) & (a ^ r) & sign) { *flags |= OF; }
    if (op == INC || op == DEC) { *flags = (*flags & ~CF) | carry; }
    return op == CMPXCHG ? memory_result : r;
}
static void one(int size,int op,int offset,u64 initial,u64 src,int cf,int mismatch)
{
    active_size=size; active_op=op; active_offset=offset; active_carry=cf;
    unsigned char saved[sizeof(arena)];
    for (unsigned n = 0; n < sizeof(arena); n++) { arena[n] = 0xa5; }
    initial &= mask(size); src &= mask(size);
    void *p = arena + 64 + offset;
    put(p,size,initial);
    for (unsigned n = 0; n < sizeof(arena); n++) { saved[n] = arena[n]; }
    u64 compare = (initial - mismatch) & mask(size), out_src, out_acc;
    u32 expected_flags, defined;
    u64 expected = reference(size,op,initial,src,cf,compare,&expected_flags,&defined);
    uptr flags = execute(size,op,p,src,cf,compare,&out_src,&out_acc);
    actual_flags=flags & defined; wanted_flags=expected_flags & defined;
    if (get(p,size) != expected) {
        message("memory="); hex(get(p,size));
        message(" expected_memory="); hex(expected); message("\n");
        fail(31);
    }
    if ((flags & defined) != (expected_flags & defined)) { fail(32); }
    if ((op == XADD || op == XCHG) && out_src != initial) { fail(33); }
    if (op == CMPXCHG && out_acc != (mismatch ? initial : compare)) { fail(34); }
    for (unsigned n = 0; n < sizeof(arena); n++) {
        if ((n < 64u + offset || n >= 64u + offset + size) && arena[n] != saved[n]) { fail(35); }
    }
}
static void concurrent(void)
{
    u8 *p = mapping(1);
    if ((uptr)p >= (uptr)-4095) { fail(40); }
    /* Two processes share the same deliberately unaligned counter. */
    put(p + 3,4,0);
    long child = sys(NR_FORK,0,0,0,0);
    if (child < 0) { fail(41); }
    for (int n = 0; n < 10000; n++) {
        __asm__ volatile("lock addl $1,(%0)" :: "r"(p + 3) : "cc","memory");
    }
    if (!child) { sys(NR_EXIT,0,0,0,0); __builtin_unreachable(); }
    int status = -1;
    if (sys(NR_WAIT,child,(long)&status,0,0) != child || status || get(p + 3,4) != 20000) { fail(42); }
}
static void protection(void)
{
    u8 *p = mapping(1);
    if ((uptr)p >= (uptr)-4095) { fail(43); }
    put(p + 4092,4,0x12345678);
    if (sys(NR_MPROTECT,(long)(p + 4096),4096,0,0)) { fail(44); }
    long child = sys(NR_FORK,0,0,0,0);
    if (child < 0) { fail(45); }
    if (!child) {
        __asm__ volatile("lock addl $1,(%0)" :: "r"(p + 4094) : "cc","memory");
        sys(NR_EXIT,99,0,0,0); __builtin_unreachable();
    }
    int status = -1;
    if (sys(NR_WAIT,child,(long)&status,0,0) != child || (status & 127) != 11 || get(p + 4092,4) != 0x12345678) { fail(46); }
}

static void compare8(void)
{
    for (int offset = 0; offset < 64; offset++) {
        for (int equal = 0; equal < 2; equal++) {
            for (unsigned n = 0; n < sizeof(arena); n++) { arena[n] = 0xa5; }
            void *p = arena + 64 + offset;
            u64 initial = 0x123456789abcdef0ULL, replacement = 0x23456789abcdef01ULL;
            put(p,8,initial);
            u32 lo = (u32)initial + !equal, hi = initial >> 32;
            uptr flags;
            __asm__ volatile(INITIAL0 "lock cmpxchg8b (%5)\n\t" PUSHF "pop %0"
                             : "=&r"(flags), "+a"(lo), "+d"(hi)
                             : "b"((u32)replacement), "c"((u32)(replacement >> 32)), "r"(p)
                             : "cc", "memory");
            if (!!(flags & ZF) != equal || get(p,8) != (equal ? replacement : initial) ||
                lo != (u32)initial || hi != (u32)(initial >> 32)) { fail(49); }
            for (unsigned n = 0; n < sizeof(arena); n++) {
                if ((n < 64u + offset || n >= 72u + offset) && arena[n] != 0xa5) { fail(50); }
            }
        }
    }
}

#define BIT_CASE(suffix, reg) \
    switch (op) { \
    case 0: __asm__ volatile("lock bts" suffix " %" reg "1,(%2)\n\t" PUSHF "pop %0" : "=&r"(flags) : "r"(index), "r"(p) : "cc","memory");break; \
    case 1: __asm__ volatile("lock btr" suffix " %" reg "1,(%2)\n\t" PUSHF "pop %0" : "=&r"(flags) : "r"(index), "r"(p) : "cc","memory");break; \
    case 2: __asm__ volatile("lock btc" suffix " %" reg "1,(%2)\n\t" PUSHF "pop %0" : "=&r"(flags) : "r"(index), "r"(p) : "cc","memory");break; \
    }
static void bits(int max_size)
{
    for (int size = 2; size <= max_size; size *= 2) {
        long indices[] = {-1,0,size*8-1,size*8+3};
        for (int offset = 0; offset < 64; offset++) for (int k = 0; k < 4; k++) for (int op = 0; op < 3; op++) {
            for (unsigned n = 0; n < sizeof(arena); n++) { arena[n] = 0xa5; }
            long index = indices[k];
            u8 *p = arena + 64 + offset;
            int byte = 64 + offset + (index >> 3);
            int bit = index & 7, old = (arena[byte] >> bit) & 1;
            u8 expected = op == 0 ? (0xa5 | (1 << bit)) : op == 1 ? (0xa5 & ~(1 << bit)) : (0xa5 ^ (1 << bit));
            uptr flags = 0;
            if (size == 2) { BIT_CASE("w","w") }
            else if (size == 4) { BIT_CASE("l","k") }
#ifdef __x86_64__
            else { BIT_CASE("q","") }
#endif
            if ((flags & CF) != (unsigned)old || arena[byte] != expected) { fail(51); }
            for (unsigned n = 0; n < sizeof(arena); n++) {
                if ((int)n != byte && arena[n] != 0xa5) { fail(52); }
            }
        }
    }
}
#ifdef __x86_64__
static void compare16(void)
{
    u64 *p = (void *)(arena + 64);
    p[0] = 0x123456789abcdef0ULL; p[1] = 0xfedcba9876543210ULL;
    u64 a = p[0], d = p[1], b = 7, c = 9;
    unsigned char success;
    __asm__ volatile("lock cmpxchg16b (%5);setz %0" : "=q"(success), "+a"(a), "+d"(d), "+m"(*p) : "b"(b), "r"(p), "c"(c) : "cc","memory");
    if (!success || p[0] != 7 || p[1] != 9) { fail(47); }
    a = d = 0;
    __asm__ volatile("lock cmpxchg16b (%5);setz %0" : "=q"(success), "+a"(a), "+d"(d), "+m"(*p) : "b"(b), "r"(p), "c"(c) : "cc","memory");
    if (success || a != 7 || d != 9 || p[0] != 7 || p[1] != 9) { fail(48); }
}
#endif
int main(void)
{
#ifdef __x86_64__
    int max_size = 8;
#else
    int max_size = 4;
#endif
    for (int size = 1; size <= max_size; size *= 2) {
        u64 values[] = {0,1,15,((u64)1 << (size * 8 - 1)) - 1,(u64)1 << (size * 8 - 1),mask(size)-1,mask(size)};
        for (int offset = 0; offset <= 64 - size; offset++) {
            for (int op = ADD; op <= CMPXCHG; op++) {
                for (int n = 0; n < 7; n++) {
                    for (int carry = 0; carry < 2; carry++) {
                        one(size,op,offset,values[n],values[6-n],carry,0);
                        if (op == CMPXCHG) { one(size,op,offset,values[n],values[6-n],carry,1); }
                    }
                }
            }
        }
        /* Cross-line operations still have to preserve the x86 width. */
        for (int offset = 65-size; offset < 64; offset++) {
            one(size,ADC,offset,mask(size),0,1,0);
            one(size,SBB,offset,0,0,1,0);
            one(size,NEG,offset,1,0,0,0);
            one(size,XCHG,offset,mask(size),1,0,0);
            one(size,CMPXCHG,offset,1,2,0,0);
        }
    }
    message("PASS: arithmetic, flags, neighbors and cross-line cases\n");
    compare8();
    message("PASS: CMPXCHG8B\n");
    bits(max_size);
    message("PASS: locked bit operations\n");
    concurrent();
    message("PASS: shared counter\n");
    protection();
    message("PASS: page protection\n");
#ifdef __x86_64__
    compare16();
#endif
    message("PASS: x86 unaligned atomics, flags, neighbors, cross-line, shared counter and protection\n");
    return 0;
}
