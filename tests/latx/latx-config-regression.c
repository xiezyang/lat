/*
 * SPDX-FileCopyrightText: 2021-2026 LAT Project Authors
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "qemu/osdep.h"
#include <glib.h>

#include "latx-string-utils.h"
#include "latx-options.h"
#include "latx-runtime.h"

static bool config_option_registered(const char *expected)
{
#define ENVFUN(NAME, handler) \
    if (g_str_equal(expected, #NAME)) { \
        return true; \
    }
    ENVS
#undef ENVFUN
    return false;
}

static void test_target_config_files(void)
{
#ifdef TARGET_X86_64
    g_assert_cmpstr(LATX_SYSTEM_CONFIG_FILE, ==,
                    "/etc/latx-x86_64.conf");
    g_assert_cmpstr(LATX_USER_CONFIG_FILE, ==, "latx-x86_64.conf");
#else
    g_assert_cmpstr(LATX_SYSTEM_CONFIG_FILE, ==, "/etc/latx-i386.conf");
    g_assert_cmpstr(LATX_USER_CONFIG_FILE, ==, "latx-i386.conf");
#endif
}

static void test_host_hwcap_parser(void)
{
    unsigned long value = 0;

    g_assert_true(latx_parse_host_hwcap_arg("0x1234", &value));
    g_assert_cmpuint(value, ==, 0x1234UL);
    g_assert_false(latx_parse_host_hwcap_arg("garbage", &value));
}

static void test_no_lbt_restrictions(void)
{
    options_init();
    option_enable_lbt = 1;
    option_enable_lasx = 1;
    option_tu = 1;
    option_aot = 1;
    option_load_aot = 1;
    option_jr_ra = 1;
    option_jr_ra_stack = 1;
    option_tunnel_lib = 1;
    option_fputag = 1;
    option_softfpu = 0;
#ifdef CONFIG_LATX_INSTS_PATTERN
    option_instptn = 0x3ffffff;
#endif
#ifdef CONFIG_LATX_AVX_OPT
    option_avx_cpuid = 1;
#endif
#ifdef CONFIG_LATX_KZT
    option_kzt = 1;
#endif

    latx_apply_no_lbt_restrictions();

    g_assert_true(latx_no_lbt_mode_enabled());
    g_assert_cmpint(option_enable_lbt, ==, 0);
    g_assert_cmpint(option_enable_lasx, ==, 0);
    g_assert_cmpint(option_tu, ==, 0);
    g_assert_cmpint(option_aot, ==, 0);
    g_assert_cmpint(option_load_aot, ==, 0);
    g_assert_cmpint(option_jr_ra, ==, 0);
    g_assert_cmpint(option_jr_ra_stack, ==, 0);
    g_assert_cmpint(option_tunnel_lib, ==, 0);
    g_assert_cmpint(option_fputag, ==, 0);
    g_assert_cmpint(option_softfpu, ==, 0);
#ifdef CONFIG_LATX_INSTS_PATTERN
    g_assert_cmpint(option_instptn, ==, 0);
#endif
#ifdef CONFIG_LATX_AVX_OPT
    g_assert_cmpint(option_avx_cpuid, ==, 0);
#endif
#ifdef CONFIG_LATX_KZT
    g_assert_cmpint(option_kzt, ==, 0);
#endif
}

static void test_no_lbt_host_hwcap_application(void)
{
    options_init();
#if defined(__loongarch__)
    latx_apply_host_hwcap(0);
    g_assert_true(latx_no_lbt_mode_enabled());
    g_assert_cmpint(option_enable_lbt, ==, 0);
    g_assert_cmpint(option_enable_lasx, ==, 0);
    g_assert_cmpint(option_tu, ==, 0);
    g_assert_cmpint(option_aot, ==, 0);
    g_assert_cmpint(option_load_aot, ==, 0);
    g_assert_cmpint(option_jr_ra, ==, 0);
    g_assert_cmpint(option_jr_ra_stack, ==, 0);
    g_assert_cmpint(option_tunnel_lib, ==, 0);
    g_assert_cmpint(option_softfpu, ==, 0);

    /* This release target stays in no-LBT mode even if HWCAP advertises it. */
    latx_apply_host_hwcap(~0UL);
    g_assert_true(latx_no_lbt_mode_enabled());
    g_assert_cmpint(option_enable_lbt, ==, 0);
    g_assert_cmpint(option_enable_lasx, ==, 0);
#else
    g_test_skip("host hwcap application only changes LATX feature policy on LoongArch");
#endif
}

static void test_no_lbt_helper_source_audit(void)
{
    g_autofree char *flag_header_path = NULL;
    g_autofree char *flag_header = NULL;
    g_autofree char *flag_wrap_path = NULL;
    g_autofree char *flag_wrap = NULL;
    g_autofree char *flag_source_path = NULL;
    g_autofree char *flag_source = NULL;
    g_autofree char *eflag_process_path = NULL;
    g_autofree char *eflag_process = NULL;
    g_autofree char *opnd_process_path = NULL;
    g_autofree char *opnd_process = NULL;
    g_autofree char *options_path = NULL;
    g_autofree char *options_source = NULL;
    g_autofree char *main_path = NULL;
    g_autofree char *main_source = NULL;
    g_autofree char *config_path = NULL;
    g_autofree char *config_source = NULL;
    g_autofree char *fctrl_path = NULL;
    g_autofree char *fctrl_source = NULL;
    g_autofree char *extcontext_path = NULL;
    g_autofree char *extcontext_source = NULL;
    gsize length = 0;

    flag_header_path = g_test_build_filename(G_TEST_DIST,
                                             "target", "i386", "latx",
                                             "include", "flag-lbt.h", NULL);
    g_assert_true(g_file_get_contents(flag_header_path, &flag_header,
                                      &length, NULL));
    flag_wrap_path = g_test_build_filename(G_TEST_DIST,
                                            "target", "i386", "latx",
                                            "include", "flag-lbt-wrap.h", NULL);
    g_assert_true(g_file_get_contents(flag_wrap_path, &flag_wrap,
                                      &length, NULL));
    g_assert_nonnull(strstr(flag_wrap,
                            "#define la_x86mtflag(value, mask) latx_write_eflags((value), (mask))"));
    g_assert_nonnull(strstr(flag_wrap,
                            "latx_set_eflag_condition((dest), (condition))"));

    flag_source_path =
        g_test_build_filename(G_TEST_DIST, "target", "i386", "latx",
                              "optimization", "flag-lbt.c", NULL);
    g_assert_true(g_file_get_contents(flag_source_path, &flag_source,
                                      &length, NULL));
    g_assert_nonnull(strstr(flag_source,
                            "la_bstrins_d(eflags, zero_ir2_opnd, OF_BIT_INDEX"));
    g_assert_nonnull(strstr(flag_source,
                            "latx_set_eflag_condition(*cond, COND_NO);"));
    g_assert_nonnull(strstr(flag_source,
                            "latx_read_eflags(*cond, 0x8);"));
    g_assert_null(strstr(flag_source,
                         "la_andi(eflags, eflags, (~eflags_mask) & 0xfff);"));
    g_assert_null(strstr(flag_source, "IR2_OPND clear_mask;"));
    g_assert_nonnull(strstr(flag_source, "IR2_OPND selected;"));

    eflag_process_path = g_test_build_filename(G_TEST_DIST,
                                               "target", "i386", "latx",
                                               "translator", "tr-eflag-process.c", NULL);
    g_assert_true(g_file_get_contents(eflag_process_path, &eflag_process,
                                      &length, NULL));
    g_assert_null(strstr(eflag_process, "static uint16_t usedef_to_eflags_mask"));
    g_assert_null(strstr(eflag_process, "static void latx_write_eflags"));
    g_assert_null(strstr(eflag_process, "static void latx_read_eflags"));
    g_assert_nonnull(strstr(eflag_process, "la_and(of, first, of);"));
    g_assert_null(strstr(eflag_process, "la_and(of, first, second);"));
    g_assert_nonnull(strstr(eflag_process, "la_and(of, lhs, of);"));
    g_assert_nonnull(strstr(eflag_process, "soft_flag_materialize"));
    g_assert_null(strstr(eflag_process, "IR2_OPND second = ra_alloc_itemp();"));
    g_assert_null(strstr(eflag_process, "IR2_OPND product = ra_alloc_itemp();"));
    g_assert_null(strstr(eflag_process, "IR2_OPND check = ra_alloc_itemp();"));
    g_assert_nonnull(strstr(eflag_process,
                            "ra_free_temp(tmp);\n    la_x86mtflag(zf, ZF_USEDEF_BIT);"));
    g_assert_nonnull(strstr(eflag_process,
                            "if (generate_xcomisx_eflags(src0, src1, pir1))"));

    opnd_process_path = g_test_build_filename(G_TEST_DIST,
                                              "target", "i386", "latx",
                                              "translator", "tr-opnd-process.c",
                                              NULL);
    g_assert_true(g_file_get_contents(opnd_process_path, &opnd_process,
                                      &length, NULL));
    g_assert_nonnull(strstr(opnd_process, "void latx_load_v128"));
    g_assert_nonnull(strstr(opnd_process,
                            "if (!latx_no_lbt_mode_enabled())"));
    g_assert_nonnull(strstr(opnd_process,
                            "la_vinsgr2vr_d(dest, scratch, 1);"));
    g_assert_nonnull(strstr(opnd_process,
                            "la_vpickve2gr_du(scratch, src, 1);"));
    g_assert_nonnull(strstr(opnd_process,
                            "latx_load_v128(opnd2, mem_opnd, little_disp);"));
    g_assert_nonnull(strstr(opnd_process,
                            "latx_store_v128(opnd2, mem_opnd, little_disp);"));

    options_path = g_test_build_filename(G_TEST_DIST,
                                         "target", "i386", "latx",
                                         "latx-options.c", NULL);
    g_assert_true(g_file_get_contents(options_path, &options_source,
                                      &length, NULL));
    g_assert_nonnull(strstr(options_source, "option_fputag = 0;"));

    main_path = g_test_build_filename(G_TEST_DIST,
                                      "linux-user", "main.c", NULL);
    g_assert_true(g_file_get_contents(main_path, &main_source, &length, NULL));
    g_assert_nonnull(strstr(main_source,
                            "options_set(target_argv);"));
    g_assert_nonnull(strstr(main_source,
                            "if (option_host_hwcap_override) {\n"
                            "        latx_apply_host_hwcap(option_host_hwcap);"));
    g_assert_nonnull(strstr(main_source,
                            "LATX: software state mode enabled "
                            "(LSX=1 LASX=0 LBT_X86=0)"));

    config_path = g_test_build_filename(G_TEST_DIST,
                                        "target", "i386", "latx",
                                        "latx-config.c", NULL);
    g_assert_true(g_file_get_contents(config_path, &config_source,
                                      &length, NULL));
    g_assert_nonnull(strstr(config_source,
                            "Non-TU translation still uses tu_data bookkeeping"));
    g_assert_nonnull(strstr(config_source, "tu_control_init();"));

    fctrl_path = g_test_build_filename(G_TEST_DIST,
                                       "target", "i386", "latx",
                                       "translator", "tr-fctrl.c", NULL);
    g_assert_true(g_file_get_contents(fctrl_path, &fctrl_source,
                                      &length, NULL));
    g_assert_nonnull(strstr(fctrl_source,
                            "if (option_enable_lbt) {\n        la_x86settm();\n    }"));

    extcontext_path = g_test_build_filename(G_TEST_DIST,
                                            "include", "loongarch-extcontext.h", NULL);
    g_assert_true(g_file_get_contents(extcontext_path, &extcontext_source,
                                      &length, NULL));
    g_assert_nonnull(strstr(extcontext_source,
                            "UC_LBT(_uc) ? (*(_type *)&UC_LBT(_uc)->eflags) : 0;"));
}

static void test_release_loader_prefix_config(void)
{
    g_assert_true(config_option_registered("LAT_LD_PREFIX"));
}

static void test_runtime_prefix_source(void)
{
    latx_runtime_reset();
    g_assert_cmpstr(latx_runtime_prefix_source_name(), ==, "default");

    latx_runtime_option_source_set(LATX_RUNTIME_SOURCE_SYSTEM_CONFIG);
    latx_runtime_prefix_selected();
    g_assert_cmpstr(latx_runtime_prefix_source_name(), ==,
                    "system_config");

    latx_runtime_option_source_set(LATX_RUNTIME_SOURCE_USER_CONFIG);
    latx_runtime_prefix_selected();
    g_assert_cmpstr(latx_runtime_prefix_source_name(), ==, "user_config");

    latx_runtime_option_source_set(LATX_RUNTIME_SOURCE_ENVIRONMENT);
    latx_runtime_prefix_selected();
    g_assert_cmpstr(latx_runtime_prefix_source_name(), ==, "environment");

    latx_runtime_option_source_set(LATX_RUNTIME_SOURCE_COMMAND_LINE);
    latx_runtime_prefix_selected();
    g_assert_cmpstr(latx_runtime_prefix_source_name(), ==, "command_line");

#ifdef TARGET_X86_64
    g_assert_cmpstr(latx_runtime_guest_abi(), ==, "x86_64");
#else
    g_assert_cmpstr(latx_runtime_guest_abi(), ==, "i386");
#endif

    g_assert_cmpstr(latx_runtime_loader_errno_name(ENOENT), ==, "not_found");
    g_assert_cmpstr(latx_runtime_loader_errno_name(EACCES), ==,
                    "permission_denied");
    g_assert_cmpstr(latx_runtime_loader_errno_name(EIO), ==, "io_error");
}

static void test_user_config_path(void)
{
    char expected[PATH_MAX];
    char path[PATH_MAX] = "must be cleared";
    char truncated[8] = "unclear";

    g_assert_true(latx_user_config_home_is_safe("/home/test", 1000, 1000,
                                                1000, 1000));
    g_assert_false(latx_user_config_home_is_safe("/home/test", 1000, 0,
                                                 1000, 1000));
    g_assert_false(latx_user_config_home_is_safe("/home/test", 1000, 1000,
                                                 1000, 0));
    g_assert_false(latx_user_config_home_is_safe("relative/home", 1000, 1000,
                                                 1000, 1000));

    g_assert_cmpint(snprintf(expected, sizeof(expected),
                             "/home/test/.config/%s",
                             LATX_USER_CONFIG_FILE), >, 0);
    g_assert_true(latx_user_config_path(path, sizeof(path), "/home/test",
                                       LATX_USER_CONFIG_FILE));
    g_assert_cmpstr(path, ==, expected);

    g_assert_false(latx_user_config_path(path, sizeof(path), NULL,
                                        LATX_USER_CONFIG_FILE));
    g_assert_cmpstr(path, ==, "");
    g_assert_false(latx_user_config_path(path, sizeof(path), "relative/home",
                                        LATX_USER_CONFIG_FILE));
    g_assert_cmpstr(path, ==, "");
    g_assert_false(latx_user_config_path(truncated, sizeof(truncated),
                                        "/home/test",
                                        LATX_USER_CONFIG_FILE));
    g_assert_cmpstr(truncated, ==, "");
    g_assert_false(latx_user_config_path(path, sizeof(path), "/home/test",
                                        NULL));
    g_assert_cmpstr(path, ==, "");
    g_assert_false(latx_user_config_path(NULL, 0, "/home/test",
                                        LATX_USER_CONFIG_FILE));
}

static void test_empty_config_value(void)
{
    char line[] = "LATX_CLOSE_PARALLEL=\n";
    char *name = NULL;
    char *value = NULL;

    g_assert_true(latx_option_line_init(line, &name, &value));
    g_assert_cmpstr(name, ==, "LATX_CLOSE_PARALLEL");
    g_assert_cmpstr(value, ==, "");
}

static void test_trim_empty_and_whitespace(void)
{
    char empty[] = "";
    char whitespace[] = " \t \n";

    g_assert_cmpstr(latx_trim(empty), ==, "");
    g_assert_cmpstr(latx_trim(whitespace), ==, "");
}

static void test_long_filename_is_terminated(void)
{
    char filename[512];
    char buffer[8];

    filename[0] = '/';
    filename[1] = 't';
    filename[2] = 'm';
    filename[3] = 'p';
    filename[4] = '/';
    memset(filename + 5, 'b', sizeof(filename) - 6);
    filename[sizeof(filename) - 1] = '\0';

    memset(buffer, 'x', sizeof(buffer));
    latx_extract_filename(filename, buffer, sizeof(buffer));

    g_assert_cmpint(buffer[sizeof(buffer) - 1], ==, '\0');
    g_assert_cmpstr(buffer, ==, "bbbbbbb");
}

static void test_single_byte_filename_buffer(void)
{
    char buffer[1] = { 'x' };

    latx_extract_filename("program", buffer, sizeof(buffer));

    g_assert_cmpint(buffer[0], ==, '\0');
}

static void test_filename_path_and_extension(void)
{
    char buffer[32];

    latx_extract_filename("/usr/bin/program.exe", buffer, sizeof(buffer));

    g_assert_cmpstr(buffer, ==, "program");
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/latx/config/empty-value", test_empty_config_value);
    g_test_add_func("/latx/config/trim-empty-and-whitespace",
                    test_trim_empty_and_whitespace);
    g_test_add_func("/latx/config/long-filename-terminated",
                    test_long_filename_is_terminated);
    g_test_add_func("/latx/config/single-byte-filename-buffer",
                    test_single_byte_filename_buffer);
    g_test_add_func("/latx/config/filename-path-and-extension",
                    test_filename_path_and_extension);
    g_test_add_func("/latx/config/host-hwcap-parser",
                    test_host_hwcap_parser);
    g_test_add_func("/latx/config/no-lbt-restrictions",
                    test_no_lbt_restrictions);
    g_test_add_func("/latx/config/no-lbt-host-hwcap-application",
                    test_no_lbt_host_hwcap_application);
    g_test_add_func("/latx/config/no-lbt-helper-source-audit",
                    test_no_lbt_helper_source_audit);
    g_test_add_func("/latx/config/target-config-files",
                    test_target_config_files);
    g_test_add_func("/latx/config/release-loader-prefix",
                    test_release_loader_prefix_config);
    g_test_add_func("/latx/config/runtime-prefix-source",
                    test_runtime_prefix_source);
    g_test_add_func("/latx/config/user-config-path",
                    test_user_config_path);

    return g_test_run();
}
