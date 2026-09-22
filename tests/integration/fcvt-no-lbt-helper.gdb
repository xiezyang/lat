set pagination off
break tr_translate_tb
run
set $frame = (char *)malloc(1360)
call (void)memset($frame, 0, 1360)
set {unsigned long long}($frame+256) = 0x3ff0000000000000
call ((void (*)(void *, unsigned int, unsigned int, unsigned int, unsigned int))latx_fcvt_soft)($frame, 1, 1, 0, 0)
if *(unsigned long long *)($frame+288) != 0x8000000000000000
  quit 1
end
call ((void (*)(void *, unsigned int, unsigned int, unsigned int, unsigned int))latx_fcvt_soft)($frame, 2, 2, 0, 0)
if *(unsigned long long *)($frame+320) != 0x3fff
  quit 2
end
call ((void (*)(void *, unsigned int, unsigned int, unsigned int, unsigned int))latx_fcvt_soft)($frame, 0, 3, 1, 2)
if *(unsigned long long *)($frame+352) != 0x3ff0000000000000
  quit 3
end
set {unsigned long long}($frame+288) = 0x8000000000000400
set {unsigned long long}($frame+1280) = 0
call ((void (*)(void *, unsigned int, unsigned int, unsigned int, unsigned int))latx_fcvt_soft)($frame, 0, 3, 1, 2)
if *(unsigned long long *)($frame+352) != 0x3ff0000000000000
  quit 4
end
if (*(unsigned long long *)($frame+1280) & 0x1010000) != 0x1010000
  quit 5
end
set {unsigned long long}($frame+1280) = 0x200
call ((void (*)(void *, unsigned int, unsigned int, unsigned int, unsigned int))latx_fcvt_soft)($frame, 0, 3, 1, 2)
if *(unsigned long long *)($frame+352) != 0x3ff0000000000001
  quit 6
end
printf "PASS: FCVT software helper roundtrip and directed rounding\n"
quit 0
