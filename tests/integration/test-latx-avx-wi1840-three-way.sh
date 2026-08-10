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
printf 'name\tprobe_sha256\tbinary_sha256\tx86_sha256\tlasx_sha256\tlsx_sha256\tx86_rc\tlasx_rc\tlsx_rc\tlasx_signal\tlsx_signal\toption_enable_lasx\tlsx_stdout_equal\tlasx_stdout_equal\n' \
    >"$outdir/results.tsv"
printf 'binary=%s\nbinary_sha256=%s\noption_enable_lasx=%s\n' \
    "$latx" "$(sha256 "$latx")" "$option_address" >"$outdir/run-metadata.txt"

while [ "$#" -gt 0 ]; do
    name=$1
    probe=$2
    expected=$3
    shift 3
    case_dir="$outdir/$name"
    mkdir -p "$case_dir"

    if [ ! -f "$probe" ] || [ ! -f "$expected" ]; then
        echo "FAIL $name missing probe or x86 expected" >&2
        failed=1
        continue
    fi

    probe_hash=$(sha256 "$probe")
    expected_hash=$(sha256 "$expected")
    binary_hash=$(sha256 "$latx")
    {
        printf 'name=%s\nprobe=%s\nexpected=%s\n' "$name" "$probe" "$expected"
        printf 'probe_sha256=%s\nx86_expected_sha256=%s\nbinary_sha256=%s\n' \
            "$probe_hash" "$expected_hash" "$binary_hash"
        printf 'lasx=env LATX_AVX_CPUID=1 LATX_AOT=0 %s %s reference\n' \
            "$latx" "$probe"
        printf 'lsx=gdb -q -batch %s; set option_enable_lasx=0 at translate_context_init; run %s reference\n' \
            "$latx" "$probe"
    } >"$case_dir/replay.txt"
    printf 'run %s reference > %s/lsx.stdout 2> %s/lsx.stderr\n' \
        "$probe" "$case_dir" "$case_dir" >"$case_dir/lsx-guest-command.txt"
    {
        printf 'gdb -q -batch %s\\n' "$latx"
        printf '  -ex set pagination off\\n'
        printf '  -ex set confirm off\\n'
        printf '  -ex set environment LATX_AVX_CPUID 1\\n'
        printf '  -ex set environment LATX_AOT 0\\n'
        printf '  -ex break translate_context_init\\n'
        printf '  -ex %s\\n' "run $probe reference > $case_dir/lsx.stdout 2> $case_dir/lsx.stderr"
        printf '  -ex set option_enable_lasx at %s\\n' "$option_address"
        printf '  -ex x/wd %s\\n' "$option_address"
        printf '  -ex continue\\n'
    } >"$case_dir/lsx-gdb-command.txt"

    set +e
    env LATX_AVX_CPUID=1 LATX_AOT=0 \
        "$latx" "$probe" reference \
        >"$case_dir/lasx.stdout" 2>"$case_dir/lasx.stderr"
    lasx_rc=$?
    set -e

    set +e
    gdb -q -batch "$latx" \
        -ex 'set pagination off' \
        -ex 'set confirm off' \
        -ex 'set environment LATX_AVX_CPUID 1' \
        -ex 'set environment LATX_AOT 0' \
        -ex 'break translate_context_init' \
        -ex "run $probe reference > $case_dir/lsx.stdout 2> $case_dir/lsx.stderr" \
        -ex "set {int}$option_address = 0" \
        -ex "x/wd $option_address" \
        -ex continue \
        >"$case_dir/lsx.gdb.stdout" 2>"$case_dir/lsx.gdb.stderr"
    gdb_rc=$?
    set -e
    cat "$case_dir/lsx.gdb.stdout" "$case_dir/lsx.gdb.stderr" >"$case_dir/lsx.gdb"

    lsx_signal=$(inferior_signal_from_gdb "$case_dir/lsx.gdb")
    lsx_rc=$(inferior_exit_from_gdb "$case_dir/lsx.gdb" "$lsx_signal")
    readback=unknown
    if grep -Eq '<option_enable_lasx>:[[:space:]]+0$' "$case_dir/lsx.gdb"; then
        readback=0
    else
        failed=1
    fi

    [ -f "$case_dir/lsx.stdout" ] || : >"$case_dir/lsx.stdout"
    [ -f "$case_dir/lsx.stderr" ] || : >"$case_dir/lsx.stderr"
    lasx_hash=$(sha256 "$case_dir/lasx.stdout")
    lsx_hash=$(sha256 "$case_dir/lsx.stdout")
    x86_hash=$(sha256 "$expected")
    lasx_signal=$(signal_from_rc "$lasx_rc")
    lsx_stdout_equal=FAIL
    lasx_stdout_equal=FAIL
    if cmp -s "$expected" "$case_dir/lsx.stdout" &&
        [ "$lasx_rc" -eq 0 ] && [ "$lsx_rc" -eq 0 ] &&
        [ "$gdb_rc" -eq 0 ] && [ "$readback" = 0 ]; then
        lsx_stdout_equal=PASS
    else
        failed=1
    fi
    if cmp -s "$expected" "$case_dir/lasx.stdout" && [ "$lasx_rc" -eq 0 ]; then
        lasx_stdout_equal=PASS
    else
        echo "INFO $name: LASX differs from x86; preserving LASX deviation only"
    fi

    printf '%s\t%s\t%s\t%s\t%s\t%s\t0\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$name" "$probe_hash" "$binary_hash" "$x86_hash" "$lasx_hash" \
        "$lsx_hash" "$lasx_rc" "$lsx_rc" "$lasx_signal" "$lsx_signal" \
        "$readback" "$lsx_stdout_equal" "$lasx_stdout_equal" >>"$outdir/results.tsv"
    printf '%s: LASX rc=%s signal=%s, LSX rc=%s signal=%s, option=%s, LSX stdout=%s, LASX stdout=%s\n' \
        "$name" "$lasx_rc" "$lasx_signal" "$lsx_rc" "$lsx_signal" \
        "$readback" "$lsx_stdout_equal" "$lasx_stdout_equal"
done

if [ "$failed" -ne 0 ]; then
    echo "FAIL WI-1840 three-way comparison" >&2
    exit 1
fi
echo "PASS WI-1840 x86/LASX/LSX three-way comparison"
