#!/bin/sh
set -u

usage()
{
    echo "usage: $0 LATX_X86_64 OUTPUT_DIR NAME PROBE X86_EXPECTED [NAME PROBE X86_EXPECTED ...]" >&2
    exit 2
}

[ "$#" -ge 5 ] || usage
[ $((($# - 2) % 3)) -eq 0 ] || usage

latx=$1
outdir=$2
shift 2

[ -x "$latx" ] || {
    echo "FAIL LATX binary is not executable: $latx" >&2
    exit 1
}
command -v gdb >/dev/null 2>&1 || {
    echo "FAIL gdb is required for the LSX run" >&2
    exit 1
}
command -v nm >/dev/null 2>&1 || {
    echo "FAIL nm is required to locate option_enable_lasx" >&2
    exit 1
}
command -v sha256sum >/dev/null 2>&1 || {
    echo "FAIL sha256sum is required" >&2
    exit 1
}

mkdir -p "$outdir"
option_address=$(nm -g "$latx" | awk '$3 == "option_enable_lasx" {
    print "0x" $1
    exit
}')
[ -n "$option_address" ] || {
    echo "FAIL option_enable_lasx symbol is missing" >&2
    exit 1
}

sha256()
{
    sha256sum "$1" | awk '{print $1}'
}

signal_from_rc()
{
    rc=$1
    if [ "$rc" -ge 128 ]; then
        printf '%s' "$((rc - 128))"
    else
        printf '0'
    fi
}

inferior_signal_from_gdb()
{
    signal_name=$(sed -n 's/.*received signal \(SIG[A-Z0-9]*\).*/\1/p' "$1" | tail -1)
    case "$signal_name" in
        SIGBUS) printf '7' ;;
        SIGSEGV) printf '11' ;;
        SIGFPE) printf '8' ;;
        SIGILL) printf '4' ;;
        SIGABRT) printf '6' ;;
        '') printf '0' ;;
        *) printf 'unknown' ;;
    esac
}

inferior_exit_from_gdb()
{
    log=$1
    signal=$2
    if [ "$signal" != 0 ] && [ "$signal" != unknown ]; then
        printf '%s' "$((128 + signal))"
        return
    fi
    code=$(sed -n 's/.*exited with code \([0-9][0-9]*\).*/\1/p' "$log" | tail -1)
    if [ -n "$code" ]; then
        printf '%s' "$code"
    elif grep -q 'exited normally' "$log"; then
        printf '0'
    else
        printf 'unknown'
    fi
}

failed=0
printf 'name\tfixture\tsource_sha256\tbinary_sha256\tx86_expected_sha256\tlasx_sha256\tlsx_sha256\tx86_exit_code\tlasx_exit_code\tlsx_inferior_exit_code\tlsx_option_enable_lasx_readback\tlasx_signal\tlsx_signal\thigh_half\n' \
    >"$outdir/results.tsv"
printf 'binary=%s\nbinary_sha256=%s\n' "$latx" "$(sha256 "$latx")" \
    >"$outdir/run-metadata.txt"

while [ "$#" -gt 0 ]; do
    name=$1
    probe=$2
    expected=$3
    shift 3
    case_dir="$outdir/$name"
    mkdir -p "$case_dir"

    [ -f "$probe" ] || {
        echo "FAIL $name probe is missing: $probe" >&2
        failed=1
        continue
    }
    [ -f "$expected" ] || {
        echo "FAIL $name x86 expected output is missing: $expected" >&2
        failed=1
        continue
    }

    source_hash=$(sha256 "$probe")
    expected_hash=$(sha256 "$expected")
    binary_hash=$(sha256 "$latx")
    {
        printf 'name=%s\nprobe=%s\nexpected=%s\n' "$name" "$probe" "$expected"
        printf 'source_sha256=%s\nx86_expected_sha256=%s\nbinary_sha256=%s\n' \
            "$source_hash" "$expected_hash" "$binary_hash"
        printf 'lasx_command=env LATX_AVX_CPUID=1 LATX_AOT=0 %s %s reference\n' \
            "$latx" "$probe"
        printf 'lsx_command=gdb -q -batch %s: set option_enable_lasx=0 before translate_context_init; run %s reference\n' \
            "$latx" "$probe"
    } >"$case_dir/commands.txt"

    set +e
    env LATX_AVX_CPUID=1 LATX_AOT=0 \
        "$latx" "$probe" reference \
        >"$case_dir/lasx.bin" 2>"$case_dir/lasx.stderr"
    lasx_rc=$?
    set -e

    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set confirm off' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex 'break translate_context_init' \
        -ex "run $probe reference > $case_dir/lsx.bin 2> $case_dir/lsx.stderr" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue \
        >"$case_dir/lsx.gdb" 2>&1
    gdb_rc=$?
    set -e

    lsx_signal=$(inferior_signal_from_gdb "$case_dir/lsx.gdb")
    lsx_rc=$(inferior_exit_from_gdb "$case_dir/lsx.gdb" "$lsx_signal")

    if grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$case_dir/lsx.gdb"; then
        readback=0
    else
        readback=unknown
        failed=1
    fi

    high_status=FAIL
    if [ "$lasx_rc" -eq 0 ] && [ "$gdb_rc" -eq 0 ] &&
        [ "$lsx_rc" -eq 0 ] && [ "$lsx_signal" -eq 0 ] &&
        [ "$readback" = 0 ]; then
        if python3 - "$expected" "$case_dir/lasx.bin" "$case_dir/lsx.bin" \
            "$case_dir/high-half.tsv" <<'PY'
import hashlib
import sys

expected_path, lasx_path, lsx_path, report_path = sys.argv[1:]
record_size = 64
high_start = 16
high_end = 32

def read(path):
    with open(path, "rb") as stream:
        return stream.read()

expected = read(expected_path)
lasx = read(lasx_path)
lsx = read(lsx_path)
if not expected or len(expected) % record_size:
    raise SystemExit("x86 expected output is empty or not 64-byte records")
if len(lasx) != len(expected) or len(lsx) != len(expected):
    raise SystemExit("LASX/LSX output size differs from x86 expected output")

rows = []
lasx_ok = True
lsx_ok = True
for index in range(0, len(expected), record_size):
    expected_high = expected[index + high_start:index + high_end]
    lasx_high = lasx[index + high_start:index + high_end]
    lsx_high = lsx[index + high_start:index + high_end]
    if lasx_high != expected_high:
        lasx_ok = False
    if lsx_high != expected_high:
        lsx_ok = False
    rows.append((index // record_size,
                 hashlib.sha256(expected_high).hexdigest(),
                 hashlib.sha256(lasx_high).hexdigest(),
                 hashlib.sha256(lsx_high).hexdigest(),
                 int(lasx_high == expected_high),
                 int(lsx_high == expected_high)))

with open(report_path, "w", encoding="ascii") as report:
    report.write("record\tx86_high_sha256\tlasx_high_sha256\tlsx_high_sha256\tlasx_matches_x86\tlsx_matches_x86\n")
    for row in rows:
        report.write("%d\t%s\t%s\t%s\t%d\t%d\n" % row)

if not lasx_ok or not lsx_ok:
    raise SystemExit("YMM high-half mismatch")
PY
        then
            high_status=PASS
        else
            failed=1
        fi
    else
        failed=1
    fi

    lasx_hash=missing
    lsx_hash=missing
    [ -f "$case_dir/lasx.bin" ] && lasx_hash=$(sha256 "$case_dir/lasx.bin")
    [ -f "$case_dir/lsx.bin" ] && lsx_hash=$(sha256 "$case_dir/lsx.bin")
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t0\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$name" "$probe" "$source_hash" "$binary_hash" "$expected_hash" \
        "$lasx_hash" "$lsx_hash" "$lasx_rc" "$lsx_rc" "$readback" \
        "$(signal_from_rc "$lasx_rc")" "$lsx_signal" \
        "$high_status" >>"$outdir/results.tsv"

    printf '%s: LASX rc=%s, LSX inferior_rc=%s signal=%s, option_enable_lasx=%s, high-half=%s\n' \
        "$name" "$lasx_rc" "$lsx_rc" "$lsx_signal" "$readback" "$high_status"
done

if [ "$failed" -ne 0 ]; then
    echo "FAIL WI-1849 YMM high-half consumer regression" >&2
    exit 1
fi
echo "PASS WI-1849 YMM high-half consumer regression"
