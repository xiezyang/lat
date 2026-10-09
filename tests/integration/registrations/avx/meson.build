if 'x86_64-linux-user' in target_dirs
  latx_integration_tests += [{
    'name': 'test-avx-single-instruction',
    'runner': find_program('../../test-avx-regressions.py'),
    'args': [
      emulators['latx-x86_64'],
      files('../../avx-single-instruction-boundaries.asm'),
      files('../../avx128-shuffle.S'),
      files('../../avx-ymm-writeback.S'),
      files('../../vpshufd-distinct-regs.S'),
      files('../../vroundps-imm0b.S'),
      files('../../avx-regression-extract-alias.S'),
      files('../../avx-regression-narrow-state.S'),
      files('../../avx-regression-fma-alias.S'),
      files('../../avx-regression-vround-alias.S'),
      files('../../avx-regression-vround-boundaries.S'),
      files('../../vcvtps2dq-alias.c'),
      files('../../vpackus-fast.c'),
      files('../../vpmulhrsw-fast.c'),
      files('../../vpshufb-fast.c'),
    ],
    'timeout': 300,
  }]

  test(
    'test-mxcsr-fcsr-model',
    find_program('python3'),
    args: files('../../../latx/test-mxcsr-fcsr.py'),
    suite: 'latx-integration',
  )
  foreach exception_option : ['0', '1']
    test(
      'test-mxcsr-exceptions-' + exception_option,
      find_program('../../test-avx-regressions.py'),
      args: [emulators['latx-x86_64'],
             files('../../../latx/mxcsr-exceptions.asm')],
      env: ['LATX_ENABLE_FCSR_EXC=' + exception_option,
            'LATX_AVX_PREBUILT='],
      suite: 'latx-integration',
      timeout: 120,
    )
  endforeach
endif
