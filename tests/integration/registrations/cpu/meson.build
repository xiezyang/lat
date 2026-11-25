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

foreach arch : ['x86_64', 'i386']
  if arch + '-linux-user' in target_dirs
    latx_integration_tests += [{
      'name': 'test-6600-hash-' + arch,
      'runner': find_program('../../test-6600-hash.sh'),
      'args': [
        emulators['latx-' + arch],
        files('../../6600-hash.asm'),
        arch,
      ],
      'timeout': 120,
    }]
  endif
endforeach

foreach arch : ['x86_64', 'i386']
  if arch + '-linux-user' in target_dirs
    latx_integration_tests += [{
      'name': 'test-x86addi-' + arch,
      'runner': find_program('../../test-x86addi.py'),
      'args': [emulators['latx-' + arch], arch],
      'timeout': 120,
    }]
  endif
endforeach
