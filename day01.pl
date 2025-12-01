#!/usr/bin/env perl
use strict;
use warnings;

# Advent of Code Day 01

my $pos = 50;
my $count_0 = 0;
my $MOD = 100;
my $part1 = 0;
my $part2 = 0;

my $fh;
if (-t STDIN) {
    $fh = *DATA;
} else {         
    $fh = *STDIN; 
}

while (my $line = <$fh>) {
    chomp $line;
    next if $line eq '';

    my $dir = substr($line, 0, 1);
    my $dist = substr($line, 1);
    my $hits = 0;
    my $s = $pos % $MOD; # normalize start position
    my $d = $dist;
    if ($d > 0) {
        my $t0;
        if ($dir eq 'R') {
            # Right: positions visited are (s+1), (s+2), ..., (s+d) % MOD
            # t is in [1..d] such that (s + t) % MOD == 0
            # => t == -s (% MOD)
            $t0 = ($MOD - $s) % $MOD;   # in 0..99
            $t0 = $MOD if $t0 == 0;     # smallest positive t
        } else {
            # Left: positions visited are (s-1), (s-2), ..., (s-d) % MOD
            # We need t in [1..d] such that (s - t) % MOD == 0
            # => t ≡ s (% MOD)
            $t0 = $s % $MOD;            # in 0..99
            print "Left: $t0\n";
            $t0 = $MOD if $t0 == 0;     # smallest positive t
        }

        if ($t0 <= $d) {
            # First hit at t0, then every MOD steps after that
            $hits = 1 + int( ($d - $t0) / $MOD );
        }
    }

    $part2 += $hits;

    if ($dir eq 'L') {
        $pos = ($pos - $dist) % $MOD;
    } else {  # 'R'
        $pos = ($pos + $dist) % $MOD;
    }

    $part1++ if $pos == 0;
}

print "Part 1: $part1\n";
print "Part 2: $part2\n";

__DATA__
L68
L30
R48
L5
R60
L55
L1
L99
R14
L82
