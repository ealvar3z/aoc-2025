#include "aoc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void
find_start(const AocGrid *g, int *out_sr, int *out_sc)
{
    if (g == NULL || out_sr == NULL || out_sc == NULL) {
        fprintf(stderr, "find_start: null argument\n");
        exit(EXIT_FAILURE);
    }

    for (int r = 0; r < g->h; r++) {
        for (int c = 0; c < g->w; c++) {
            if (g->cells[r][c] == 'S') {
                *out_sr = r;
                *out_sc = c;
                return;
            }
        }
    }

    fprintf(stderr, "find_start: no 'S' found in grid\n");
    exit(EXIT_FAILURE);
}

static u64
count_splits(const AocGrid *g)
{
    if (g == NULL) {
        return 0U;
    }

    int sr = 0;
    int sc = 0;
    find_start(g, &sr, &sc);

    int H = g->h;
    int W = g->w;

    if (H <= 0 || W <= 0) {
        return 0U;
    }

    static bool seen_cell[AOC_MAX_H][AOC_MAX_W];
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            seen_cell[r][c] = false;
        }
    }

    static int qr[AOC_MAX_H * AOC_MAX_W];
    static int qc[AOC_MAX_H * AOC_MAX_W];
    int head = 0;
    int tail = 0;

    int start_r = sr + 1;
    if (start_r < H && sc >= 0 && sc < W) {
        qr[tail] = start_r;
        qc[tail] = sc;
        tail++;
    }

    u64 splits = 0U;

    while (head < tail) {
        int r0 = qr[head];
        int c0 = qc[head];
        head++;

        int r = r0;
        while (r < H) {
            if (seen_cell[r][c0]) {
                break;
            }

            seen_cell[r][c0] = true;
            char ch = g->cells[r][c0];

            if (ch == '^') {
                splits++;

                int left_c  = c0 - 1;
                int right_c = c0 + 1;

                if (left_c >= 0 && left_c < W) {
                    if (tail >= AOC_MAX_H * AOC_MAX_W) {
                        fprintf(stderr, "queue overflow (left)\n");
                        exit(EXIT_FAILURE);
                    }
                    qr[tail] = r;
                    qc[tail] = left_c;
                    tail++;
                }

                if (right_c >= 0 && right_c < W) {
                    if (tail >= AOC_MAX_H * AOC_MAX_W) {
                        fprintf(stderr, "queue overflow (right)\n");
                        exit(EXIT_FAILURE);
                    }
                    qr[tail] = r;
                    qc[tail] = right_c;
                    tail++;
                }

                break;
            }

            r++;
        }
    }

    return splits;
}

// Part 2

static bool dp_seen[AOC_MAX_H][AOC_MAX_W];
static u64  dp_val[AOC_MAX_H][AOC_MAX_W];

static u64
paths_from(const AocGrid *g, int r, int c)
{
    int H = g->h;
    int W = g->w;

    if (c < 0 || c >= W) {
        return 1U;
    }
    if (r >= H) {
        return 1U;
    }

    if (dp_seen[r][c]) {
        return dp_val[r][c];
    }

    int rr = r;
    while (rr < H && g->cells[rr][c] != '^') {
        rr++;
    }

    u64 result;
    if (rr >= H) {
        result = 1U;
    } else {
        u64 left  = paths_from(g, rr, c - 1);
        u64 right = paths_from(g, rr, c + 1);
        result = left + right;
    }

    dp_seen[r][c] = true;
    dp_val[r][c] = result;
    return result;
}

static u64
count_timelines(const AocGrid *g)
{
    if (g == NULL) {
        return 0U;
    }

    int sr = 0;
    int sc = 0;
    find_start(g, &sr, &sc);

    int H = g->h;
    int W = g->w;

    if (H <= 0 || W <= 0) {
        return 0U;
    }

    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            dp_seen[r][c] = false;
            dp_val[r][c]  = 0U;
        }
    }

    int start_r = sr + 1;
    return paths_from(g, start_r, sc);
}

int
main(void)
{
    AocGrid grid;
    bool ok = grid_load(&grid, stdin);
    if (!ok) {
        fprintf(stderr, "Failed to load manifold grid from input\n");
        return EXIT_FAILURE;
    }

    u64 part1 = count_splits(&grid);
    u64 part2 = count_timelines(&grid);

    printf("Part1: %" PRIu64 "\n", part1);
    printf("Part2: %" PRIu64 "\n", part2);

    return EXIT_SUCCESS;
}
