#include "aoc.h"
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BANKS 4096
#define MAX_WIDTH 256

static char sample_banks[][MAX_WIDTH + 1U] = {
	"987654321111111",
	"811111111111119",
	"234234234234278",
	"818181911112111",
};

#define SAMPLE_BANK_COUNT (sizeof(sample_banks) / sizeof(sample_banks[0]))

static u32
best_bank(const char *s)
{
	size_t len;
	u8 best_right;
	u32 best = 0U;

	if (s == NULL) {
		return 0U;
	}
	len = strlen(s);
	if (len < 2U) {
		return 0U;
	}
	best_right = (u8)(s[len - 1U] - '0');
	for (size_t i = len - 1U; i > 0U; i--) {
		size_t pos = i - 1U;
		u8 d = (u8)(s[pos] - '0');
		u32 N = (u32)(10U * d + best_right);
		if (N > best) {
			best = N;
		}
		if (d > best_right) {
			best_right = d;
		}
	}
	return best;
}

// Part 2
static u64
output_joltage(const char *s)
{
	const size_t k = 12U;
	size_t start = 0U;
	u64 val = 0U;
	size_t len;

	if (s == NULL) {
		return 0U;
	}
	len = strlen(s);
	if (len == 0U) {
		return 0U;
	}
	// if the bank is < 12 digits, use all digits
	if (len <= k) {
		for (size_t i = 0U; i < len; i++) {
			u8 d = (u8)(s[i] - '0');
			val = val * 10U + (u64)d;
		}
		return val;
	}
	for (size_t picked = 0U; picked < k; picked++) {
		size_t end = len - (k - picked); /* inclusive */
		size_t best_pos = start;
		char best_ch = '0';

		for (size_t j = start; j <= end; j++) {
			char ch = s[j];
			if (ch > best_ch) {
				best_ch = ch;
				best_pos = j;
				if (best_ch == '9') {
				}
			}
		}

		val = val * 10U + (u64)(best_ch - '0');
		start = best_pos + 1U;
	}

	return val;
}
static u64
part1(char banks[][MAX_WIDTH + 1U], size_t n)
{
	u64 total = 0U;
	for (size_t i = 0U; i < n; i++) {
		u32 b = best_bank(banks[i]);
		total += (u64)b;
	}
	return total;
}

static u64
part2(char banks[][MAX_WIDTH + 1U], size_t n)
{
	u64 total = 0U;
	for (size_t i = 0U; i < n; i++) {
		u64 b = output_joltage(banks[i]);
		total += b;
	}
	return total;
}

static size_t
load_banks(char banks[][MAX_WIDTH + 1U], size_t cap)
{
	char line[MAX_WIDTH + 4U];
	size_t n = 0U;

	if (banks == NULL || cap == 0U) {
		return 0U;
	}
	while (read_line(stdin, line, sizeof line)) {
		if (is_blank_line(line)) {
			continue;
		}
		if (n >= cap) {
			fprintf(stderr, "too many banks (cap=%d)\n", MAX_BANKS);
			exit(EXIT_FAILURE);
		}
		size_t len = strlen(line);
		if (len > MAX_WIDTH) {
			fprintf(stderr, "bank line too long (%zu > %d)\n", len,
			    MAX_WIDTH);
			exit(EXIT_FAILURE);
		}
		memcpy(banks[n], line, len + 1U);
		n++;
	}
	return n;
}

int
main(void)
{
	static char banks[MAX_BANKS][MAX_WIDTH + 1U];
	size_t bank_count;
	u64 part_1;
	u64 part_2;
	u64 sample_1;
	u64 sample_2;

	sample_1 = part1(sample_banks, SAMPLE_BANK_COUNT);
	sample_2 = part2(sample_banks, SAMPLE_BANK_COUNT);
	printf("Part1 sample: %" PRIu64 "\n", sample_1);
	printf("Part1 sample: %" PRIu64 "\n", sample_2);

	bank_count = load_banks(banks, MAX_BANKS);
	part_1 = part1(banks, bank_count);
	part_2 = part2(banks, bank_count);
	printf("Part1: %" PRIu64 "\n", part_1);
	printf("Part2: %" PRIu64 "\n", part_2);
	return EXIT_SUCCESS;
}
