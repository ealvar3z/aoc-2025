#include "./lib/aoc.h"
#include <inttypes.h>

typedef struct Span {
    u64 lo;
    u64 hi;
} Span; // [log, hi] inclusive range of IDs

#ifndef MAX_SPANS
#define MAX_SPANS 4096
#endif

#ifndef MAX_IDS
#define MAX_IDS 200000
#endif

// a scale = 10^k
static u64 block_scales[32];

static void
prepare_block_scales(void)
{
    block_scales[0] = 1U;
    for (u32 i = 1U; i < ARRAY_LEN(block_scales); i++) {
        if (block_scales[i - 1U] > UINT64_MAX / 10U)
            block_scales[i] = UINT64_MAX;
        else
            block_scales[i] = block_scales[i - 1U] * 10U;
    }
}

static u64
block_scale(u32 exp)
{
    if (exp >= ARRAY_LEN(block_scales))
        return UINT64_MAX;
    return block_scales[exp];
}

static int
cmp_u64(const void *lhs, const void *rhs)
{
    const u64 a = *(const u64 *)lhs;
    const u64 b = *(const u64 *)rhs;
    return (a > b) - (a < b);
}

static bool
parse_span(const char *s, Span *out)
{
    const char *p = s;
    u64 lo = 0U, hi = 0U;

    if (!s || !out)
        return false;

    if (!isdigit((unsigned char)*p))
        return false;

    while (*p && isdigit((unsigned char)*p))
        lo = lo * 10U + (u64)(*p++ - '0');

    if (*p++ != '-')
        return false;

    if (!isdigit((unsigned char)*p))
        return false;

    while (*p && isdigit((unsigned char)*p))
        hi = hi * 10U + (u64)(*p++ - '0');

    if (*p != '\0')
        return false;

    out->lo = lo;
    out->hi = hi;
    return true;
}

static size_t
load_spans(Span *spans, size_t cap)
{
    char line[4096];
    size_t n = 0U;

    while (read_line(stdin, line, sizeof line)) {
        char *p = line;
        if (is_blank_line(p))
            continue;

        while (*p) {
            char tok[256];
            size_t k = 0U;

            while (*p && (isspace((unsigned char)*p) || *p == ','))
                p++;
            if (!*p)
                break;

            while (*p && *p != ',') {
                if (k + 1U < ARRAY_LEN(tok))
                    tok[k++] = *p;
                p++;
            }
            tok[k] = '\0';
            strip_spaces(tok);
            if (!tok[0])
                continue;

            if (n >= cap) {
                fprintf(stderr, "too many spans\n");
                exit(EXIT_FAILURE);
            }

            if (!parse_span(tok, &spans[n++])) {
                fprintf(stderr, "bad span token '%s'\n", tok);
                exit(EXIT_FAILURE);
            }
        }
    }

    return n;
}

static u64
sum_pair_ids_for_blocklen(u32 len, u64 lo, u64 hi, u64 max_id)
{
    u32 e2 = 2U * len - 1U;
    u64 min2 = block_scale(e2);
    u64 base = block_scale(len);
    u64 blk_min = block_scale(len - 1U);

    if (min2 == UINT64_MAX || min2 > max_id)
        return 0U;
    if (base == UINT64_MAX || blk_min == UINT64_MAX)
        return 0U;

    u64 blk_max = base - 1U;
    u64 m = base + 1U;

    u64 x_lo = (lo + m - 1U) / m;
    u64 x_hi = hi / m;

    u64 start = x_lo > blk_min ? x_lo : blk_min;
    u64 end   = x_hi < blk_max ? x_hi : blk_max;

    if (start > end)
        return 0U;

    u64 cnt = end - start + 1U;
    u64 sumx = (start + end) * cnt / 2U;

    return m * sumx;
}

static u64
sum_pair_ids(u64 lo, u64 hi, u64 max_id)
{
    u64 sum = 0U;
    for (u32 len = 1U;; len++) {
        u32 e2 = 2U * len - 1U;
        u64 min2 = block_scale(e2);
        if (min2 == UINT64_MAX || min2 > max_id)
            break;
        sum += sum_pair_ids_for_blocklen(len, lo, hi, max_id);
    }
    return sum;
}

static void
collect_repeats_for_blocklen(u32 len,
                              u64 max_id,
                              u64 *ids,
                              size_t cap,
                              size_t *n_ptr)
{
    u64 base = block_scale(len);
    u64 blk_min = block_scale(len - 1U);
    if (base == UINT64_MAX || base == 0 || blk_min == UINT64_MAX)
        return;

    u64 blk_max = base - 1U;

    for (u32 rep = 2U;; rep++) {
        u64 pow_bt = 1U;

        if (rep == 2U) {
            if (base > UINT64_MAX / base)
                break;
            pow_bt = base * base;
        } else {
            for (u32 i = 0U; i < rep; i++) {
                if (base != 0 && pow_bt > UINT64_MAX / base) {
                    pow_bt = 0U;
                    break;
                }
                pow_bt *= base;
            }
        }

        if (pow_bt == 0U)
            break;

        u64 m = (pow_bt - 1U) / (base - 1U);
        u64 min_id = blk_min * m;
        if (min_id > max_id)
            break;

        u64 blk_cap = max_id / m;
        if (blk_cap > blk_max)
            blk_cap = blk_max;

        if (blk_min > blk_cap)
            continue;

        for (u64 x = blk_min; x <= blk_cap; x++) {
            u64 id = x * m;
            if (id > max_id)
                break;

            if (*n_ptr >= cap) {
                fprintf(stderr, "too many ids\n");
                exit(EXIT_FAILURE);
            }
            ids[*n_ptr] = id;
            (*n_ptr)++;
        }
    }
}

static void
collect_repeat_ids(u64 max_id, u64 *ids, size_t cap, size_t *out_n)
{
    size_t n = 0U;

    for (u32 len = 1U;; len++) {
        u32 e2 = 2U * len - 1U;
        u64 min2 = block_scale(e2);

        if (min2 == UINT64_MAX || min2 > max_id)
            break;

        collect_repeats_for_blocklen(len, max_id, ids, cap, &n);
    }

    *out_n = n;
}

static void
uniq_ids(u64 *ids, size_t *n_ptr)
{
    size_t n = *n_ptr;
    size_t w = 0U;

    for (size_t i = 0; i < n; i++) {
        if (w == 0 || ids[i] != ids[w - 1])
            ids[w++] = ids[i];
    }

    *n_ptr = w;
}

static void
build_psum(const u64 *vals, size_t n, u64 *psum)
{
    u64 s = 0;
    psum[0] = 0;
    for (size_t i = 0; i < n; i++) {
        s += vals[i];
        psum[i + 1] = s;
    }
}

static size_t
lb(const u64 *a, size_t n, u64 key)
{
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t m = lo + (hi - lo) / 2;
        if (a[m] < key) lo = m + 1;
        else hi = m;
    }
    return lo;
}

static size_t
ub(const u64 *a, size_t n, u64 key)
{
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t m = lo + (hi - lo) / 2;
        if (a[m] <= key) lo = m + 1;
        else hi = m;
    }
    return lo;
}

static u64
sum_repeat_ids(const u64 *ids,
               size_t n,
               const u64 *psum,
               u64 lo,
               u64 hi)
{
    size_t L = lb(ids, n, lo);
    size_t R = ub(ids, n, hi);
    if (L >= R)
        return 0U;
    return psum[R] - psum[L];
}

int
main(void)
{
    Span spans[MAX_SPANS];
    size_t span_count;
    u64 max_id = 0U;
    u64 part1 = 0U, part2 = 0U;
    u64 ids[MAX_IDS];
    size_t id_count = 0U;
    u64 psum[MAX_IDS + 1U];

    prepare_block_scales();

    span_count = load_spans(spans, MAX_SPANS);
    if (!span_count) {
        fprintf(stderr, "no spans\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < span_count; i++)
        if (spans[i].hi > max_id)
            max_id = spans[i].hi;

    for (size_t i = 0; i < span_count; i++)
        part1 += sum_pair_ids(spans[i].lo, spans[i].hi, max_id);

    collect_repeat_ids(max_id, ids, MAX_IDS, &id_count);
    qsort(ids, id_count, sizeof(u64), cmp_u64);
    uniq_ids(ids, &id_count);
    build_psum(ids, id_count, psum);

    for (size_t i = 0; i < span_count; i++)
        part2 += sum_repeat_ids(ids, id_count, psum,
                                spans[i].lo, spans[i].hi);

    printf("Part 1: %" PRIu64 "\n", part1);
    printf("Part 2: %" PRIu64 "\n", part2);
    return EXIT_SUCCESS;
}

