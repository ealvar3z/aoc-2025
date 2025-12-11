#include <inttypes.h>
#include <limits.h>    // for LLONG_MAX
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 20      // max indicator lights per machine 
#endif

#ifndef MAX_BUTTONS
#define MAX_BUTTONS 64     // max buttons per machine 
#endif

#ifndef MAX_CNTS
#define MAX_CNTS 16        // max joltage counters per machine 
#endif

typedef struct {
    int count;
    int idx[MAX_CNTS];  // indices toggled / incremented by this button 
} Button;

typedef struct {
    // Part 1: lights 
    int  lights_n;       // number of lights L 
    u64  lights_target;  // bitmask target pattern 

    // Part 2: joltage counters 
    int  cnt_n;          // number of counters 
    int  target[MAX_CNTS];

    // Shared buttons 
    int    btn_n;
    Button btns[MAX_BUTTONS];
} Machine;

// pattern parser [.##.]
static bool
parse_pattern(const char **pp, Machine *m, const char *line)
{
    const char *p = *pp;

    m->lights_n      = 0;
    m->lights_target = 0U;

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    while (*p != '\0' && *p != '[') {
        p++;
    }
    if (*p != '[') {
        fprintf(stderr, "Missing '[' in line: '%s'\n", line);
        return false;
    }
    p++;  // after '[' 

    int  L = 0;
    u64  mask = 0U;

    while (*p != '\0' && *p != ']') {
        char ch = *p;

        if (ch == '.' || ch == '#') {
            if (L >= MAX_LIGHTS) {
                fprintf(stderr, "Too many lights (> %d)\n", MAX_LIGHTS);
                return false;
            }
            if (ch == '#') {
                mask |= (1ULL << L);
            }
            L++;
        } else if (ch == ' ' || ch == '\t') {
            // ignore whitespace inside pattern 
        } else {
            fprintf(stderr, "Invalid character '%c' in pattern: '%s'\n",
                    ch, line);
            return false;
        }
        p++;
    }

    if (*p != ']') {
        fprintf(stderr, "Missing ']' in line: '%s'\n", line);
        return false;
    }
    p++;  // after ']' 

    if (L <= 0) {
        fprintf(stderr, "Empty pattern in line: '%s'\n", line);
        return false;
    }

    m->lights_n      = L;
    m->lights_target = mask;
    *pp = p;
    return true;
}

// button parser (0,2,3)
static bool
parse_buttons(const char **pp, Machine *m, const char *line)
{
    const char *p = *pp;

    m->btn_n = 0;

    while (*p != '\0' && *p != '{') {
        if (*p != '(') {
            p++;
            continue;
        }

        if (m->btn_n >= MAX_BUTTONS) {
            fprintf(stderr, "Too many buttons (> %d).\n", MAX_BUTTONS);
            return false;
        }

        Button *b = &m->btns[m->btn_n];
        b->count  = 0;
        p++;  // after '(' 

        for (;;) {
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            if (*p == ')') {
                p++;
                break;
            }
            if (*p == '\0') {
                fprintf(stderr, "Unterminated '(' in line: '%s'\n", line);
                return false;
            }

            char *endptr = NULL;
            long idx = strtol(p, &endptr, 10);
            if (endptr == p) {
                fprintf(stderr, "Expected integer index in button: '%s'\n",
                        line);
                return false;
            }
            p = endptr;

            if (b->count >= MAX_CNTS) {
                fprintf(stderr, "Button has too many indices (> %d).\n",
                        MAX_CNTS);
                return false;
            }
            b->idx[b->count++] = (int)idx;

            while (*p == ' ' || *p == '\t') {
                p++;
            }
            if (*p == ',') {
                p++;
                continue;
            }
            if (*p == ')') {
                p++;
                break;
            }
            if (*p == '\0') {
                fprintf(stderr, "Unterminated button in line: '%s'\n", line);
                return false;
            }
            fprintf(stderr, "Unexpected character '%c' in button spec: '%s'\n",
                    *p, line);
            return false;
        }

        if (b->count > 0) {
            m->btn_n++;
        }
    }

    *pp = p;
    return true;
}

// joltage parser {3,5,4,7}
static bool
parse_jolts(const char **pp, Machine *m, const char *line)
{
    const char *p = *pp;

    m->cnt_n = 0;

    while (*p != '\0' && *p != '{') {
        p++;
    }
    if (*p != '{') {
        fprintf(stderr, "Missing '{' with joltage requirements in line: '%s'\n",
                line);
        return false;
    }
    p++;  // after '{' 

    int cnt_n = 0;

    for (;;) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '}') {
            p++;
            break;
        }
        if (*p == '\0') {
            fprintf(stderr, "Unterminated '{' in line: '%s'\n", line);
            return false;
        }

        char *endptr = NULL;
        long val = strtol(p, &endptr, 10);
        if (endptr == p) {
            fprintf(stderr, "Expected joltage integer in line: '%s'\n", line);
            return false;
        }
        p = endptr;

        if (cnt_n >= MAX_CNTS) {
            fprintf(stderr, "Too many counters (> %d).\n", MAX_CNTS);
            return false;
        }
        m->target[cnt_n++] = (int)val;

        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '}') {
            p++;
            break;
        }
        if (*p == '\0') {
            fprintf(stderr,
                    "Unterminated joltage list in line: '%s'\n", line);
            return false;
        }
        fprintf(stderr, "Unexpected character '%c' in joltage list: '%s'\n",
                *p, line);
        return false;
    }

    if (cnt_n <= 0) {
        fprintf(stderr, "No joltage requirements in line: '%s'\n", line);
        return false;
    }

    m->cnt_n = cnt_n;
    *pp      = p;
    return true;
}

static bool
validate_button_indices(const Machine *m, const char *line)
{
    for (int bi = 0; bi < m->btn_n; bi++) {
        const Button *b = &m->btns[bi];
        for (int k = 0; k < b->count; k++) {
            int idx = b->idx[k];

            if (idx < 0 ||
                idx >= m->lights_n ||
                idx >= m->cnt_n) {
                fprintf(stderr,
                        "Button index %d out of range "
                        "(lights=%d, counters=%d) in line: '%s'\n",
                        idx, m->lights_n, m->cnt_n, line);
                return false;
            }
        }
    }
    return true;
}

static bool
parse_machine(const char *line, Machine *m)
{
    const char *p = line;

    m->lights_n      = 0;
    m->lights_target = 0U;
    m->cnt_n         = 0;
    m->btn_n         = 0;

    if (!parse_pattern(&p, m, line)) {
        return false;
    }
    if (!parse_buttons(&p, m, line)) {
        return false;
    }
    if (!parse_jolts(&p, m, line)) {
        return false;
    }
    if (!validate_button_indices(m, line)) {
        return false;
    }

    return true;
}

static bool
build_light_masks(const Machine *m,
                  int L,
                  u64 masks_out[MAX_BUTTONS],
                  int *mask_count_out)
{
    int count = 0;

    for (int bi = 0; bi < m->btn_n; bi++) {
        const Button *b = &m->btns[bi];
        u64 mask = 0U;

        for (int k = 0; k < b->count; k++) {
            int idx = b->idx[k];

            if (idx < 0 || idx >= L) {
                continue;
            }

            mask |= (1ULL << (u64)idx);
        }

        if (mask != 0U) {
            if (count >= MAX_BUTTONS) {
                fprintf(stderr,
                    "Too many effective light buttons (> %d)\n",
                    MAX_BUTTONS);
                return false;
            }
            masks_out[count++] = mask;
        }
    }

    *mask_count_out = count;
    return (count > 0);
}

static int
bfs_min_presses_lights(int L,
                       u64 target,
                       const u64 masks[MAX_BUTTONS],
                       int mask_count)
{
    if (L <= 0 || L > MAX_LIGHTS) {
        return -1;
    }

    u32 state_count = (u32)(1U << L);

    int *dist = (int *)malloc((size_t)state_count * sizeof(int));
    u32 *queue = (u32 *)malloc((size_t)state_count * sizeof(u32));

    if (dist == NULL || queue == NULL) {
        fprintf(stderr, "Out of memory in Part 1 BFS.\n");
        free(dist);
        free(queue);
        return -1;
    }

    for (u32 i = 0U; i < state_count; i++) {
        dist[i] = -1;
    }

    u32 head = 0U;
    u32 tail = 0U;

    dist[0] = 0;
    queue[tail++] = 0U;

    while (head < tail) {
        u32 s = queue[head++];
        int d = dist[s];

        if ((u64)s == target) {
            int answer = d;
            free(dist);
            free(queue);
            return answer;
        }

        for (int bi = 0; bi < mask_count; bi++) {
            u64 ns_u64 = (u64)s ^ masks[bi];
            u32 ns = (u32)ns_u64;

            if (dist[ns] == -1) {
                dist[ns] = d + 1;
                queue[tail++] = ns;
            }
        }
    }

    free(dist);
    free(queue);
    return -1;
}

// Part 1
static int
min_presses_lights(const Machine *m)
{
    int L = m->lights_n;
    u64 target = m->lights_target;

    if (target == 0U) {
        return 0;
    }

    u64 masks[MAX_BUTTONS];
    int  mask_count = 0;

    if (!build_light_masks(m, L, masks, &mask_count)) {
        return -1;  // no useful buttons or error 
    }

    return bfs_min_presses_lights(L, target, masks, mask_count);
}

// Part 2
typedef struct {
    vlong num;
    vlong den;   // always > 0 when value is defined 
} Frac;

static vlong
ll_gcd(vlong a, vlong b)
{
    if (a < 0) {
        a = -a;
    }
    if (b < 0) {
        b = -b;
    }
    while (b != 0) {
        vlong t = a % b;
        a = b;
        b = t;
    }
    return (a == 0) ? 1 : a;
}

static Frac
frac_make(vlong num, vlong den)
{
    Frac f;
    if (den < 0) {
        num = -num;
        den = -den;
    }
    if (den == 0) {
        // treat as "infinite"; only used rarely, keep simple 
        f.num = 1;
        f.den = 0;
        return f;
    }
    if (num == 0) {
        f.num = 0;
        f.den = 1;
        return f;
    }
    vlong g = ll_gcd(num, den);
    f.num = num / g;
    f.den = den / g;
    return f;
}

static Frac
frac_sub(Frac a, Frac b)
{
    vlong num = a.num * b.den - b.num * a.den;
    vlong den = a.den * b.den;
    return frac_make(num, den);
}

static Frac
frac_mul(Frac a, Frac b)
{
    vlong num = a.num * b.num;
    vlong den = a.den * b.den;
    return frac_make(num, den);
}

static Frac
frac_div(Frac a, Frac b)
{
    vlong num = a.num * b.den;
    vlong den = a.den * b.num;
    return frac_make(num, den);
}

static bool
frac_is_zero(Frac a)
{
    return a.num == 0;
}

static bool
frac_is_int(Frac a)
{
    return a.den == 1;
}

static bool
build_jolt_matrix(const Machine *m,
                  int rows,
                  int cols,
                  Frac M[MAX_CNTS][MAX_BUTTONS + 1],
                  int max_press[MAX_BUTTONS])
{
    bool seen[MAX_CNTS] = { false };

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int val = 0;
            const Button *b = &m->btns[j];

            for (int k = 0; k < b->count; k++) {
                if (b->idx[k] == i) {
                    val = 1;
                    break;
                }
            }
            M[i][j] = frac_make(val, 1);
        }
        M[i][cols] = frac_make(m->target[i], 1);
    }

    for (int j = 0; j < cols; j++) {
        int bound = INT_MAX;
        const Button *b = &m->btns[j];

        if (b->count == 0) {
            max_press[j] = 0;
            continue;
        }

        for (int k = 0; k < b->count; k++) {
            int idx = b->idx[k];
            if (idx >= 0 && idx < rows) {
                seen[idx] = true;
                int t = m->target[idx];
                if (t < bound) {
                    bound = t;
                }
            }
        }

        if (bound < 0 || bound == INT_MAX) {
            bound = 0;
        }
        max_press[j] = bound;
    }

    for (int i = 0; i < rows; i++) {
        if (m->target[i] > 0 && !seen[i]) {
            return false;
        }
    }

    return true;
}

static bool
gauss_rref(int rows,
           int cols,
           Frac M[MAX_CNTS][MAX_BUTTONS + 1],
           int pivot_row_for_col[MAX_BUTTONS],
           int *rank_out)
{
    for (int j = 0; j < cols; j++) {
        pivot_row_for_col[j] = -1;
    }

    int rank = 0;
    int lead_col = 0;

    while (rank < rows && lead_col < cols) {
        int pivot_row = -1;

        for (int r = rank; r < rows; r++) {
            if (!frac_is_zero(M[r][lead_col])) {
                pivot_row = r;
                break;
            }
        }

        if (pivot_row == -1) {
            lead_col++;
            continue;
        }

        if (pivot_row != rank) {
            for (int c = 0; c <= cols; c++) {
                Frac tmp      = M[rank][c];
                M[rank][c]    = M[pivot_row][c];
                M[pivot_row][c] = tmp;
            }
        }

        Frac pv = M[rank][lead_col];
        for (int c = lead_col; c <= cols; c++) {
            M[rank][c] = frac_div(M[rank][c], pv);
        }

        for (int r = 0; r < rows; r++) {
            if (r == rank) {
                continue;
            }
            Frac factor = M[r][lead_col];
            if (frac_is_zero(factor)) {
                continue;
            }
            for (int c = lead_col; c <= cols; c++) {
                Frac delta = frac_mul(factor, M[rank][c]);
                M[r][c] = frac_sub(M[r][c], delta);
            }
        }

        pivot_row_for_col[lead_col] = rank;
        rank++;
        lead_col++;
    }

    for (int r = rank; r < rows; r++) {
        bool all_zero = true;
        for (int c = 0; c < cols; c++) {
            if (!frac_is_zero(M[r][c])) {
                all_zero = false;
                break;
            }
        }
        if (all_zero && !frac_is_zero(M[r][cols])) {
            return false;
        }
    }

    *rank_out = rank;
    return true;
}

static int
solve_unique_solution(int cols,
                      const Frac M[MAX_CNTS][MAX_BUTTONS + 1],
                      const int pivot_row_for_col[MAX_BUTTONS],
                      const int max_press[MAX_BUTTONS])
{
    vlong sum = 0;

    for (int j = 0; j < cols; j++) {
        int r = pivot_row_for_col[j];

        if (r < 0) {
            return -1;
        }

        Frac val = M[r][cols];

        if (!frac_is_int(val)) {
            return -1;
        }
        if (val.num < 0) {
            return -1;
        }
        if (val.num > max_press[j]) {
            return -1;
        }

        sum += val.num;
        if (sum > (vlong)INT_MAX) {
            return -1;
        }
    }

    return (int)sum;
}

typedef struct {
    int rows;
    int cols;
    int rank;

    const Frac (*M)[MAX_BUTTONS + 1];

    const int *pivot_row_for_col;
    const int *free_cols;
    int        num_free;

    const int *max_press;

    vlong best_sum;
    vlong x_sol[MAX_BUTTONS];
    vlong x_curr[MAX_BUTTONS];
} SearchCtx;

static void
dfs_free(SearchCtx *ctx, int idx, vlong partial_sum)
{
    if (partial_sum >= ctx->best_sum) {
        return;
    }

    if (idx == ctx->num_free) {
        vlong sum = partial_sum;

        for (int j = 0; j < ctx->cols; j++) {
            int r = ctx->pivot_row_for_col[j];

            if (r < 0) {
                continue;  // free variable or non-pivot column 
            }

            Frac val = ctx->M[r][ctx->cols];

            for (int fi = 0; fi < ctx->num_free; fi++) {
                int fc   = ctx->free_cols[fi];
                Frac cof = ctx->M[r][fc];

                if (frac_is_zero(cof)) {
                    continue;
                }

                vlong xfree = ctx->x_curr[fc];
                Frac term   = frac_mul(cof, frac_make(xfree, 1));
                val = frac_sub(val, term);
            }

            if (!frac_is_int(val)) {
                return;
            }

            vlong xj = val.num;
            if (xj < 0) {
                return;
            }
            if (xj > ctx->max_press[j]) {
                return;
            }

            ctx->x_curr[j] = xj;
            sum += xj;

            if (sum >= ctx->best_sum) {
                return;
            }
        }

        if (sum < ctx->best_sum) {
            ctx->best_sum = sum;
            for (int j = 0; j < ctx->cols; j++) {
                ctx->x_sol[j] = ctx->x_curr[j];
            }
        }

        return;
    }

    int fc   = ctx->free_cols[idx];
    int maxv = ctx->max_press[fc];

    for (int v = 0; v <= maxv; v++) {
        ctx->x_curr[fc] = (vlong)v;
        dfs_free(ctx, idx + 1, partial_sum + (vlong)v);
    }
}

static int
solve_with_free_vars(int rows,
                     int cols,
                     int rank,
                     const Frac M[MAX_CNTS][MAX_BUTTONS + 1],
                     const int pivot_row_for_col[MAX_BUTTONS],
                     const int max_press[MAX_BUTTONS],
                     const int free_cols[MAX_BUTTONS],
                     int num_free)
{
    SearchCtx ctx;

    ctx.rows  = rows;
    ctx.cols  = cols;
    ctx.rank  = rank;
    ctx.M     = M;

    ctx.pivot_row_for_col = pivot_row_for_col;
    ctx.free_cols         = free_cols;
    ctx.num_free          = num_free;

    ctx.max_press = max_press;

    ctx.best_sum = LLONG_MAX;

    for (int j = 0; j < cols; j++) {
        ctx.x_curr[j] = 0;
        ctx.x_sol[j]  = 0;
    }

    dfs_free(&ctx, 0, 0);

    if (ctx.best_sum == LLONG_MAX) {
        return -1;
    }

    if (ctx.best_sum > (vlong)INT_MAX) {
        return -1;
    }

    return (int)ctx.best_sum;
}

static int
min_presses_jolts(const Machine *m)
{
    int rows = m->cnt_n;
    int cols = m->btn_n;

    if (rows <= 0 || cols <= 0) {
        return -1;
    }

    Frac M[MAX_CNTS][MAX_BUTTONS + 1];
    int  max_press[MAX_BUTTONS];

    if (!build_jolt_matrix(m, rows, cols, M, max_press)) {
        return -1;
    }

    int pivot_row_for_col[MAX_BUTTONS];
    int rank = 0;

    if (!gauss_rref(rows, cols, M, pivot_row_for_col, &rank)) {
        return -1;
    }

    bool is_pivot_col[MAX_BUTTONS];
    for (int j = 0; j < cols; j++) {
        is_pivot_col[j] = (pivot_row_for_col[j] != -1);
    }

    int free_cols[MAX_BUTTONS];
    int num_free = 0;

    for (int j = 0; j < cols; j++) {
        if (!is_pivot_col[j]) {
            free_cols[num_free++] = j;
        }
    }

    if (num_free == 0) {
        return solve_unique_solution(cols, M, pivot_row_for_col, max_press);
    }

    return solve_with_free_vars(rows, cols, rank,
                                M,
                                pivot_row_for_col,
                                max_press,
                                free_cols,
                                num_free);
}

int
main(void)
{
    char line[1024];
    u64 total_part1 = 0U;
    u64 total_part2 = 0U;
    int machine_index = 0;

    while (read_line(stdin, line, sizeof line)) {
        if (is_blank_line(line)) {
            continue;
        }

        Machine m;
        if (!parse_machine(line, &m)) {
            fprintf(stderr, "Failed to parse machine %d.\n", machine_index);
            return EXIT_FAILURE;
        }

        int p1 = min_presses_lights(&m);
        if (p1 < 0) {
            fprintf(stderr, "Machine %d: Part1 configuration impossible.\n",
                    machine_index);
            return EXIT_FAILURE;
        }
        total_part1 += (u64)p1;

        int p2 = min_presses_jolts(&m);
        if (p2 < 0) {
            fprintf(stderr,
                    "Machine %d: Part2 configuration impossible (or search exhausted).\n",
                    machine_index);
            return EXIT_FAILURE;
        }
        total_part2 += (u64)p2;

        machine_index++;
    }

    printf("Part1: %" PRIu64 "\n", total_part1);
    printf("Part2: %" PRIu64 "\n", total_part2);

    return EXIT_SUCCESS;
}
