#!/bin/sh
set -eu

repo_root=${1:-$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)}
latx=${2:-$repo_root/build64/latx-x86_64}
source_file=$repo_root/target/i386/latx/translator/tr-opnd-process.c

function_body()
{
    name=$1
    awk -v signature="^.* $name\\(" '
        $0 ~ signature { found = 1 }
        found { print }
        found && /^}/ { exit }
    ' "$source_file"
}

load_body=$(function_body load_u64_from_ir1_mem_exact)
store_body=$(function_body store_u64_to_ir1_mem_exact)
range_body=$(function_body check_guest_mem_range)

for required in convert_mem_to_itemp check_guest_mem_range load_u64_from_guest_addr; do
    if ! printf '%s\n' "$load_body" | grep -Fq "$required"; then
        echo "FAIL exact m64 load misses $required" >&2
        exit 1
    fi
done
for required in convert_mem_to_itemp check_guest_mem_range store_u64_to_guest_addr; do
    if ! printf '%s\n' "$store_body" | grep -Fq "$required"; then
        echo "FAIL exact m64 store misses $required" >&2
        exit 1
    fi
done
if ! printf '%s\n' "$range_body" | grep -Fq 'gen_test_page_flag_force'; then
    echo "FAIL exact range check is not forced" >&2
    exit 1
fi
if printf '%s\n%s\n' "$load_body" "$store_body" | grep -Eq '\bla_xv'; then
    echo "FAIL exact m64 helper contains LASX" >&2
    exit 1
fi

if [ ! -x "$latx" ]; then
    echo "FAIL LATX binary is not executable: $latx" >&2
    exit 2
fi
for symbol in load_u64_from_ir1_mem_exact store_u64_to_ir1_mem_exact; do
    dump=$(objdump -d --disassemble="$symbol" "$latx")
    if ! printf '%s\n' "$dump" | grep -q "<$symbol>"; then
        echo "FAIL compiled helper is missing: $symbol" >&2
        exit 1
    fi
    if printf '%s\n' "$dump" | grep -Eq '<la_xv[^>]*>'; then
        echo "FAIL compiled $symbol can call a LASX generator" >&2
        exit 1
    fi
done

echo "PASS exact m64 helpers use integer byte accesses without LASX"
