# WI-1941 AI usage

- Read the six `la_vexth_*` generator call sites and existing IR generator
  function-pointer declarations.
- Reproduced the compile failure with the serial WI-1940 build.
- Applied only the return-type correction and two unreachable wrapper returns.
- No other AVX instruction, fixture, LASX path, or compiler warning policy was
  changed.
