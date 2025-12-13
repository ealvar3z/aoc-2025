#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#ifndef MAX_SHAPES
# define MAX_SHAPES 128
#endif

#ifndef MAX_CELLS_PER_SHAPE
# define MAX_CELLS_PER_SHAPE 64
#endif

#ifndef MAX_ORI
# define MAX_ORI 16
#endif

#ifndef MAX_LINE
# define MAX_LINE 8192
#endif

static void *
xrealloc(void *p, size_t n)
{
	void *q = realloc(p, n);
	if (!q) {
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	return q;
}

typedef struct {
	int x;
	int y;
} Cell;

typedef struct {
	int n;
	Cell c[MAX_CELLS_PER_SHAPE];
	int w;
	int h;
} Poly;

typedef struct {
	int id;
	Poly base;
	int ori_n;
	Poly ori[MAX_ORI];
	int area;
} Shape;

static int
cmp_cell(const void *a, const void *b)
{
	const Cell *pa = (const Cell *)a;
	const Cell *pb = (const Cell *)b;
	if (pa->y != pb->y)
		return (pa->y < pb->y) ? -1 : 1;
	if (pa->x != pb->x)
		return (pa->x < pb->x) ? -1 : 1;
	return 0;
}

static void
poly_sort(Poly *p)
{
	if (!p || p->n <= 0)
		return;
	qsort(p->c, (size_t)p->n, sizeof(Cell), cmp_cell);
}

static void
poly_norm(Poly *p)
{
	if (!p || p->n <= 0)
		return;

	int minx = p->c[0].x, miny = p->c[0].y;
	int maxx = p->c[0].x, maxy = p->c[0].y;

	for (int i = 1; i < p->n; i++) {
		if (p->c[i].x < minx)
			minx = p->c[i].x;
		if (p->c[i].y < miny)
			miny = p->c[i].y;
		if (p->c[i].x > maxx)
			maxx = p->c[i].x;
		if (p->c[i].y > maxy)
			maxy = p->c[i].y;
	}

	for (int i = 0; i < p->n; i++) {
		p->c[i].x -= minx;
		p->c[i].y -= miny;
	}

	p->w = (maxx - minx) + 1;
	p->h = (maxy - miny) + 1;
	poly_sort(p);
}

static bool
poly_eq(const Poly *a, const Poly *b)
{
	if (!a || !b)
		return false;
	if (a->n != b->n)
		return false;
	for (int i = 0; i < a->n; i++) {
		if (a->c[i].x != b->c[i].x || a->c[i].y != b->c[i].y)
			return false;
	}
	return true;
}

static Poly
poly_rot90(const Poly *src)
{
	Poly out = {0};
	out.n = src->n;
	for (int i = 0; i < src->n; i++) {
		out.c[i].x = src->c[i].y;
		out.c[i].y = -src->c[i].x;
	}
	poly_norm(&out);
	return out;
}

static Poly
poly_flipx(const Poly *src)
{
	Poly out = {0};
	out.n = src->n;
	for (int i = 0; i < src->n; i++) {
		out.c[i].x = -src->c[i].x;
		out.c[i].y = src->c[i].y;
	}
	poly_norm(&out);
	return out;
}

static void
shape_make_oris(Shape *s)
{
	s->ori_n = 0;

	Poly p = s->base;
	poly_norm(&p);

	Poly r0 = p;
	Poly r1 = poly_rot90(&r0);
	Poly r2 = poly_rot90(&r1);
	Poly r3 = poly_rot90(&r2);

	Poly f0 = poly_flipx(&r0);
	Poly f1 = poly_rot90(&f0);
	Poly f2 = poly_rot90(&f1);
	Poly f3 = poly_rot90(&f2);

	Poly cand[8] = {r0, r1, r2, r3, f0, f1, f2, f3};

	for (int i = 0; i < 8; i++) {
		bool dup = false;
		for (int j = 0; j < s->ori_n; j++) {
			if (poly_eq(&cand[i], &s->ori[j])) {
				dup = true;
				break;
			}
		}
		if (!dup && s->ori_n < MAX_ORI) {
			s->ori[s->ori_n++] = cand[i];
		}
	}
}

static bool
is_region_line(const char *s)
{
	if (!s)
		return false;
	int w = 0, h = 0;
	int n = sscanf(s, "%dx%d", &w, &h);
	if (n == 2 && w > 0 && h > 0 && strchr(s, ':') != NULL)
		return true;
	return false;
}

static bool
parse_shape_header(const char *s, int *out_id)
{
	if (!s || !out_id)
		return false;
	int id = -1;
	char c = '\0';
	int n = sscanf(s, "%d%c", &id, &c);
	if (n == 2 && c == ':' && id >= 0) {
		*out_id = id;
		return true;
	}
	return false;
}

static bool
parse_region(const char *s, int *W, int *H, int *counts, int counts_cap,
    int *count_n)
{
	if (!s || !W || !H || !counts || !count_n)
		return false;

	int w = 0, h = 0;
	const char *colon = strchr(s, ':');
	if (!colon)
		return false;

	if (sscanf(s, "%dx%d", &w, &h) != 2)
		return false;
	if (w <= 0 || h <= 0)
		return false;

	const char *p = colon + 1;
	int n = 0;
	while (*p) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;
		if (n >= counts_cap)
			return false;
		char *end = NULL;
		long v = strtol(p, &end, 10);
		if (end == p)
			return false;
		counts[n++] = (int)v;
		p = end;
	}

	*W = w;
	*H = h;
	*count_n = n;
	return true;
}

static int
commit_shape(Shape *sh, bool *present, int *max_id, int id, const Poly *cur)
{
	if (id < 0 || id >= MAX_SHAPES) {
		fprintf(stderr, "Shape id out of range: %d\n", id);
		return -1;
	}
	if (present[id]) {
		fprintf(stderr, "Duplicate shape id: %d\n", id);
		return -1;
	}

	Shape *s = &sh[id];
	s->id = id;
	s->base = *cur;

	poly_norm(&s->base);
	s->area = s->base.n;
	shape_make_oris(s);

	present[id] = true;
	if (id > *max_id)
		*max_id = id;
	return 0;
}

static int
read_shapes_and_regions(Shape *sh, int *sh_n, char regions[][MAX_LINE],
    int *reg_n)
{
	*sh_n = 0;
	*reg_n = 0;

	char line[MAX_LINE];
	int cur_id = -1;
	Poly cur = {0};
	bool in_shape = false;
	bool present[MAX_SHAPES] = {0};
	int max_id = -1;

	while (read_line(stdin, line, sizeof line)) {
		if (is_blank_line(line)) {
			continue;
		}

		if (is_region_line(line)) {
			if (*reg_n >= 4096) {
				fprintf(stderr, "Too many regions.\n");
				return -1;
			}
			snprintf(regions[*reg_n], MAX_LINE, "%s", line);
			regions[*reg_n][MAX_LINE - 1] = '\0';
			(*reg_n)++;
			continue;
		}

		int id = -1;
		if (parse_shape_header(line, &id)) {
			if (in_shape) {
				if (commit_shape(sh, present, &max_id, cur_id,
					&cur) != 0) {
					return -1;
				}
			}
			*sh_n = max_id + 1;

			cur_id = id;
			cur.n = 0;
			cur.w = 0;
			cur.h = 0;
			in_shape = true;
			continue;
		}

		if (!in_shape) {
			fprintf(stderr,
			    "Unexpected line before any shape header: '%s'\n",
			    line);
			return -1;
		}

		int y = cur.h;
		int x = 0;
		for (const char *p = line; *p; p++) {
			if (*p == '#') {
				if (cur.n >= MAX_CELLS_PER_SHAPE) {
					fprintf(stderr,
					    "Shape too many cells.\n");
					return -1;
				}
				cur.c[cur.n].x = x;
				cur.c[cur.n].y = y;
				cur.n++;
				x++;
			} else if (*p == '.') {
				x++;
			} else if (*p == ' ' || *p == '\t') {
				// ignore
			} else {
				fprintf(stderr,
				    "Invalid char in shape grid: '%c'\n", *p);
				return -1;
			}
		}

		cur.h++;
		if (x > cur.w)
			cur.w = x;
	}

	if (in_shape) {
		if (commit_shape(sh, present, &max_id, cur_id, &cur) != 0) {
			return -1;
		}
	}

	*sh_n = max_id + 1;
	return 0;
}

static inline int
nwords_for_board(int W, int H)
{
	int bits = W * H;
	return (bits + 63) / 64;
}

static inline void
bitset_zero(uint64_t *b, int nwords)
{
	memset(b, 0, (size_t)nwords * sizeof(uint64_t));
}

static inline void
bitset_set(uint64_t *b, int idx)
{
	b[idx >> 6] |= 1ULL << (idx & 63);
}

static inline bool
mask_overlaps(const uint64_t *occ, const uint64_t *m, int nwords)
{
	for (int i = 0; i < nwords; i++) {
		if ((occ[i] & m[i]) != 0)
			return true;
	}
	return false;
}

static inline void
mask_apply(uint64_t *occ, const uint64_t *m, int nwords)
{
	for (int i = 0; i < nwords; i++)
		occ[i] |= m[i];
}

static inline void
mask_unapply(uint64_t *occ, const uint64_t *m, int nwords)
{
	// XOR is safe: this only unapply masks that were just applied and never
	// overlap.
	for (int i = 0; i < nwords; i++)
		occ[i] ^= m[i];
}

// placement list: contiguous storage (n * nwords words)
typedef struct {
	uint64_t *data; // length cap*nwords
	int n;
	int cap;
} PlaceList;

static void
placelist_free(PlaceList *pl)
{
	free(pl->data);
	pl->data = NULL;
	pl->n = pl->cap = 0;
}

static void
placelist_push(PlaceList *pl, const uint64_t *mask, int nwords)
{
	if (pl->n == pl->cap) {
		int nc = (pl->cap == 0) ? 1024 : pl->cap * 2;
		pl->data = (uint64_t *)xrealloc(pl->data,
		    (size_t)nc * (size_t)nwords * sizeof(uint64_t));
		pl->cap = nc;
	}
	memcpy(&pl->data[(size_t)pl->n * (size_t)nwords], mask,
	    (size_t)nwords * sizeof(uint64_t));
	pl->n++;
}

static void
build_placements_for_shape(PlaceList *out, const Shape *s, int W, int H,
    int nwords)
{
	// Temporary mask buffer; nwords <= 64 for all reasonable boards here.
	uint64_t tmp[64];

	for (int oi = 0; oi < s->ori_n; oi++) {
		const Poly *p = &s->ori[oi];
		if (p->w > W || p->h > H)
			continue;

		for (int y0 = 0; y0 <= H - p->h; y0++) {
			for (int x0 = 0; x0 <= W - p->w; x0++) {
				bitset_zero(tmp, nwords);
				for (int ci = 0; ci < p->n; ci++) {
					int x = x0 + p->c[ci].x;
					int y = y0 + p->c[ci].y;
					int idx = y * W + x;
					bitset_set(tmp, idx);
				}
				placelist_push(out, tmp, nwords);
			}
		}
	}
}

static void
poly_bw_counts(const Poly *p, int *b, int *w)
{
	int bb = 0, ww = 0;
	for (int i = 0; i < p->n; i++) {
		int parity = (p->c[i].x + p->c[i].y) & 1; // (0,0) black
		if (parity == 0)
			bb++;
		else
			ww++;
	}
	*b = bb;
	*w = ww;
}

// Checkerboard parity feasibility for a multiset of pieces.
// Necessary (safe) prune.
static bool
parity_prune_possible(const Shape *sh, int sh_n, int W, int H, const int *need,
    int need_n, int area_sum)
{
	int board_area = W * H;
	int Bcap = (board_area + 1) / 2;
	int Wcap = board_area / 2;

	int sumD = 0;
	for (int si = 0; si < sh_n; si++) {
		int cnt = (si < need_n) ? need[si] : 0;
		if (cnt <= 0)
			continue;

		int b0 = 0, w0 = 0;
		poly_bw_counts(&sh[si].ori[0], &b0, &w0);
		int d = b0 - w0;
		if (d < 0)
			d = -d;
		sumD += d * cnt;
		if (sumD > area_sum)
			sumD = area_sum;
	}

	int nbits = sumD + 1;
	int nwords = (nbits + 63) / 64;
	uint64_t *dp = (uint64_t *)calloc((size_t)nwords, sizeof(uint64_t));
	if (!dp)
		return true; // skip prune if OOM

	dp[0] = 1ULL;

	for (int si = 0; si < sh_n; si++) {
		int cnt = (si < need_n) ? need[si] : 0;
		if (cnt <= 0)
			continue;

		int b0 = 0, w0 = 0;
		poly_bw_counts(&sh[si].ori[0], &b0, &w0);
		int d = b0 - w0;
		if (d < 0)
			d = -d;
		if (d == 0)
			continue;

		for (int rep = 0; rep < cnt; rep++) {
			int shift = d;
			int wshift = shift / 64;
			int bshift = shift % 64;

			for (int i = nwords - 1; i >= 0; i--) {
				uint64_t v = 0;
				int src = i - wshift;
				if (src >= 0) {
					v |= dp[src] << bshift;
					if (bshift && src - 1 >= 0)
						v |= dp[src - 1] >>
						     (64 - bshift);
				}
				dp[i] |= v;
			}

			int extra = (nwords * 64) - nbits;
			if (extra > 0)
				dp[nwords - 1] &= (~0ULL) >> extra;
		}
	}

	bool ok = false;
	for (int t = 0; t <= sumD; t++) {
		if ((dp[t / 64] & (1ULL << (t % 64))) == 0)
			continue;

		int signed_imb = sumD - 2 * t;
		if (((area_sum + signed_imb) & 1) != 0)
			continue;

		int black_used = (area_sum + signed_imb) / 2;
		int white_used = area_sum - black_used;

		if (black_used < 0 || white_used < 0)
			continue;
		if (black_used <= Bcap && white_used <= Wcap) {
			ok = true;
			break;
		}
	}

	free(dp);
	return ok;
}

// DFS solver
typedef struct {
	int W, H;
	int nwords;

	const Shape *sh;
	int sh_n;

	PlaceList place[MAX_SHAPES];

	int need[MAX_SHAPES];
	int last_idx[MAX_SHAPES];
	int area[MAX_SHAPES];

	uint64_t occ[64]; // up to 64 words (supports boards up to 4096 bits
			  // comfortably)

	int remaining_area;
	int free_cells;
} Ctx;

static int
choose_next_type(Ctx *c)
{
	int best = -1;
	int best_count = INT_MAX;

	for (int t = 0; t < c->sh_n; t++) {
		if (c->need[t] <= 0)
			continue;

		PlaceList *pl = &c->place[t];
		int start = c->last_idx[t] + 1;
		if (start < 0)
			start = 0;

		int cnt = 0;
		for (int i = start; i < pl->n; i++) {
			const uint64_t *m =
			    &pl->data[(size_t)i * (size_t)c->nwords];
			if (!mask_overlaps(c->occ, m, c->nwords)) {
				cnt++;
				if (cnt >= best_count)
					break;
			}
		}

		if (cnt == 0)
			return t; // immediate dead end
		if (cnt < best_count) {
			best_count = cnt;
			best = t;
			if (best_count <= 1)
				break;
		}
	}

	return best;
}

static bool
dfs(Ctx *c)
{
	if (c->remaining_area == 0)
		return true;
	if (c->remaining_area > c->free_cells)
		return false;

	int t = choose_next_type(c);
	if (t < 0)
		return false;

	PlaceList *pl = &c->place[t];

	// If there are no placements at all, fail.
	if (pl->n == 0)
		return false;

	int saved_last = c->last_idx[t];
	int start = saved_last + 1;
	if (start < 0)
		start = 0;

	// consume one instance of type t
	c->need[t]--;
	c->remaining_area -= c->area[t];

	for (int i = start; i < pl->n; i++) {
		const uint64_t *m = &pl->data[(size_t)i * (size_t)c->nwords];
		if (mask_overlaps(c->occ, m, c->nwords))
			continue;

		c->last_idx[t] = i; // symmetry breaking: increasing indices per
				    // identical type

		mask_apply(c->occ, m, c->nwords);
		c->free_cells -= c->area[t];

		if (dfs(c))
			return true;

		c->free_cells += c->area[t];
		mask_unapply(c->occ, m, c->nwords);
	}

	// undo instance
	c->remaining_area += c->area[t];
	c->need[t]++;
	c->last_idx[t] = saved_last;

	return false;
}

static bool
solve_region_fast(const Shape *sh, int sh_n, int W, int H, const int *need_in,
    int need_n)
{
	Ctx c;
	memset(&c, 0, sizeof c);
	c.W = W;
	c.H = H;
	c.nwords = nwords_for_board(W, H);
	c.sh = sh;
	c.sh_n = sh_n;

	if (c.nwords > (int)(sizeof c.occ / sizeof c.occ[0])) {
		// unreachable??
		// Shouldn't happen for AoC input sizes; keep safe.
		return false;
	}

	int area_sum = 0;
	int min_piece_area = INT_MAX;

	for (int i = 0; i < sh_n; i++) {
		c.need[i] = (i < need_n) ? need_in[i] : 0;
		c.last_idx[i] = -1;
		c.area[i] = sh[i].area;

		if (c.need[i] < 0)
			return false;
		if (c.need[i] > 0) {
			if (sh[i].area <= 0)
				return false;
			area_sum += sh[i].area * c.need[i];
			if (sh[i].area < min_piece_area)
				min_piece_area = sh[i].area;
		}
	}

	if (area_sum == 0)
		return true;
	if (area_sum > W * H)
		return false;

	if (!parity_prune_possible(sh, sh_n, W, H, need_in, need_n, area_sum)) {
		return false;
	}

	for (int i = 0; i < sh_n; i++) {
		if (c.need[i] <= 0)
			continue;

		bool bbox_ok = false;
		for (int oi = 0; oi < sh[i].ori_n; oi++) {
			const Poly *p = &sh[i].ori[oi];
			if (p->w <= W && p->h <= H) {
				bbox_ok = true;
				break;
			}
		}
		if (!bbox_ok)
			return false;

		build_placements_for_shape(&c.place[i], &sh[i], W, H, c.nwords);

		// If a required shape has no placements in this region => fail.
		if (c.place[i].n == 0) {
			for (int k = 0; k < sh_n; k++)
				placelist_free(&c.place[k]);
			return false;
		}
	}

	bitset_zero(c.occ, c.nwords);
	c.remaining_area = area_sum;
	c.free_cells = W * H;

	bool ok = dfs(&c);

	for (int i = 0; i < sh_n; i++)
		placelist_free(&c.place[i]);
	return ok;
}

int
main(void)
{
	Shape sh[MAX_SHAPES];
	int sh_n = 0;

	static char regions[4096][MAX_LINE];
	int reg_n = 0;

	if (read_shapes_and_regions(sh, &sh_n, regions, &reg_n) != 0) {
		return EXIT_FAILURE;
	}

	if (sh_n <= 0) {
		fprintf(stderr, "No shapes parsed.\n");
		return EXIT_FAILURE;
	}

	int ok_count = 0;

	for (int ri = 0; ri < reg_n; ri++) {
		int W = 0, H = 0;
		int counts[MAX_SHAPES];
		int count_n = 0;

		if (!parse_region(regions[ri], &W, &H, counts, MAX_SHAPES,
			&count_n)) {
			fprintf(stderr, "Bad region line: '%s'\n", regions[ri]);
			return EXIT_FAILURE;
		}

		bool ok = solve_region_fast(sh, sh_n, W, H, counts, count_n);
		if (ok)
			ok_count++;
	}

	printf("%d\n", ok_count);
	return EXIT_SUCCESS;
}
