# WI-1941 TEST: compile blocker regression

## Static checks

```text
rg -n 'latx_vpmovx_extend_lsx_fn|la_vexth_' target/i386/latx/translator/tr-avx.c
git diff --check
```

The typedef must use `IR2_INST *`, all six `la_vexth_*` assignments must remain,
and both wrapper switches must contain the explicit fallback return.

## Build check

After the source fix, recreate `/home/xzy/github/lat/build64-todesk`, configure
with the exact WI-1940 flags and the matching local Meson SHA, then run only:

```text
ninja -C /home/xzy/github/lat/build64-todesk -j1 latx-x86_64
```

Save complete stdout/stderr/exit and do not accept an old binary. This WI does
not run the dynamic AVX matrix.
