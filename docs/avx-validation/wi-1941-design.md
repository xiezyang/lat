# WI-1941 DESIGN: type-correct IR generator calls

The LSX vector extension generators return `IR2_INST *`, as shown by the
existing function-pointer declarations in the AVX shift and SIMD conversion
translators. `translate_vpmovx_extend_lane_lsx()` ignores that returned IR
instruction after emitting it, but its function-pointer type must still match
the generator declaration. Only the typedef return type changes; call order
and operands remain identical.

The two opcode wrapper switches already return the result of
`translate_vpmovx_lsx()` for every supported opcode. A `return false` after the
assertion makes the non-void contract explicit for compilers without treating
`lsassert(0)` as noreturn. It is unreachable for valid opcodes and does not
alter generated IR.
