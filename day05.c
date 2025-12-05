#include "aoc.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	u64 lo;
	u64 hi;
} Range;

#ifndef MAX_RANGES
# define MAX_RANGES 4096
#endif

#ifndef MAX_IDS
# define MAX_IDS 100000
#endif

static bool
parse_range_line(const char *s, Range *out)
{
	if (s == NULL || out == NULL) {
		return false;
	}

	const char *p = s;
	char *end = NULL;

	u64 lo = strtoull(p, &end, 10);
	if (end == p || *end != '-') {
		return false;
	}

	p = end + 1;
	u64 hi = strtoull(p, &end, 10);
	if (end == p || (*end != '\0' && !isspace((unsigned char)*end))) {
		return false;
	}

	if (hi < lo) {
		u64 tmp = lo;
		lo = hi;
		hi = tmp;
	}

	out->lo = lo;
	out->hi = hi;
	return true;
}

static bool
parse_id_line(const char *s, u64 *out)
{
	if (s == NULL || out == NULL) {
		return false;
	}

	const char *p = s;
	char *end = NULL;

	while (*p != '\0' && isspace((unsigned char)*p)) {
		p++;
	}
	if (*p == '\0') {
		return false;
	}

	u64 id = strtoull(p, &end, 10);
	if (end == p) {
		return false;
	}

	*out = id;
	return true;
}

static int
cmp_range(const void *a, const void *b)
{
	const Range *ra = (const Range *)a;
	const Range *rb = (const Range *)b;

	if (ra->lo < rb->lo)
		return -1;
	if (ra->lo > rb->lo)
		return 1;
	if (ra->hi < rb->hi)
		return -1;
	if (ra->hi > rb->hi)
		return 1;
	return 0;
}

// Returns new count.
static size_t
merge_ranges(Range *r, size_t n)
{
	if (n == 0) {
		return 0;
	}

	qsort(r, n, sizeof(Range), cmp_range);

	size_t w = 0;
	Range cur = r[0];

	for (size_t i = 1; i < n; i++) {
		Range next = r[i];
		if (next.lo <= cur.hi + 1U) {
			if (next.hi > cur.hi) {
				cur.hi = next.hi;
			}
		} else {
			r[w++] = cur;
			cur = next;
		}
	}

	r[w++] = cur;
	return w;
}

// Binary search the id inside any of the merged ranges
static bool
is_fresh(u64 id, const Range *r, size_t n)
{
	size_t lo = 0U;
	size_t hi = n;

	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2U;
		u64 a = r[mid].lo;
		u64 b = r[mid].hi;

		if (id < a) {
			hi = mid;
		} else if (id > b) {
			lo = mid + 1U;
		} else {
			return true;
		}
	}

	return false;
}

int
main(void)
{
	char buf[256];
	Range ranges[MAX_RANGES];
	size_t n_ranges = 0U;

	u64 ids[MAX_IDS];
	size_t n_ids = 0U;

	while (read_line(stdin, buf, sizeof buf)) {
		if (is_blank_line(buf)) {
			break;
		}
		if (n_ranges >= MAX_RANGES) {
			fprintf(stderr, "too many ranges (cap=%d)\n",
			    MAX_RANGES);
			return EXIT_FAILURE;
		}
		Range r;
		if (!parse_range_line(buf, &r)) {
			fprintf(stderr, "bad range line: '%s'\n", buf);
			return EXIT_FAILURE;
		}
		ranges[n_ranges++] = r;
	}

	if (n_ranges == 0U) {
		fprintf(stderr, "no ranges found\n");
		return EXIT_FAILURE;
	}

	while (read_line(stdin, buf, sizeof buf)) {
		if (is_blank_line(buf)) {
			continue;
		}
		if (n_ids >= MAX_IDS) {
			fprintf(stderr, "too many IDs (cap=%d)\n", MAX_IDS);
			return EXIT_FAILURE;
		}

		u64 id;
		if (!parse_id_line(buf, &id)) {
			fprintf(stderr, "bad ID line: '%s'\n", buf);
			return EXIT_FAILURE;
		}
		ids[n_ids++] = id;
	}

	if (n_ids == 0U) {
		fprintf(stderr, "no IDs found\n");
		return EXIT_FAILURE;
	}

	size_t merged = merge_ranges(ranges, n_ranges);

	// Part1
	u64 part1 = 0U;
	for (size_t i = 0U; i < n_ids; i++) {
		if (is_fresh(ids[i], ranges, merged)) {
			part1++;
		}
	}
	// Part2
	u64 part2 = 0U;
	for (size_t i = 0U; i < merged; i++) {
		u64 lo = ranges[i].lo;
		u64 hi = ranges[i].hi;
		if (hi >= lo) {
			part2 += (hi - lo + 1U);
		}
	}

	printf("%" PRIu64 "\n", part1);
	printf("%" PRIu64 "\n", part2);
	return EXIT_SUCCESS;
}
