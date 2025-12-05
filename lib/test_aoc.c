#include "aoc.h"
#include <assert.h>
#include <stdio.h>

static void
test_swap_int(void)
{
	int a = 10;
	int b = -5;

	SWAP(a, b);
	assert(a == -5);
	assert(b == 10);

	SWAP(a, b);
	assert(a == 10);
	assert(b == -5);
}

static void
test_swap_ptr(void)
{
	int x = 7;
	int y = 9;
	int *px = &x;
	int *py = &y;

	SWAP_PTR(px, py);
	assert(*px == 9);
	assert(*py == 7);

	SWAP_PTR(px, py);
	assert(*px == 7);
	assert(*py == 9);
}

struct Pair {
	int k;
	int v;
};

static void
test_swap_struct(void)
{
	struct Pair p1 = {1, 100};
	struct Pair p2 = {2, 200};

	SWAP(p1, p2);

	assert(p1.k == 2 && p1.v == 200);
	assert(p2.k == 1 && p2.v == 100);
}

static void
test_min_max(void)
{
	assert(MIN(3, 5) == 3);
	assert(MIN(-10, -2) == -10);
	assert(MIN(0, 0) == 0);

	assert(MAX(3, 5) == 5);
	assert(MAX(-10, -2) == -2);
	assert(MAX(0, 0) == 0);

	/* mix of positive and negative */
	int a = -3;
	int b = 7;
	assert(MIN(a, b) == -3);
	assert(MAX(a, b) == 7);
}

static void
test_clamp(void)
{
	/* inside range */
	assert(CLAMP(5, 0, 10) == 5);

	/* below range */
	assert(CLAMP(-5, 0, 10) == 0);

	/* above range */
	assert(CLAMP(25, 0, 10) == 10);

	/* exact boundaries */
	assert(CLAMP(0, 0, 10) == 0);
	assert(CLAMP(10, 0, 10) == 10);

	/* symmetric range */
	assert(CLAMP(-3, -5, 5) == -3);
	assert(CLAMP(-10, -5, 5) == -5);
	assert(CLAMP(10, -5, 5) == 5);
}

int
main(void)
{
	printf("Running Integer tests...\n");

	test_swap_int();
	printf("  SWAP(int,int)   OK\n");

	test_swap_ptr();
	printf("  SWAP_PTR(int*)  OK\n");

	test_swap_struct();
	printf("  SWAP(struct)    OK\n");

	test_min_max();
	printf("  MIN / MAX       OK\n");

	test_clamp();
	printf("  CLAMP           OK\n");

	printf("All tests passed.\n");
	return 0;
}
