// day01.c - Advent of Code Day 01 (Parts 1 and 2)
// C23, stdlib-only
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>   // isatty, fileno

enum {
    MODULUS       = 100,
    LINE_BUF_SIZE = 128
};

static int
mod_int(int value, int m)
{
    int r = value % m;
    if (r < 0) {
        r += m;
    }
    return r;
}

static void
trim_whitespace(char *s)
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

static void
process_line(const char *line, int *pos, long long *part1, long long *part2)
{
    if (line == NULL || *line == '\0') {
        return;
    }

    char dir = line[0];
    if (dir != 'L' && dir != 'R') {
        fprintf(stderr, "Invalid direction in line: %s\n", line);
        exit(EXIT_FAILURE);
    }

    const char *p = line + 1;
    if (*p == '\0') {
        fprintf(stderr, "Missing distance in line: %s\n", line);
        exit(EXIT_FAILURE);
    }

    char *endptr = NULL;
    long long dist_full = strtoll(p, &endptr, 10);
    if (endptr == p || dist_full < 0) {
        fprintf(stderr, "Invalid distance in line: %s\n", line);
        exit(EXIT_FAILURE);
    }

    // Part 2: count hits of 0 during this rotation
    long long hits = 0;
    int s = mod_int(*pos, MODULUS); // normalize starting position
    long long d = dist_full;

    if (d > 0) {
        long long t0;

        if (dir == 'R') {
            t0 = (MODULUS - s) % MODULUS; 
            if (t0 == 0) {
                t0 = MODULUS;
            }
        } else {
            t0 = s % MODULUS;
            if (t0 == 0) {
                t0 = MODULUS;
            }
        }

        if (t0 <= d) {
            hits = 1 + ( (d - t0) / MODULUS );
        }
    }

    *part2 += hits;

    // Part 1 + position update:
    int step = (int)(dist_full % MODULUS);
    if (dir == 'L') {
        *pos = mod_int(*pos - step, MODULUS);
    } else {
        *pos = mod_int(*pos + step, MODULUS);
    }

    if (*pos == 0) {
        (*part1)++;
    }
}

// Embedded sample data a la Perl's __DATA__ equivalent
static const char *sample_data[] = {
    "L68",
    "L30",
    "R48",
    "L5",
    "R60",
    "L55",
    "L1",
    "L99",
    "R14",
    "L82"
};
static const size_t sample_count =
    sizeof sample_data / sizeof sample_data[0];

int
main(void)
{
    int        pos   = 50;
    long long  part1 = 0;
    long long  part2 = 0;

    // if stdin is a terminal, use embedded sample.
    bool use_sample = false;
    if (isatty(fileno(stdin))) {
        use_sample = true;
    }

    if (use_sample) {
        for (size_t i = 0U; i < sample_count; i++) {
            process_line(sample_data[i], &pos, &part1, &part2);
        }
    } else {
        char buf[LINE_BUF_SIZE];

        while (fgets(buf, (int)sizeof buf, stdin) != NULL) {
            size_t len = strlen(buf);
            if (len > 0U && buf[len - 1U] == '\n') {
                buf[len - 1U] = '\0';
            }

            trim_whitespace(buf);
            if (buf[0] == '\0') {
                continue;
            }

            process_line(buf, &pos, &part1, &part2);
        }

        if (ferror(stdin)) {
            fprintf(stderr, "Error reading stdin\n");
            return EXIT_FAILURE;
        }
    }

    printf("Part 1: %lld\n", part1);
    printf("Part 2: %lld\n", part2);

    return EXIT_SUCCESS;
}
