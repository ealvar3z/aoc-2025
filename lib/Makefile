CC      = cc
CFLAGS  = -std=c23 -Wall -Wextra -Wpedantic \
	-D_POSIX_C_SOURCE=200809L

BIN = test_aoc

all: $(BIN)

test_aoc: test_aoc.c aoc.h
	$(CC) $(CFLAGS) test_aoc.c -o test_aoc

clean:
	rm -f $(BIN) *.o

.PHONY: all clean
