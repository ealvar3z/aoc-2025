package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

type Graph map[string][]string

type stateKey struct {
	node string
	mask uint8
}

func main() {
	g := readGraph(os.Stdin)

	start := "svr"
	target := "out"
	dac := "dac"
	fft := "fft"

	if _, ok := g[start]; !ok {
		fmt.Println("0")
		return
	}
	memo := make(map[stateKey]uint64)
	visiting := make(map[stateKey]bool)

	paths := countPathsWithDevices(g, start, target, dac, fft, 0, memo, visiting)
	fmt.Println(paths)
}

// readGraph parses lines:
// into adjacency list entries: aaa -> [you, hhh]
func readGraph(f *os.File) Graph {
	g := make(Graph)
	sc := bufio.NewScanner(f)

	for sc.Scan() {
		line := strings.TrimSpace(sc.Text())
		if line == "" {
			continue
		}

		parts := strings.SplitN(line, ":", 2)
		if len(parts) != 2 {
			continue
		}
		src := strings.TrimSpace(parts[0])
		right := strings.TrimSpace(parts[1])

		if src == "" {
			continue
		}

		var dests []string
		if right != "" {
			dests = strings.Fields(right)
		}

		g[src] = append(g[src], dests...)
	}

	if err := sc.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "read error: %v\n", err)
		os.Exit(1)
	}

	return g
}

// countPathsWithDevices counts paths from `node` to `target`,
// where each path must visit both `dac` and `fft` (in any order).
// `mask` encodes which of them we have visited so far:
//   bit 0 -> saw dac
//   bit 1 -> saw fft
func countPathsWithDevices(
	g Graph,
	node, target, dac, fft string,
	mask uint8,
	memo map[stateKey]uint64,
	visiting map[stateKey]bool,
) uint64 {
	if node == dac {
		mask |= 1 << 0
	}
	if node == fft {
		mask |= 1 << 1
	}

	key := stateKey{node: node, mask: mask}

	// Base case: reached the output.
	if node == target {
		if mask == 0b11 {
			return 1
		}
		return 0
	}

	// Memoized?
	if v, ok := memo[key]; ok {
		return v
	}

	if visiting[key] {
		return 0
	}
	visiting[key] = true

	var total uint64
	for _, next := range g[node] {
		total += countPathsWithDevices(g, next, target, dac, fft, mask, memo, visiting)
	}

	visiting[key] = false
	memo[key] = total
	return total
}
