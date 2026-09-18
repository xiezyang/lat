i386_ss = ss.source_set()
i386_ss.add(files(
  'cpu.c',
  'gdbstub.c',
  'helper.c',
  'xsave_helper.c',
  'cpu-dump.c',
))
i386_ss.add(when: 'CONFIG_SEV', if_true: files('sev.c'), if_false: files('sev-stub.c'))

subdir('tcg')
if config_host['ARCH'] in ['loongarch64', 'loongarch']
  subdir('latx')
endif

target_arch += {'i386': i386_ss}
