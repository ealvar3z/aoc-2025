#include "aoc.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const char *sample_rows[] = {
    "..@@.@@@@.",
    "@@@.@.@.@@",
    "@@@@@.@.@@",
    "@.@@@@..@.",
    "@@.@@@@.@@",
    ".@@@@@@@.@",
    ".@.@.@.@@@",
    "@.@@@.@@@@",
    ".@@@@@@@@.",
    "@.@.@@@.@."
};

static void
fill_sample(AocGrid *g)
{
    int H = (int)(sizeof sample_rows / sizeof sample_rows[0]);
    int W = (int)strlen(sample_rows[0]);

    g->h = H;
    g->w = W;

    for (int r = 0; r < H; r++) {
        const char *src = sample_rows[r];
        int len = (int)strlen(src);
        if (len != W) {
            fprintf(stderr, "sample row width mismatch\n");
            exit(EXIT_FAILURE);
        }
        for (int c = 0; c < W; c++) {
            g->cells[r][c] = src[c];
        }
    }
}

static u32
count_access(const AocGrid *rolls)
{
    static const int dr[8] = { -1, -1, -1,  0, 0, 1, 1, 1 };
    static const int dc[8] = { -1,  0,  1, -1, 1,-1, 0, 1 };

    u32 free = 0U;

    for (int r = 0; r < rolls->h; r++) {
        for (int c = 0; c < rolls->w; c++) {
            if (rolls->cells[r][c] != '@') {
                continue;
            }

            int adj = 0;

            for (int k = 0; k < 8; k++) {
                int nr = r + dr[k];
                int nc = c + dc[k];
                if (!grid_in_bounds(rolls, nr, nc)) {
                    continue;
                }
                if (rolls->cells[nr][nc] == '@') {
                    adj++;
                }
            }

            if (adj < 4) {
                free++;
            }
        }
    }

    return free;
}

static u32
count_removed(const AocGrid *rolls)
{
    static const int dr[8] = { -1, -1, -1,  0, 0, 1, 1, 1 };
    static const int dc[8] = { -1,  0,  1, -1, 1,-1, 0, 1 };

    int H = rolls->h;
    int W = rolls->w;

    static int  adj[AOC_MAX_H][AOC_MAX_W];
    static char gone[AOC_MAX_H][AOC_MAX_W];

    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            adj[r][c]  = 0;
            gone[r][c] = 0;
        }
    }

    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            if (rolls->cells[r][c] != '@') {
                continue;
            }
            int count = 0;
            for (int k = 0; k < 8; k++) {
                int nr = r + dr[k];
                int nc = c + dc[k];
                if (!grid_in_bounds(rolls, nr, nc)) {
                    continue;
                }
                if (rolls->cells[nr][nc] == '@') {
                    count++;
                }
            }
            adj[r][c] = count;
        }
    }

    static int qr[AOC_MAX_H * AOC_MAX_W];
    static int qc[AOC_MAX_H * AOC_MAX_W];
    int head = 0;
    int tail = 0;

    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            if (rolls->cells[r][c] != '@') {
                continue;
            }
            if (adj[r][c] < 4) {
                qr[tail] = r;
                qc[tail] = c;
                tail++;
            }
        }
    }

    u32 removed = 0U;

    while (head < tail) {
        int r = qr[head];
        int c = qc[head];
        head++;

        if (gone[r][c]) {
            continue;
        }
        if (rolls->cells[r][c] != '@') {
            continue;
        }

        gone[r][c] = 1;
        removed++;

        for (int k = 0; k < 8; k++) {
            int nr = r + dr[k];
            int nc = c + dc[k];

            if (!grid_in_bounds(rolls, nr, nc)) {
                continue;
            }
            if (rolls->cells[nr][nc] != '@') {
                continue;
            }
            if (gone[nr][nc]) {
                continue;
            }

            if (adj[nr][nc] > 0) {
                adj[nr][nc]--;
            }

            if (adj[nr][nc] < 4) {
                qr[tail] = nr;
                qc[tail] = nc;
                tail++;
            }
        }
    }

    return removed;
}

int
main(void)
{
  AocGrid rolls;
  u32 part1;
  u32 part2;

  bool have_input = grid_load(&rolls, stdin);
  if (have_input) {
    part1 = count_access(&rolls);
    part2 = count_removed(&rolls);
    printf("Part1 sample: %u\n", part1);
    printf("Part2 sample: %u\n", part2);
  } else {
    fill_sample(&rolls);
    part1 = count_access(&rolls);
    part2 = count_removed(&rolls);
    printf("Part1: %u\n", part1);
    printf("Part2: %u\n", part2);
  }
  return EXIT_SUCCESS;
}

