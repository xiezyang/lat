/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "qemu/osdep.h"
#include <limits.h>
#include <signal.h>
#include <sys/syscall.h>

#include "aot.h"
#include "avx-trace.h"
#include "latx-options.h"
#include "reg-alloc.h"
#include "translate.h"

#define AVX_TRACE_BUCKETS 4096
#define AVX_TRACE_CLASS_MASK 0xff
#define AVX_TRACE_ENCODING_SHIFT 8
#define AVX_TRACE_WIDTH_SHIFT 16

enum avx_trace_encoding {
    AVX_TRACE_ENCODING_NONE,
    AVX_TRACE_ENCODING_VEX,
    AVX_TRACE_ENCODING_EVEX,
    AVX_TRACE_ENCODING_XOP,
};

typedef struct AvxTraceSite {
    pid_t pid;
    uint64_t guest_pc;
    uint64_t count;
    uint64_t module_offset;
    uint64_t bytes_lo;
    uint64_t bytes_hi;
    uint32_t opcode;
    uint16_t width;
    uint8_t insn_size;
    uint8_t isa_features;
    uint8_t encoding;
    char module[PATH_MAX];
    struct AvxTraceSite *next;
} AvxTraceSite;

static pthread_once_t avx_trace_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t avx_trace_lock = PTHREAD_MUTEX_INITIALIZER;
static AvxTraceSite *avx_trace_sites[AVX_TRACE_BUCKETS];
static uint32_t avx_trace_cpuid_mask;
static uint64_t avx_trace_xgetbv_count;
static bool avx_trace_trap_issued;
static bool avx_trace_summary_emitted;
static bool avx_trace_ymm_init_issued;

#define AVX_TRACE_YMM_INIT_HIGH0 UINT64_C(0x1122334455667788)
#define AVX_TRACE_YMM_INIT_HIGH1 UINT64_C(0x99aabbccddeeff00)

static pid_t avx_trace_pid(void)
{
    return syscall(SYS_getpid);
}

static unsigned long avx_trace_tid(void)
{
    return syscall(SYS_gettid);
}

static unsigned int avx_trace_hash(pid_t pid, uint64_t guest_pc)
{
    return ((guest_pc >> 4) ^ guest_pc ^ (uint64_t)pid) &
           (AVX_TRACE_BUCKETS - 1);
}

static bool avx_trace_legacy_prefix(uint8_t byte)
{
    switch (byte) {
    case 0x26:
    case 0x2e:
    case 0x36:
    case 0x3e:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0xf0:
    case 0xf2:
    case 0xf3:
        return true;
    default:
        return false;
    }
}

static enum avx_trace_encoding avx_trace_encoding(const IR1_INST *ir1)
{
    const uint8_t *bytes = ir1->info->bytes;
    int index = 0;

    while (index < ir1->info->size && avx_trace_legacy_prefix(bytes[index])) {
        index++;
    }
    if (index >= ir1->info->size) {
        return AVX_TRACE_ENCODING_NONE;
    }
    if (bytes[index] == 0xc4 || bytes[index] == 0xc5) {
        return AVX_TRACE_ENCODING_VEX;
    }
    if (bytes[index] == 0x62) {
        return AVX_TRACE_ENCODING_EVEX;
    }
    if (bytes[index] == 0x8f && index + 1 < ir1->info->size &&
        (bytes[index + 1] & 0x1f) >= 8) {
        return AVX_TRACE_ENCODING_XOP;
    }
    return AVX_TRACE_ENCODING_NONE;
}

static uint16_t avx_trace_width(IR1_INST *ir1)
{
    uint16_t width = 0;

    for (int i = 0; i < ir1_get_opnd_num(ir1); i++) {
        int operand_width = ir1_opnd_size(ir1_get_opnd(ir1, i));
        if (operand_width > width) {
            width = operand_width;
        }
    }
    if (ir1_opcode(ir1) == dt_X86_INS_VZEROUPPER ||
        ir1_opcode(ir1) == dt_X86_INS_VZEROALL) {
        width = 256;
    }
    return width;
}

static uint8_t avx_trace_features(IR1_INST *ir1,
                                  enum avx_trace_encoding encoding)
{
    const char *name = ir1_name(ir1_opcode(ir1));
    uint8_t features = ir1->info->isa_features |
                       LATX_X86_ISA_AVX_OPT_ONLY;

    if (encoding != AVX_TRACE_ENCODING_NONE) {
        features |= LATX_X86_ISA_AVX;
    }
    if (encoding == AVX_TRACE_ENCODING_EVEX) {
        features |= LATX_X86_ISA_AVX512;
    } else if (encoding == AVX_TRACE_ENCODING_XOP) {
        features |= LATX_X86_ISA_XOP;
    }
    if (ir1_opcode(ir1) == dt_X86_INS_VCVTPH2PS ||
        ir1_opcode(ir1) == dt_X86_INS_VCVTPS2PH) {
        features |= LATX_X86_ISA_F16C;
    }
    if (name && (!strncmp(name, "vfm", 3) || !strncmp(name, "vfnm", 4))) {
        features |= encoding == AVX_TRACE_ENCODING_XOP ?
                    LATX_X86_ISA_FMA4 : LATX_X86_ISA_FMA;
    }
    return features;
}

static void avx_trace_format_features(char *buffer, size_t size,
                                      uint8_t features)
{
    size_t used = 0;

#define APPEND_FEATURE(bit, name)                                           \
    do {                                                                    \
        if (features & (bit)) {                                             \
            used += snprintf(buffer + used, size - used, "%s%s",          \
                             used ? "," : "", name);                      \
            if (used >= size) {                                             \
                buffer[size - 1] = '\0';                                   \
                return;                                                     \
            }                                                               \
        }                                                                   \
    } while (0)

    APPEND_FEATURE(LATX_X86_ISA_AVX_OPT_ONLY, "a-only");
    APPEND_FEATURE(LATX_X86_ISA_AVX, "avx");
    APPEND_FEATURE(LATX_X86_ISA_AVX2, "avx2");
    APPEND_FEATURE(LATX_X86_ISA_AVX512, "avx512");
    APPEND_FEATURE(LATX_X86_ISA_F16C, "f16c");
    APPEND_FEATURE(LATX_X86_ISA_FMA, "fma");
    APPEND_FEATURE(LATX_X86_ISA_FMA4, "fma4");
    APPEND_FEATURE(LATX_X86_ISA_XOP, "xop");
#undef APPEND_FEATURE
}

static const char *avx_trace_encoding_name(uint8_t encoding)
{
    switch (encoding) {
    case AVX_TRACE_ENCODING_VEX:
        return "vex";
    case AVX_TRACE_ENCODING_EVEX:
        return "evex";
    case AVX_TRACE_ENCODING_XOP:
        return "xop";
    case AVX_TRACE_ENCODING_NONE:
        return "legacy";
    default:
        return "unknown";
    }
}

static void avx_trace_format_bytes(char *buffer, size_t size,
                                   const AvxTraceSite *site)
{
    uint8_t bytes[16] = {0};
    size_t used = 0;

    memcpy(bytes, &site->bytes_lo, sizeof(site->bytes_lo));
    memcpy(bytes + sizeof(site->bytes_lo), &site->bytes_hi,
           sizeof(site->bytes_hi));
    for (int i = 0; i < site->insn_size && used + 2 < size; i++) {
        used += snprintf(buffer + used, size - used, "%02x", bytes[i]);
    }
}

static void avx_trace_find_module(uint64_t guest_pc, char *module,
                                  size_t module_size,
                                  uint64_t *module_offset)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    char line[PATH_MAX + 128];

    pstrcpy(module, module_size, "[unmapped]");
    *module_offset = guest_pc;
    if (!maps) {
        return;
    }

    while (fgets(line, sizeof(line), maps)) {
        unsigned long start;
        unsigned long end;
        unsigned long file_offset;
        char permissions[5];
        char path[PATH_MAX] = {0};
        int fields = sscanf(line, "%lx-%lx %4s %lx %*s %*s %4095[^\n]",
                            &start, &end, permissions, &file_offset, path);

        if (fields < 4 || guest_pc < start || guest_pc >= end) {
            continue;
        }
        char *name = path;
        while (*name == ' ') {
            name++;
        }
        pstrcpy(module, module_size, *name ? name : "[anonymous]");
        *module_offset = guest_pc - start + file_offset;
        break;
    }
    fclose(maps);
}

static void avx_trace_print_site(const char *event, const AvxTraceSite *site,
                                 unsigned long tid)
{
    char features[64] = {0};
    char bytes[33] = {0};

    avx_trace_format_features(features, sizeof(features), site->isa_features);
    avx_trace_format_bytes(bytes, sizeof(bytes), site);
    fprintf(stderr,
            "LATX_AVX_TRACE event=%s pid=%d tid=%lu pc=0x%" PRIx64
            " module=%s module_offset=0x%" PRIx64
            " count=%" PRIu64 " encoding=%s class=%s width=%u"
            " opcode=%u bytes=%s cpuid_mask=0x%x xgetbv=%" PRIu64
            "\n",
            event, site->pid, tid, site->guest_pc, site->module,
            site->module_offset, site->count,
            avx_trace_encoding_name(site->encoding), features, site->width,
            site->opcode, bytes, avx_trace_cpuid_mask,
            avx_trace_xgetbv_count);
    fflush(stderr);
}

static void avx_trace_print_ymm_state(const AvxTraceSite *site,
                                      unsigned long tid)
{
    CPUX86State *env;
    int reg = option_avx_trace_ymm;

    if (reg < 0) {
        return;
    }
    env = (CPUX86State *)lsenv->cpu_state;
    fprintf(stderr,
            "LATX_AVX_TRACE event=ymm_state pid=%d tid=%lu pc=0x%" PRIx64
            " count=%" PRIu64 " reg=%d"
            " low0=%016" PRIx64 " low1=%016" PRIx64
            " physical_high0=%016" PRIx64
            " physical_high1=%016" PRIx64
            " shadow_high0=%016" PRIx64
            " shadow_high1=%016" PRIx64 "\n",
            site->pid, tid, site->guest_pc, site->count, reg,
            env->xmm_regs[reg]._q_ZMMReg[0],
            env->xmm_regs[reg]._q_ZMMReg[1],
            env->xmm_regs[reg]._q_ZMMReg[2],
            env->xmm_regs[reg]._q_ZMMReg[3],
            env->ymmh_regs[reg]._q[0], env->ymmh_regs[reg]._q[1]);
    fflush(stderr);
}

static void avx_trace_init_ymm_state(const AvxTraceSite *site,
                                     unsigned long tid)
{
    CPUX86State *env;
    int reg = option_avx_trace_ymm;

    if (!option_avx_trace_ymm_init || avx_trace_ymm_init_issued) {
        return;
    }
    if (reg < 0) {
        fprintf(stderr,
                "LATX_AVX_TRACE_YMM_INIT requires LATX_AVX_TRACE_YMM\n");
        fflush(stderr);
        _exit(EXIT_FAILURE);
    }
    env = (CPUX86State *)lsenv->cpu_state;
    env->ymmh_regs[reg]._q[0] = AVX_TRACE_YMM_INIT_HIGH0;
    env->ymmh_regs[reg]._q[1] = AVX_TRACE_YMM_INIT_HIGH1;
    avx_trace_ymm_init_issued = true;
    fprintf(stderr,
            "LATX_AVX_TRACE event=ymm_init pid=%d tid=%lu pc=0x%" PRIx64
            " count=%" PRIu64 " reg=%d"
            " shadow_high0=%016" PRIx64
            " shadow_high1=%016" PRIx64 "\n",
            site->pid, tid, site->guest_pc, site->count, reg,
            env->ymmh_regs[reg]._q[0], env->ymmh_regs[reg]._q[1]);
    fflush(stderr);
}

void latx_avx_trace_flush(void)
{
    pid_t pid = avx_trace_pid();
    uint64_t unique = 0;
    uint64_t total = 0;

    if (!option_avx_trace) {
        return;
    }
    pthread_mutex_lock(&avx_trace_lock);
    if (avx_trace_summary_emitted) {
        pthread_mutex_unlock(&avx_trace_lock);
        return;
    }
    avx_trace_summary_emitted = true;
    for (int i = 0; i < AVX_TRACE_BUCKETS; i++) {
        for (AvxTraceSite *site = avx_trace_sites[i]; site; site = site->next) {
            if (site->pid != pid) {
                continue;
            }
            unique++;
            total += site->count;
            avx_trace_print_site("summary", site, 0);
        }
    }
    fprintf(stderr,
            "LATX_AVX_TRACE event=process_summary pid=%d unique=%" PRIu64
            " total=%" PRIu64 " cpuid_mask=0x%x xgetbv=%" PRIu64 "\n",
            pid, unique, total, avx_trace_cpuid_mask,
            avx_trace_xgetbv_count);
    fflush(stderr);
    pthread_mutex_unlock(&avx_trace_lock);
}

static void avx_trace_fork_prepare(void)
{
    pthread_mutex_lock(&avx_trace_lock);
}

static void avx_trace_fork_parent(void)
{
    pthread_mutex_unlock(&avx_trace_lock);
}

static void avx_trace_fork_child(void)
{
    memset(avx_trace_sites, 0, sizeof(avx_trace_sites));
    avx_trace_cpuid_mask = 0;
    avx_trace_xgetbv_count = 0;
    avx_trace_trap_issued = false;
    avx_trace_summary_emitted = false;
    avx_trace_ymm_init_issued = false;
    pthread_mutex_unlock(&avx_trace_lock);
}

static void avx_trace_init_once(void)
{
    atexit(latx_avx_trace_flush);
    pthread_atfork(avx_trace_fork_prepare, avx_trace_fork_parent,
                   avx_trace_fork_child);
}

void latx_avx_trace_init(void)
{
    pthread_once(&avx_trace_once, avx_trace_init_once);
}

void latx_avx_trace_hit(uint64_t guest_pc, uint64_t metadata,
                        uint64_t opcode, uint64_t bytes_lo,
                        uint64_t bytes_hi, uint64_t insn_size)
{
    pid_t pid = avx_trace_pid();
    unsigned int bucket = avx_trace_hash(pid, guest_pc);
    AvxTraceSite *site;
    bool first_site = false;
    bool trap = false;

    latx_avx_trace_init();
    pthread_mutex_lock(&avx_trace_lock);
    for (site = avx_trace_sites[bucket]; site; site = site->next) {
        if (site->pid == pid && site->guest_pc == guest_pc) {
            break;
        }
    }
    if (!site) {
        site = g_new0(AvxTraceSite, 1);
        site->pid = pid;
        site->guest_pc = guest_pc;
        site->opcode = opcode;
        site->bytes_lo = bytes_lo;
        site->bytes_hi = bytes_hi;
        site->insn_size = MIN(insn_size, 15);
        site->isa_features = metadata & AVX_TRACE_CLASS_MASK;
        site->encoding = (metadata >> AVX_TRACE_ENCODING_SHIFT) & 0xff;
        site->width = metadata >> AVX_TRACE_WIDTH_SHIFT;
        avx_trace_find_module(guest_pc, site->module, sizeof(site->module),
                              &site->module_offset);
        site->next = avx_trace_sites[bucket];
        avx_trace_sites[bucket] = site;
        first_site = true;
    }
    site->count++;
    unsigned long tid = avx_trace_tid();
    if (first_site || option_avx_trace == 3) {
        avx_trace_print_site("hit", site, tid);
    }
    if (option_avx_trace_ymm >= 0) {
        avx_trace_init_ymm_state(site, tid);
        avx_trace_print_ymm_state(site, tid);
    }
    if (option_avx_trace == 2 && !avx_trace_trap_issued) {
        avx_trace_trap_issued = true;
        trap = true;
    }
    pthread_mutex_unlock(&avx_trace_lock);

    if (trap) {
        raise(SIGILL);
        _exit(128 + SIGILL);
    }
}

void latx_avx_trace_record_cpuid(uint32_t leaf, uint32_t subleaf,
                                uint32_t eax, uint32_t ebx,
                                uint32_t ecx, uint32_t edx)
{
    uint32_t mask = 0;

    if (!option_avx_trace) {
        return;
    }
    if (leaf == 1) {
        mask = 1 << 0;
    } else if (leaf == 7 && subleaf == 0) {
        mask = 1 << 1;
    } else if (leaf == 0xd && subleaf == 0) {
        mask = 1 << 2;
    } else if (leaf == 0xd && subleaf == 1) {
        mask = 1 << 3;
    } else {
        return;
    }

    if (leaf == 1) {
        subleaf = 0;
    }

    latx_avx_trace_init();
    pthread_mutex_lock(&avx_trace_lock);
    if (!(avx_trace_cpuid_mask & mask)) {
        fprintf(stderr,
                "LATX_AVX_TRACE event=cpuid pid=%d tid=%lu leaf=0x%x"
                " subleaf=0x%x eax=0x%08x ebx=0x%08x ecx=0x%08x"
                " edx=0x%08x\n",
                avx_trace_pid(), avx_trace_tid(), leaf, subleaf,
                eax, ebx, ecx, edx);
        fflush(stderr);
    }
    avx_trace_cpuid_mask |= mask;
    pthread_mutex_unlock(&avx_trace_lock);
}

void latx_avx_trace_record_xgetbv(uint32_t index, uint64_t value,
                                 bool allowed)
{
    if (!option_avx_trace) {
        return;
    }
    latx_avx_trace_init();
    pthread_mutex_lock(&avx_trace_lock);
    avx_trace_xgetbv_count++;
    if (avx_trace_xgetbv_count == 1) {
        fprintf(stderr,
                "LATX_AVX_TRACE event=xgetbv pid=%d tid=%lu index=%u"
                " value=0x%" PRIx64 " allowed=%d\n",
                avx_trace_pid(), avx_trace_tid(), index, value, allowed);
        fflush(stderr);
    }
    pthread_mutex_unlock(&avx_trace_lock);
}

void latx_avx_trace_instrument(IR1_INST *ir1)
{
    enum avx_trace_encoding encoding;
    uint8_t features;
    uint16_t width;
    uint64_t bytes[2] = {0};
    uint64_t metadata;

    if (!option_avx_trace) {
        return;
    }
    encoding = avx_trace_encoding(ir1);

    features = avx_trace_features(ir1, encoding);
    width = avx_trace_width(ir1);
    metadata = features | ((uint64_t)encoding << AVX_TRACE_ENCODING_SHIFT) |
               ((uint64_t)width << AVX_TRACE_WIDTH_SHIFT);
    memcpy(bytes, ir1->info->bytes, MIN(ir1->info->size, sizeof(bytes)));

    tr_save_registers_to_env(0xff, 0xff, 0xff, options_to_save());
#ifdef TARGET_X86_64
    tr_save_x64_8_registers_to_env(0xff, 0xff);
#endif

    li_d(a0_ir2_opnd, ir1_addr(ir1));
    li_d(a1_ir2_opnd, metadata);
    li_d(a2_ir2_opnd, ir1_opcode(ir1));
    li_d(a3_ir2_opnd, bytes[0]);
    li_d(a4_ir2_opnd, bytes[1]);
    li_d(a5_ir2_opnd, ir1->info->size);

    IR2_OPND helper = ra_alloc_dbt_arg2();
    aot_load_host_addr(helper, (ADDR)latx_avx_trace_hit,
                       LOAD_HELPER_AVX_TRACE_HIT, 0);
    la_jirl(ra_ir2_opnd, helper, 0);

    tr_load_registers_from_env(0xff, 0xff, 0xff, options_to_save());
#ifdef TARGET_X86_64
    tr_load_x64_8_registers_from_env(0xff, 0xff);
#endif
    ra_free_all();
}
