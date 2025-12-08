#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

#ifndef MAX_PT
# define MAX_PT 4096 // maximum junction boxes
#endif

typedef struct {
	i64 x;
	i64 y;
	i64 z;
} Pt;

typedef struct {
	u64 d2; // squared distance
	int a;  // index of first point
	int b;  // index of second point
} Edge;

static int parent_[MAX_PT];
static int size_[MAX_PT];

static void
uf_init(int n)
{
	for (int i = 0; i < n; i++) {
		parent_[i] = i;
		size_[i] = 1;
	}
}

static int
uf_find(int x)
{
	int r = x;

	while (parent_[r] != r) {
		r = parent_[r];
	}
	while (parent_[x] != x) {
		int p = parent_[x];
		parent_[x] = r;
		x = p;
	}
	return r;
}

static void
uf_union_roots(int rx, int ry)
{
	if (rx == ry) {
		return;
	}
	if (size_[rx] < size_[ry]) {
		SWAP(rx, ry);
	}
	parent_[ry] = rx;
	size_[rx] += size_[ry];
}

static void
uf_union(int x, int y)
{
	int rx = uf_find(x);
	int ry = uf_find(y);
	uf_union_roots(rx, ry);
}

static inline u64
sq_euclid(const Pt *p, const Pt *q)
{
	i64 dx = p->x - q->x;
	i64 dy = p->y - q->y;
	i64 dz = p->z - q->z;

	return (u64)(dx * dx) + (u64)(dy * dy) + (u64)(dz * dz);
}

static int
edge_cmp(const void *va, const void *vb)
{
	const Edge *ea = (const Edge *)va;
	const Edge *eb = (const Edge *)vb;

	if (ea->d2 < eb->d2) {
		return -1;
	}
	if (ea->d2 > eb->d2) {
		return 1;
	}
	if (ea->a != eb->a) {
		return (ea->a < eb->a) ? -1 : 1;
	}
	if (ea->b != eb->b) {
		return (ea->b < eb->b) ? -1 : 1;
	}
	return 0;
}

// Part 1
static u64
solve_part1(const Edge *edges, size_t ecount, int n, u64 K)
{
	uf_init(n);

	u64 limit_u = (u64)ecount;
	if (limit_u > K) {
		limit_u = K;
	}
	size_t limit = (size_t)limit_u;

	for (size_t k = 0; k < limit; k++) {
		int a = edges[k].a;
		int b = edges[k].b;
		int ra = uf_find(a);
		int rb = uf_find(b);
		if (ra != rb) {
			uf_union_roots(ra, rb);
		}
	}

	u64 top1 = 0U;
	u64 top2 = 0U;
	u64 top3 = 0U;

	for (int i = 0; i < n; i++) {
		if (parent_[i] == i) {
			u64 s = (u64)size_[i];
			if (s > top1) {
				top3 = top2;
				top2 = top1;
				top1 = s;
			} else if (s > top2) {
				top3 = top2;
				top2 = s;
			} else if (s > top3) {
				top3 = s;
			}
		}
	}

	return top1 * top2 * top3;
}

// Part 2

static u64
solve_part2(const Edge *edges, size_t ecount, int n, const Pt *pts)
{
	if (n <= 1) {
		return 0U;
	}

	uf_init(n);
	int components = n;
	int last_a = -1;
	int last_b = -1;

	for (size_t k = 0; k < ecount; k++) {
		int a = edges[k].a;
		int b = edges[k].b;
		int ra = uf_find(a);
		int rb = uf_find(b);

		if (ra == rb) {
			continue;
		}

		uf_union_roots(ra, rb);
		components--;
		last_a = a;
		last_b = b;

		if (components == 1) {
			break;
		}
	}

	if (components != 1 || last_a < 0 || last_b < 0) {
		return 0U;
	}

	u64 xa = (u64)pts[last_a].x;
	u64 xb = (u64)pts[last_b].x;

	return xa * xb;
}

int
main(void)
{
	Pt pts[MAX_PT];
	int n = 0;

	char buf[256];

	while (read_line(stdin, buf, sizeof buf)) {
		if (is_blank_line(buf)) {
			continue;
		}

		i64 x = 0;
		i64 y = 0;
		i64 z = 0;

		int matched = sscanf(buf, "%lld,%lld,%lld", (long long *)&x,
		    (long long *)&y, (long long *)&z);

		if (matched != 3) {
			fprintf(stderr, "Invalid coordinate line: '%s'\n", buf);
			return EXIT_FAILURE;
		}

		if (n >= MAX_PT) {
			fprintf(stderr, "Too many points (>%d)\n", MAX_PT);
			return EXIT_FAILURE;
		}

		pts[n].x = x;
		pts[n].y = y;
		pts[n].z = z;
		n++;
	}

	if (n == 0) {
		fprintf(stderr, "No points read.\n");
		return EXIT_FAILURE;
	}

	u64 ecount_u = (u64)n * (u64)(n - 1) / 2U;
	if (ecount_u == 0U) {
		printf("Part1: 1\n");
		printf("Part2: 0\n");
		return EXIT_SUCCESS;
	}

	if (ecount_u > 100000000ULL) {
		fprintf(stderr, "Too many edges (%" PRIu64 ")\n", ecount_u);
		return EXIT_FAILURE;
	}

	size_t ecount = (size_t)ecount_u;
	Edge *edges = (Edge *)malloc(ecount * sizeof(Edge));
	if (edges == NULL) {
		fprintf(stderr, "Out of memory allocating edges.\n");
		return EXIT_FAILURE;
	}

	size_t idx = 0;
	for (int i = 0; i < n; i++) {
		for (int j = i + 1; j < n; j++) {
			edges[idx].a = i;
			edges[idx].b = j;
			edges[idx].d2 = sq_euclid(&pts[i], &pts[j]);
			idx++;
		}
	}

	if (idx != ecount) {
		fprintf(stderr, "Edge count mismatch.\n");
		free(edges);
		return EXIT_FAILURE;
	}

	qsort(edges, ecount, sizeof(Edge), edge_cmp);

	const u64 K = 1000U;

	u64 part1 = solve_part1(edges, ecount, n, K);
	u64 part2 = solve_part2(edges, ecount, n, pts);

	printf("Part1: %" PRIu64 "\n", part1);
	printf("Part2: %" PRIu64 "\n", part2);

	free(edges);
	return EXIT_SUCCESS;
}
