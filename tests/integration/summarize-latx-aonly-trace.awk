#!/usr/bin/awk -f

function field(name,    i, pair) {
    for (i = 1; i <= NF; i++) {
        split($i, pair, "=")
        if (pair[1] == name) {
            return substr($i, length(name) + 2)
        }
    }
    return ""
}

/LATX_AVX_TRACE event=cpuid/ {
    cpuid_lines++
}

/LATX_AVX_TRACE event=xgetbv/ {
    xgetbv_lines++
}

/LATX_AVX_TRACE event=hit/ {
    pid = field("pid")
    pc = field("pc")
    key = pid ":" pc
    count = field("count") + 0

    hit_lines++
    if (!(key in max_count)) {
        order[++site_count] = key
        site_pid[key] = pid
        site_pc[key] = pc
        site_module[key] = field("module")
        site_offset[key] = field("module_offset")
        site_width[key] = field("width") + 0
        site_encoding[key] = field("encoding")
        site_class[key] = field("class")
        site_bytes[key] = field("bytes")
        process_seen[pid] = 1
        module_seen[site_module[key]] = 1
        if (site_class[key] !~ /(^|,)a-only(,|$)/) {
            non_a_only_sites++
        }
        if (!first_hit) {
            first_hit = key
        }
    }
    if (count > max_count[key]) {
        max_count[key] = count
    }
}

END {
    for (key in process_seen) {
        process_count++
    }
    for (key in module_seen) {
        module_count++
    }

    for (i = 1; i <= site_count; i++) {
        key = order[i]
        count = max_count[key]
        module = site_module[key]
        width = site_width[key]
        total_executions += count
        module_unique[module]++
        module_executions[module] += count
        width_unique[width]++
        width_executions[width] += count
        if (site_encoding[key] == "legacy") {
            legacy_unique++
            legacy_executions += count
        } else {
            vector_unique++
            vector_executions += count
        }
    }

    printf "SUMMARY hit_lines=%d unique_sites=%d total_executions=%d", \
           hit_lines, site_count, total_executions
    printf " processes=%d modules=%d cpuid_lines=%d xgetbv_lines=%d", \
           process_count, module_count, cpuid_lines, xgetbv_lines
    printf " non_a_only_sites=%d\n", non_a_only_sites
    printf "CLASS vector_unique=%d vector_executions=%d", \
           vector_unique, vector_executions
    printf " legacy_unique=%d legacy_executions=%d\n", \
           legacy_unique, legacy_executions

    for (module in module_unique) {
        printf "MODULE module=%s unique_sites=%d total_executions=%d\n", \
               module, module_unique[module], module_executions[module]
    }
    for (width in width_unique) {
        printf "WIDTH bits=%d unique_sites=%d total_executions=%d\n", \
               width, width_unique[width], width_executions[width]
    }

    if (first_hit) {
        printf "FIRST pid=%s pc=%s module=%s offset=%s width=%d", \
               site_pid[first_hit], site_pc[first_hit], \
               site_module[first_hit], site_offset[first_hit], \
               site_width[first_hit]
        printf " encoding=%s class=%s bytes=%s count=%d\n", \
               site_encoding[first_hit], site_class[first_hit], \
               site_bytes[first_hit], max_count[first_hit]
    }

    limit = site_count < 20 ? site_count : 20
    for (rank = 1; rank <= limit; rank++) {
        best = ""
        for (i = 1; i <= site_count; i++) {
            key = order[i]
            if (!(key in selected) &&
                (!best || max_count[key] > max_count[best])) {
                best = key
            }
        }
        selected[best] = 1
        printf "TOP rank=%d pid=%s pc=%s module=%s offset=%s count=%d", \
               rank, site_pid[best], site_pc[best], site_module[best], \
               site_offset[best], max_count[best]
        printf " width=%d encoding=%s class=%s bytes=%s\n", \
               site_width[best], site_encoding[best], site_class[best], \
               site_bytes[best]
    }
}
