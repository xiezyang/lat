if 'x86_64-linux-user' in target_dirs and \
   'CONFIG_LATX_AVX_OPT' in config_host
  latx_integration_tests += [{
    'name': 'test-avx-cpu-features',
    'runner': find_program('../../test-avx-cpu-features.sh'),
    'args': [
      emulators['latx-x86_64'],
      files('../../avx-cpu-features.S'),
    ],
    'timeout': 120,
  }]
endif

if host_machine.cpu_family() == 'loongarch64'
  latx_integration_tests += [{
    'name': 'test-native-unaligned-atomic',
    'runner': find_program('../../../native/test-unaligned-atomic.py'),
    'args': [],
    'timeout': 120,
  }]
  foreach arch : ['x86_64', 'i386']
    if 'CONFIG_LATX' in config_host and arch + '-linux-user' in target_dirs
      foreach mode : ['0', '1']
        latx_integration_tests += [{
          'name': 'test-unaligned-atomic-' + arch + '-' + mode,
          'runner': find_program('../../test-unaligned-atomic.py'),
          'args': [
            emulators['latx-' + arch],
            files('../../unaligned-atomic.c'),
            arch,
            mode,
          ],
          'timeout': 240,
        }]
      endforeach
    endif
  endforeach
endif
