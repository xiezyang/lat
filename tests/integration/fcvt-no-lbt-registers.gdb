set pagination off
# Test the primitive conversion emitters independently of x87 top remapping.
# The guest deliberately reads physical MMX registers, not logical ST(0).
# Normal no-LBT startup selects SOFTFPU=2; this is a diagnostic-only override.
break tr_translate_tb
run
set {int}&option_softfpu = 0
set {int}&option_aot = 0
set {int}&option_load_aot = 0
disable 1
continue
if $_exitcode != 0
  quit 1
end
printf "PASS: generated FCVT fallback preserves scalar/vector registers and flags\n"
quit 0
