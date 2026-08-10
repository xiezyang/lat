#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct result {
    uint8_t bytes[32];
    size_t length;
};

static __m128i source128(void)
{
    static const uint8_t bytes[16] = {
        0x00, 0x80, 0x01, 0x7f, 0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc,
    };
    return _mm_loadu_si128((const __m128i *)bytes);
}

static __m256i source256(void)
{
    static const uint8_t bytes[32] = {
        0x00, 0x80, 0x01, 0x7f, 0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc,
        0xde, 0xad, 0xbe, 0xef, 0x10, 0x20, 0x30, 0x40,
        0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0,
    };
    return _mm256_loadu_si256((const __m256i *)bytes);
}

static struct result vector_result(__m256i value)
{
    struct result result = { { 0 }, 32 };
    _mm256_storeu_si256((__m256i *)result.bytes, value);
    return result;
}

static struct result vector128_result(__m128i value)
{
    struct result result = { { 0 }, 16 };
    _mm_storeu_si128((__m128i *)result.bytes, value);
    return result;
}

static struct result scalar_result(uint64_t value, size_t length)
{
    struct result result = { { 0 }, length };
    memcpy(result.bytes, &value, length);
    return result;
}

static struct result run_vbroadcastf128(void)
{
    __m128 value = _mm_castsi128_ps(source128());
    return vector_result(_mm256_castps_si256(_mm256_broadcast_ps(&value)));
}

static struct result run_vbroadcasti128(void)
{
    return vector_result(_mm256_broadcastsi128_si256(source128()));
}

static struct result run_vbroadcastsd(void)
{
    double value;
    __m128i source = source128();
    memcpy(&value, &source, sizeof(value));
    return vector_result(_mm256_castpd_si256(_mm256_broadcast_sd(&value)));
}

static struct result run_vbroadcastss(void)
{
    float value;
    __m128i source = source128();
    memcpy(&value, &source, sizeof(value));
    return vector_result(_mm256_castps_si256(_mm256_broadcast_ss(&value)));
}

static struct result run_vpbroadcastb(void)
{
    __m128i source = _mm_insert_epi8(source128(), 0x5a, 0);
    return vector_result(_mm256_broadcastb_epi8(source));
}

static struct result run_vpbroadcastd(void)
{
    return vector_result(_mm256_broadcastd_epi32(source128()));
}

static struct result run_vpbroadcastq(void)
{
    return vector_result(_mm256_broadcastq_epi64(source128()));
}

static struct result run_vpbroadcastw(void)
{
    return vector_result(_mm256_broadcastw_epi16(source128()));
}

static struct result run_vextractf128(void)
{
    __m256 value = _mm256_castsi256_ps(source256());
    return vector128_result(_mm_castps_si128(_mm256_extractf128_ps(value, 1)));
}

static struct result run_vextracti128(void)
{
    return vector128_result(_mm256_extracti128_si256(source256(), 1));
}

static struct result run_vextractps(void)
{
    return scalar_result((uint32_t)_mm256_extract_epi32(source256(), 7), 4);
}

static struct result run_vinsertf128(void)
{
    __m256 value = _mm256_castsi256_ps(source256());
    __m128 insert = _mm_castsi128_ps(source128());
    return vector_result(_mm256_castps_si256(
        _mm256_insertf128_ps(value, insert, 1)));
}

static struct result run_vinserti128(void)
{
    return vector_result(_mm256_inserti128_si256(source256(), source128(), 0));
}

static struct result run_vinsertps(void)
{
    __m128 value = _mm_castsi128_ps(source128());
    __m128 insert = _mm_castsi128_ps(source128());
    return vector128_result(_mm_castps_si128(_mm_insert_ps(value, insert, 0xe1)));
}

static struct result run_vpextrb(void)
{
    return scalar_result((uint32_t)_mm_extract_epi8(source128(), 15), 1);
}

static struct result run_vpextrd(void)
{
    return scalar_result((uint32_t)_mm_extract_epi32(source128(), 3), 4);
}

static struct result run_vpextrq(void)
{
    return scalar_result((uint64_t)_mm_extract_epi64(source128(), 1), 8);
}

static struct result run_vpextrw(void)
{
    return scalar_result((uint32_t)_mm_extract_epi16(source128(), 7), 2);
}

static struct result run_vpinsrb(void)
{
    return vector128_result(_mm_insert_epi8(source128(), 0xa5, 15));
}

static struct result run_vpinsrd(void)
{
    return vector128_result(_mm_insert_epi32(source128(), 0xdeadbeef, 3));
}

static struct result run_vpinsrw(void)
{
    return vector128_result(_mm_insert_epi16(source128(), 0xbeef, 7));
}

static struct result run_case(int case_id)
{
    switch (case_id) {
    case 0: return run_vbroadcastf128();
    case 1: return run_vbroadcasti128();
    case 2: return run_vbroadcastsd();
    case 3: return run_vbroadcastss();
    case 4: return run_vpbroadcastb();
    case 5: return run_vpbroadcastd();
    case 6: return run_vpbroadcastq();
    case 7: return run_vpbroadcastw();
    case 8: return run_vextractf128();
    case 9: return run_vextracti128();
    case 10: return run_vextractps();
    case 11: return run_vinsertf128();
    case 12: return run_vinserti128();
    case 13: return run_vinsertps();
    case 14: return run_vpextrb();
    case 15: return run_vpextrd();
    case 16: return run_vpextrq();
    case 17: return run_vpextrw();
    case 18: return run_vpinsrb();
    case 19: return run_vpinsrd();
    case 20: return run_vpinsrw();
    default:
        fprintf(stderr, "invalid WI-1912 case: %d\n", case_id);
        exit(2);
    }
}

int main(void)
{
    struct result result = run_case(WI1912_CASE);

    printf("length=%zu ", result.length);
    for (size_t i = 0; i < result.length; ++i) {
        printf("%02x", result.bytes[i]);
    }
    putchar('\n');
    return 0;
}
