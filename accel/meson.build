specific_ss.add(files('accel-common.c'))
user_ss.add(files('accel-user.c'))

subdir('qtest')
subdir('tcg')
subdir('stubs')

dummy_ss = ss.source_set()
dummy_ss.add(files(
  'dummy-cpus.c',
))
