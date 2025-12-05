#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif // !_POSIX_C_SOURCE

#ifndef AOC_H_INCLUDED
# define AOC_H_INCLUDED 1

# include <ctype.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef long long vlong;           // signed 64-bit
typedef unsigned long long uvlong; // unsigned 64-bit

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

# ifndef AOC_MAX_H
#  define AOC_MAX_H 256
# endif

# ifndef AOC_MAX_W
#  define AOC_MAX_W 256
# endif

# define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
# define SWAP(a, b)                  \
	 do {                        \
		 typeof(a) _t = (a); \
		 (a) = (b);          \
		 (b) = _t;           \
	 } while (0)

#define SWAP_PTR(a, b) SWAP(a, b)

#define MIN(a, b) \
    ((b) ^ (((a) ^ (b)) & -((a) < (b))))

#define MAX(a, b) \
    ((a) ^ (((a) ^ (b)) & -((a) < (b))))

#define CLAMP(x, lo, hi) \
    (MAX((lo), MIN((x), (hi))))

static inline int
modi(int value, int m)
{
	int r = value % m;
	if (r < 0) {
		r += m;
	}
	return r;
}

static inline bool
is_blank_line(const char *s)
{
	if (s == NULL) {
		return true;
	}

	while (*s != '\0') {
		if (!isspace((unsigned char)*s)) {
			return false;
		}
		s++;
	}
	return true;
}

static inline void
strip_spaces(char *s)
{
	if (s == NULL) {
		return;
	}

	char *p = s;
	char *q = s;

	while (*p != '\0') {
		if (!isspace((unsigned char)*p)) {
			*q++ = *p;
		}
		p++;
	}
	*q = '\0';
}

static inline bool
read_line(FILE *fp, char *buf, size_t cap)
{
	if (fp == NULL || buf == NULL || cap == 0U) {
		return false;
	}

	if (fgets(buf, (int)cap, fp) == NULL) {
		return false; // EOF or error
	}

	size_t len = strlen(buf);
	while (len > 0U && (buf[len - 1U] == '\n' || buf[len - 1U] == '\r')) {
		buf[--len] = '\0';
	}

	return true;
}

typedef struct {
	int h;                            // rows
	int w;                            // cols
	char cells[AOC_MAX_H][AOC_MAX_W]; // row-major
} AocGrid;

// In-bounds check.
static inline bool
grid_in_bounds(const AocGrid *g, int r, int c)
{
	return g != NULL && r >= 0 && r < g->h && c >= 0 && c < g->w;
}

static inline char
grid_get(const AocGrid *g, int r, int c)
{
	return g->cells[r][c];
}

static inline void
grid_set(AocGrid *g, int r, int c, char ch)
{
	g->cells[r][c] = ch;
}

// Sets g->h, g->w. Returns true if at least one row was read.
static inline bool
grid_load(AocGrid *g, FILE *fp)
{
	if (g == NULL || fp == NULL) {
		return false;
	}

	// TTY interactivity check
	if (fp == stdin && isatty(fileno(fp))) {
		return false;
	}

	g->h = 0;
	g->w = 0;

	char buf[AOC_MAX_W + 4];
	while (g->h < AOC_MAX_H && read_line(fp, buf, sizeof buf)) {
		if (is_blank_line(buf)) {
			break; // stop at blank separator
		}

		int len = (int)strlen(buf);
		if (len > AOC_MAX_W) {
			fprintf(stderr, "Grid line too wide (%d > %d)\n", len,
			    AOC_MAX_W);
			exit(EXIT_FAILURE);
		}

		if (g->w == 0) {
			g->w = len;
		} else if (len != g->w) {
			fprintf(stderr, "Inconsistent row width in grid\n");
			exit(EXIT_FAILURE);
		}

		for (int c = 0; c < len; c++) {
			g->cells[g->h][c] = buf[c];
		}
		g->h++;
	}

	return (g->h > 0);
}

static inline int
bfs_shortest(const AocGrid *g, int sr, int sc, int tr, int tc,
    bool (*is_open)(char ch))
{
	if (g == NULL || is_open == NULL) {
		return -1;
	}
	if (!grid_in_bounds(g, sr, sc) || !grid_in_bounds(g, tr, tc)) {
		return -1;
	}
	if (!is_open(g->cells[sr][sc]) || !is_open(g->cells[tr][tc])) {
		return -1;
	}

	const int H = g->h;
	const int W = g->w;
	const int N = H * W;

	if (N <= 0) {
		return -1;
	}

	static int qr[AOC_MAX_H * AOC_MAX_W];
	static int qc[AOC_MAX_H * AOC_MAX_W];
	static int dist[AOC_MAX_H][AOC_MAX_W];
	static bool used[AOC_MAX_H][AOC_MAX_W];

	// Initialize visited and dist
	for (int r = 0; r < H; r++) {
		for (int c = 0; c < W; c++) {
			used[r][c] = false;
			dist[r][c] = -1;
		}
	}

	int head = 0;
	int tail = 0;

	used[sr][sc] = true;
	dist[sr][sc] = 0;
	qr[tail] = sr;
	qc[tail] = sc;
	tail++;

	static const int dr[4] = {-1, 1, 0, 0};
	static const int dc[4] = {0, 0, -1, 1};

	while (head < tail) {
		int r = qr[head];
		int c = qc[head];
		head++;

		if (r == tr && c == tc) {
			return dist[r][c];
		}

		for (int k = 0; k < 4; k++) {
			int nr = r + dr[k];
			int nc = c + dc[k];

			if (!grid_in_bounds(g, nr, nc)) {
				continue;
			}
			if (used[nr][nc]) {
				continue;
			}
			char ch = g->cells[nr][nc];
			if (!is_open(ch)) {
				continue;
			}

			used[nr][nc] = true;
			dist[nr][nc] = dist[r][c] + 1;

			if (tail >= AOC_MAX_H * AOC_MAX_W) {
				fprintf(stderr, "BFS queue overflow (increase "
						"AOC_MAX_H/W)\n");
				exit(EXIT_FAILURE);
			}

			qr[tail] = nr;
			qc[tail] = nc;
			tail++;
		}
	}

	return -1; // unreachable
}

#endif /* AOC_H_INCLUDED */
