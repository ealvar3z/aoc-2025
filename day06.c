#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "aoc.h"

typedef struct {
    int  c0;
    int  c1;
    char op;
} Block;

#ifndef MAX_BLOCKS
#define MAX_BLOCKS 4096
#endif

static const char *sample_rows[] = {
    "123 328  51 64 ",
    " 45 64  387 23 ",
    "  6 98  215 314",
    "*   +   *   +  "
};

static void
fill_sample(AocGrid *g)
{
    if (g == NULL) {
        return;
    }

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

static bool
is_col_blank(const AocGrid *g, int c)
{
    if (g == NULL) {
        return true;
    }
    if (c < 0 || c >= g->w) {
        return true;
    }

    for (int r = 0; r < g->h; r++) {
        if (g->cells[r][c] != ' ') {
            return false;
        }
    }
    return true;
}

static char
block_op(const AocGrid *g, int c0, int c1)
{
    if (g == NULL) {
        return '\0';
    }

    int r = g->h - 1;
    if (c0 < 0) {
        c0 = 0;
    }
    if (c1 >= g->w) {
        c1 = g->w - 1;
    }

    for (int c = c0; c <= c1; c++) {
        char ch = g->cells[r][c];
        if (ch == '+' || ch == '*') {
            return ch;
        }
    }
    return '\0';
}

/* Extract at most one integer from row r between [c0, c1] (Part 1 view). */
static bool
row_number_in_block(const AocGrid *g, int r, int c0, int c1, u64 *out)
{
    if (g == NULL || out == NULL) {
        return false;
    }
    if (r < 0 || r >= g->h) {
        return false;
    }
    if (c0 < 0) {
        c0 = 0;
    }
    if (c1 >= g->w) {
        c1 = g->w - 1;
    }
    if (c0 > c1) {
        return false;
    }

    bool in_digits = false;
    u64  val       = 0U;
    bool have      = false;

    for (int c = c0; c <= c1; c++) {
        char ch = g->cells[r][c];
        if (ch >= '0' && ch <= '9') {
            if (!in_digits) {
                in_digits = true;
                val       = 0U;
            }
            val = val * 10U + (u64)(ch - '0');
        } else {
            if (in_digits) {
                have = true;
                break;
            }
        }
    }
    if (in_digits && !have) {
        have = true;
    }

    if (have) {
        *out = val;
    }
    return have;
}

/* Describe all column blocks (problems) in the grid. */
static int
find_blocks(const AocGrid *g, Block *blocks, int max_blocks)
{
    if (g == NULL || blocks == NULL || max_blocks <= 0) {
        return 0;
    }

    int H = g->h;
    int W = g->w;

    if (H < 2 || W <= 0) {
        return 0;
    }

    bool blank[AOC_MAX_W];
    for (int c = 0; c < W; c++) {
        blank[c] = is_col_blank(g, c);
    }

    int nb = 0;
    int c  = 0;

    while (c < W) {
        while (c < W && blank[c]) {
            c++;
        }
        if (c >= W) {
            break;
        }

        int c0 = c;
        while (c < W && !blank[c]) {
            c++;
        }
        int c1 = c - 1;

        char op = block_op(g, c0, c1);
        if (op != '+' && op != '*') {
            continue;
        }

        if (nb < max_blocks) {
            blocks[nb].c0 = c0;
            blocks[nb].c1 = c1;
            blocks[nb].op = op;
            nb++;
        } else {
            fprintf(stderr, "too many blocks (cap=%d)\n", max_blocks);
            exit(EXIT_FAILURE);
        }
    }

    return nb;
}

/* Part 1: numbers are horizontal in each row within block. */
static u64
eval_block_part1(const AocGrid *g, const Block *b)
{
    int H = g->h;

    u64 acc      = (b->op == '+') ? 0U : 1U;
    bool have_any = false;

    for (int r = 0; r < H - 1; r++) {
        u64 val = 0U;
        if (row_number_in_block(g, r, b->c0, b->c1, &val)) {
            have_any = true;
            if (b->op == '+') {
                acc += val;
            } else {
                acc *= val;
            }
        }
    }
    return have_any ? acc : 0U;
}

/* Part 2: each column inside block is a number (top->bottom), read right->left. */
static u64
eval_block_part2(const AocGrid *g, const Block *b)
{
    int H = g->h;

    u64 acc      = (b->op == '+') ? 0U : 1U;
    bool have_any = false;

    for (int c = b->c1; c >= b->c0; c--) {
        u64 val  = 0U;
        bool got = false;

        for (int r = 0; r < H - 1; r++) {
            char ch = g->cells[r][c];
            if (ch >= '0' && ch <= '9') {
                val = val * 10U + (u64)(ch - '0');
                got = true;
            }
        }

        if (got) {
            have_any = true;
            if (b->op == '+') {
                acc += val;
            } else {
                acc *= val;
            }
        }
    }
    return have_any ? acc : 0U;
}

static void
solve_both(const AocGrid *g, u64 *out_p1, u64 *out_p2)
{
    Block blocks[MAX_BLOCKS];
    int   nb = find_blocks(g, blocks, MAX_BLOCKS);

    u64 p1 = 0U;
    u64 p2 = 0U;

    for (int i = 0; i < nb; i++) {
        p1 += eval_block_part1(g, &blocks[i]);
        p2 += eval_block_part2(g, &blocks[i]);
    }

    if (out_p1 != NULL) {
        *out_p1 = p1;
    }
    if (out_p2 != NULL) {
        *out_p2 = p2;
    }
}

int
main(void)
{
    AocGrid grid;
    bool    ok;

    ok = grid_load(&grid, stdin);
    if (!ok) {
        fill_sample(&grid);

        u64 sample_p1 = 0U;
        u64 sample_p2 = 0U;
        solve_both(&grid, &sample_p1, &sample_p2);

        printf("Part1 sample: %" PRIu64 "\n", sample_p1);
        printf("Part2 sample: %" PRIu64 "\n", sample_p2);
    } else {
        u64 part1 = 0U;
        u64 part2 = 0U;
        solve_both(&grid, &part1, &part2);

        printf("Part1: %" PRIu64 "\n", part1);
        printf("Part2: %" PRIu64 "\n", part2);
    }

    return EXIT_SUCCESS;
}
