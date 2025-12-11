package main

import (
	"bufio"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
)

type Pt struct {
	x, y int64
	row  int // compressed y index
	col  int // compressed x index
}

func abs64(a int64) int64 {
	if a < 0 {
		return -a
	}
	return a
}

func main() {
	points := readPoints()

	if len(points) < 2 {
		fmt.Println("Part 1:", 0)
		fmt.Println("Part 2:", 0)
		return
	}

	// ----- Part 1: largest rectangle ignoring greens -----
	part1 := int64(0)
	n := len(points)
	for i := 0; i < n; i++ {
		for j := i + 1; j < n; j++ {
			dx := abs64(points[i].x - points[j].x)
			dy := abs64(points[i].y - points[j].y)
			area := (dx + 1) * (dy + 1)
			if area > part1 {
				part1 = area
			}
		}
	}

	// ----- Part 2: use red+green only -----
	compressCoords(points)
	part2 := solvePart2(points)

	fmt.Println("Part 1:", part1)
	fmt.Println("Part 2:", part2)
}

func readPoints() []Pt {
	in := bufio.NewScanner(os.Stdin)
	pts := make([]Pt, 0, 1024)

	for in.Scan() {
		line := strings.TrimSpace(in.Text())
		if line == "" {
			continue
		}
		parts := strings.Split(line, ",")
		if len(parts) != 2 {
			fmt.Fprintf(os.Stderr, "bad line: %q\n", line)
			os.Exit(1)
		}
		x, err1 := strconv.ParseInt(strings.TrimSpace(parts[0]), 10, 64)
		y, err2 := strconv.ParseInt(strings.TrimSpace(parts[1]), 10, 64)
		if err1 != nil || err2 != nil {
			fmt.Fprintf(os.Stderr, "bad line: %q\n", line)
			os.Exit(1)
		}
		pts = append(pts, Pt{x: x, y: y})
	}
	if err := in.Err(); err != nil {
		fmt.Fprintln(os.Stderr, "scan error:", err)
		os.Exit(1)
	}
	return pts
}

func compressCoords(pts []Pt) {
	n := len(pts)
	xs := make([]int64, n)
	ys := make([]int64, n)
	for i, p := range pts {
		xs[i] = p.x
		ys[i] = p.y
	}
	sort.Slice(xs, func(i, j int) bool { return xs[i] < xs[j] })
	sort.Slice(ys, func(i, j int) bool { return ys[i] < ys[j] })

	xs = unique(xs)
	ys = unique(ys)

	xIndex := make(map[int64]int, len(xs))
	yIndex := make(map[int64]int, len(ys))
	for i, v := range xs {
		xIndex[v] = i
	}
	for i, v := range ys {
		yIndex[v] = i
	}

	for i := range pts {
		pts[i].col = xIndex[pts[i].x] // compressed x
		pts[i].row = yIndex[pts[i].y] // compressed y
	}
}

func unique(a []int64) []int64 {
	if len(a) == 0 {
		return a
	}
	out := a[:1]
	for i := 1; i < len(a); i++ {
		if a[i] != a[i-1] {
			out = append(out, a[i])
		}
	}
	return out
}

func solvePart2(pts []Pt) int64 {
	n := len(pts)

	// Dimensions in compressed space
	maxRow, maxCol := 0, 0
	for _, p := range pts {
		if p.row > maxRow {
			maxRow = p.row
		}
		if p.col > maxCol {
			maxCol = p.col
		}
	}
	H := maxRow + 1
	W := maxCol + 1

	// Grid with one-cell border all around
	GH := H + 2
	GW := W + 2

	grid := make([][]byte, GH)
	used := make([][]bool, GH)
	for r := 0; r < GH; r++ {
		grid[r] = make([]byte, GW)
		used[r] = make([]bool, GW)
		for c := 0; c < GW; c++ {
			grid[r][c] = '.' // default: outside
		}
	}

	// Draw polygon edges: connect consecutive red points (including wrap),
	// using compressed coords shifted by +1 for the border margin.
	for i := 0; i < n; i++ {
		j := (i + 1) % n
		r1 := pts[i].row + 1
		c1 := pts[i].col + 1
		r2 := pts[j].row + 1
		c2 := pts[j].col + 1

		if r1 == r2 {
			// horizontal segment
			if c1 > c2 {
				c1, c2 = c2, c1
			}
			for c := c1; c <= c2; c++ {
				grid[r1][c] = '#'
			}
		} else if c1 == c2 {
			// vertical segment
			if r1 > r2 {
				r1, r2 = r2, r1
			}
			for r := r1; r <= r2; r++ {
				grid[r][c1] = '#'
			}
		} else {
			// This *should never happen* for valid AoC input.
			fmt.Fprintf(os.Stderr,
				"non-axis-aligned edge between points %d and %d\n"+
					"  original p[%d] = (%d,%d)\n"+
					"  original p[%d] = (%d,%d)\n",
				i, j,
				i, pts[i].x, pts[i].y,
				j, pts[j].x, pts[j].y,
			)
			os.Exit(1)
		}
	}

	// Flood-fill from outside border to find reachable '.' cells.
	// Any '.' that are NOT reached are interior → green.
	qr := make([]int, 0, GH*GW)
	qc := make([]int, 0, GH*GW)
	push := func(r, c int) {
		used[r][c] = true
		qr = append(qr, r)
		qc = append(qc, c)
	}

	// Enqueue all border cells that are '.'
	for c := 0; c < GW; c++ {
		if grid[0][c] == '.' && !used[0][c] {
			push(0, c)
		}
		if grid[GH-1][c] == '.' && !used[GH-1][c] {
			push(GH-1, c)
		}
	}
	for r := 0; r < GH; r++ {
		if grid[r][0] == '.' && !used[r][0] {
			push(r, 0)
		}
		if grid[r][GW-1] == '.' && !used[r][GW-1] {
			push(r, GW-1)
		}
	}

	dr := [4]int{-1, 1, 0, 0}
	dc := [4]int{0, 0, -1, 1}

	head := 0
	for head < len(qr) {
		r := qr[head]
		c := qc[head]
		head++

		for k := 0; k < 4; k++ {
			nr := r + dr[k]
			nc := c + dc[k]
			if nr < 0 || nr >= GH || nc < 0 || nc >= GW {
				continue
			}
			if used[nr][nc] {
				continue
			}
			if grid[nr][nc] != '.' {
				continue
			}
			push(nr, nc)
		}
	}

	// Any '.' not reached is interior → mark as 'G'
	for r := 0; r < GH; r++ {
		for c := 0; c < GW; c++ {
			if grid[r][c] == '.' && !used[r][c] {
				grid[r][c] = 'G'
			}
		}
	}

	// Build 2D prefix sums over interior region [0..H-1][0..W-1]
	// Count "bad" cells = outside '.'; allowed cells are '#' or 'G'.
	pf := make([][]int, H+1)
	for r := 0; r <= H; r++ {
		pf[r] = make([]int, W+1)
	}

	for r := 0; r < H; r++ {
		rowSum := 0
		for c := 0; c < W; c++ {
			if grid[r+1][c+1] == '.' {
				rowSum++
			}
			pf[r+1][c+1] = pf[r][c+1] + rowSum
		}
	}

	// Helper: sum of outside cells in compressed rectangle [r1..r2][c1..c2]
	rectBad := func(r1, c1, r2, c2 int) int {
		return pf[r2+1][c2+1] - pf[r1][c2+1] - pf[r2+1][c1] + pf[r1][c1]
	}

	// Part 2: for each pair of red tiles, check if the entire rectangle is
	// inside the red+green region (i.e., has no '.' in it).
	best2 := int64(0)
	for i := 0; i < n; i++ {
		for j := i + 1; j < n; j++ {
			dx := abs64(pts[i].x - pts[j].x)
			dy := abs64(pts[i].y - pts[j].y)
			area := (dx + 1) * (dy + 1)
			if area <= best2 {
				continue // prune
			}

			r1 := pts[i].row
			r2 := pts[j].row
			if r1 > r2 {
				r1, r2 = r2, r1
			}
			c1 := pts[i].col
			c2 := pts[j].col
			if c1 > c2 {
				c1, c2 = c2, c1
			}

			if rectBad(r1, c1, r2, c2) == 0 {
				best2 = area
			}
		}
	}

	return best2
}

