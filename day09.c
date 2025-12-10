#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

#ifndef MAX_PT
#define MAX_PT 65536
#endif

typedef struct {
	i64 x;
	i64 y;
	int row;  /* grid row (1..H) after normalization */
	int col;  /* grid col (1..W) after normalization */
} Pt;

static u64
abs_diff(i64 a, i64 b)
{
	if (a >= b) {
		return (u64)(a - b);
	} else {
		return (u64)(b - a);
	}
}

static int
read_points(Pt *pts, int cap, i64 *min_x, i64 *max_x, i64 *min_y, i64 *max_y)
{
	char buf[256];
	int n = 0;
	bool first = true;

	while (read_line(stdin, buf, sizeof buf)) {
		if (is_blank_line(buf)) {
			continue;
		}

		i64 x = 0;
		i64 y = 0;

		int matched = sscanf(buf, " %lld , %lld ",
		    (long long *)&x,
		    (long long *)&y);
		if (matched != 2) {
			fprintf(stderr, "Invalid coordinate line: '%s'\n", buf);
			return -1;
		}
		if (n >= cap) {
			fprintf(stderr, "Too many points (>%d)\n", cap);
			return -1;
		}

		pts[n].x = x;
		pts[n].y = y;
		pts[n].row = 0;
		pts[n].col = 0;
		n++;

		if (first) {
			*min_x = x;
			*max_x = x;
			*min_y = y;
			*max_y = y;
			first = false;
		} else {
			if (x < *min_x) *min_x = x;
			if (x > *max_x) *max_x = x;
			if (y < *min_y) *min_y = y;
			if (y > *max_y) *max_y = y;
		}
	}

	if (n == 0) {
		fprintf(stderr, "No red tiles read.\n");
		return -1;
	}

	return n;
}

int
main(void)
{
	static Pt pts[MAX_PT];

	i64 min_x = 0;
	i64 max_x = 0;
	i64 min_y = 0;
	i64 max_y = 0;

	int n = read_points(pts, MAX_PT, &min_x, &max_x, &min_y, &max_y);
	if (n < 0) {
		return EXIT_FAILURE;
	}

	i64 span_x = max_x - min_x + 1;
	i64 span_y = max_y - min_y + 1;

	if (span_x <= 0 || span_y <= 0) {
		fprintf(stderr, "Invalid span.\n");
		return EXIT_FAILURE;
	}

	int W = (int)span_x;
	int H = (int)span_y;

	int GW = W + 2;
	int GH = H + 2;

	unsigned char **grid = (unsigned char **)malloc((size_t)GH * sizeof(*grid));
	bool          **used = (bool **)malloc((size_t)GH * sizeof(*used));
	if (grid == NULL || used == NULL) {
		fprintf(stderr, "Out of memory (rows).\n");
		return EXIT_FAILURE;
	}

	for (int r = 0; r < GH; r++) {
		grid[r] = (unsigned char *)malloc((size_t)GW * sizeof(unsigned char));
		used[r] = (bool *)malloc((size_t)GW * sizeof(bool));
		if (grid[r] == NULL || used[r] == NULL) {
			fprintf(stderr, "Out of memory (grid/used).\n");
			return EXIT_FAILURE;
		}
		for (int c = 0; c < GW; c++) {
			grid[r][c] = '.';
			used[r][c] = false;
		}
	}

	for (int i = 0; i < n; i++) {
		int row = (int)(pts[i].y - min_y) + 1;
		int col = (int)(pts[i].x - min_x) + 1;

		if (row < 1 || row > H || col < 1 || col > W) {
			fprintf(stderr, "Mapped coordinate out of range.\n");
			return EXIT_FAILURE;
		}

		pts[i].row = row;
		pts[i].col = col;
		grid[row][col] = '#'; /* red */
	}

	for (int i = 0; i < n; i++) {
		int j = (i + 1 == n) ? 0 : (i + 1);

		int r0 = pts[i].row;
		int c0 = pts[i].col;
		int r1 = pts[j].row;
		int c1 = pts[j].col;

		if (r0 == r1) {
			int dc = (c1 > c0) ? 1 : -1;
			for (int c = c0 + dc; c != c1; c += dc) {
				if (grid[r0][c] == '.') {
					grid[r0][c] = 'G';
				}
			}
		} else if (c0 == c1) {
			/* vertical segment */
			int dr = (r1 > r0) ? 1 : -1;
			for (int r = r0 + dr; r != r1; r += dr) {
				if (grid[r][c0] == '.') {
					grid[r][c0] = 'G';
				}
			}
		} else {
			fprintf(stderr, "Non-axial edge between points %d and %d.\n", i, j);
			return EXIT_FAILURE;
		}
	}

	int qcap = GW * GH;
	int *qr = (int *)malloc((size_t)qcap * sizeof(int));
	int *qc = (int *)malloc((size_t)qcap * sizeof(int));
	if (qr == NULL || qc == NULL) {
		fprintf(stderr, "Out of memory (queue).\n");
		return EXIT_FAILURE;
	}

	int head = 0;
	int tail = 0;

	used[0][0] = true;
	qr[tail] = 0;
	qc[tail] = 0;
	tail++;

	static const int dr[4] = { -1, 1, 0, 0 };
	static const int dc[4] = {  0, 0,-1, 1 };

	while (head < tail) {
		int r = qr[head];
		int c = qc[head];
		head++;

		for (int k = 0; k < 4; k++) {
			int nr = r + dr[k];
			int nc = c + dc[k];

			if (nr < 0 || nr >= GH || nc < 0 || nc >= GW) {
				continue;
			}
			if (used[nr][nc]) {
				continue;
			}
			if (grid[nr][nc] != '.') {
				continue;
			}
			used[nr][nc] = true;
			qr[tail] = nr;
			qc[tail] = nc;
			tail++;
		}
	}

	free(qr);
	free(qc);

	for (int r = 1; r <= H; r++) {
		for (int c = 1; c <= W; c++) {
			if (grid[r][c] == '.' && !used[r][c]) {
				grid[r][c] = 'G';
			}
		}
	}

	u32 **pf = (u32 **)malloc((size_t)(H + 1) * sizeof(*pf));
	if (pf == NULL) {
		fprintf(stderr, "Out of memory (prefix rows).\n");
		return EXIT_FAILURE;
	}
	for (int r = 0; r <= H; r++) {
		pf[r] = (u32 *)malloc((size_t)(W + 1) * sizeof(u32));
		if (pf[r] == NULL) {
			fprintf(stderr, "Out of memory (prefix cols).\n");
			return EXIT_FAILURE;
		}
		for (int c = 0; c <= W; c++) {
			pf[r][c] = 0U;
		}
	}

	for (int r = 1; r <= H; r++) {
		for (int c = 1; c <= W; c++) {
			bool forbidden = !(grid[r][c] == '#' || grid[r][c] == 'G');
			u32 val = forbidden ? 1U : 0U;
			pf[r][c] = val
			     + pf[r - 1][c]
			     + pf[r][c - 1]
			     - pf[r - 1][c - 1];
		}
	}

	u64 best_part1 = 0U;
	u64 best_part2 = 0U;

	for (int i = 0; i < n; i++) {
		for (int j = i + 1; j < n; j++) {
			int r1 = pts[i].row;
			int c1 = pts[i].col;
			int r2 = pts[j].row;
			int c2 = pts[j].col;

			int rmin = (r1 < r2) ? r1 : r2;
			int rmax = (r1 > r2) ? r1 : r2;
			int cmin = (c1 < c2) ? c1 : c2;
			int cmax = (c1 > c2) ? c1 : c2;

			u64 dx = (u64)(cmax - cmin);
			u64 dy = (u64)(rmax - rmin);
			u64 area = (dx + 1U) * (dy + 1U);

			if (area > best_part1) {
				best_part1 = area;
			}

			u32 sum_forbid =
			    pf[rmax][cmax]
			  - pf[rmin - 1][cmax]
			  - pf[rmax][cmin - 1]
			  + pf[rmin - 1][cmin - 1];

			if (sum_forbid == 0U && area > best_part2) {
				best_part2 = area;
			}
		}
	}

	printf("Part1: %" PRIu64 "\n", best_part1);
	printf("Part2: %" PRIu64 "\n", best_part2);

	for (int r = 0; r < GH; r++) {
		free(grid[r]);
		free(used[r]);
	}
	free(grid);
	free(used);

	for (int r = 0; r <= H; r++) {
		free(pf[r]);
	}
	free(pf);

	return EXIT_SUCCESS;
}
