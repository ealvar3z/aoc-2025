#!/usr/bin/env perl
use v5.42;
use lib 'lib';
use aoc qw/read_nonblank_lines/;

if (@ARGV && -f $ARGV[0]) {
    open STDIN, '<', $ARGV[0]
      or die "cannot open $ARGV[0]: $!";
} elsif (-t *STDIN) {
    *STDIN = *DATA;
}

sub sum_exact2_in_range ($L, $R, $maxR) {
    my $sum = 0;

    # k = length of block S, total length = 2k digits
    my $k = 1;
    while (10 ** (2 * $k - 1) <= $maxR) {   # smallest 2k-digit number <= maxR
        my $M = 10 ** $k + 1;               # N = x * (10^k + 1)

        # x must satisfy L <= xM <= R
        my $x_range_low  = int( ($L + $M - 1) / $M );  # ceil(L/M)
        my $x_range_high = int(  $R / $M );            # floor(R/M)

        # x must be k-digit: 10^(k-1) <= x <= 10^k - 1
        my $x_min = 10 ** ($k - 1);
        my $x_max = 10 ** $k - 1;

        my $A = $x_range_low  > $x_min ? $x_range_low  : $x_min;
        my $B = $x_range_high < $x_max ? $x_range_high : $x_max;

        if ($A <= $B) {
            my $cnt   = $B - $A + 1;                # number of x
            my $sum_x = ($A + $B) * $cnt / 2;       # sum x=A..B
            $sum += $M * $sum_x;                    # sum N = xM
        }

        $k++;
    }

    return $sum;
}

sub generate_repeated_candidates ($maxR) {
    my %seen;
    my @cand;

    my $k = 1;
    # smallest length is 2k digits; stop once that exceeds maxR
    while (10 ** (2 * $k - 1) <= $maxR) {
        my $base  = 10 ** $k;
        my $x_min = 10 ** ($k - 1);   # smallest k-digit block

        my $t = 2;
        while (1) {
            # M_{k,t} = 10^{k(t-1)} + ... + 1 = (base^t - 1) / (base - 1)
            my $M = ( $base ** $t - 1 ) / ( $base - 1 );

            my $lowN = $x_min * $M;   # smallest N for this (k,t)
            last if $lowN > $maxR;    # larger t will only increase N, so break

            my $x_max     = 10 ** $k - 1;        # largest k-digit block
            my $x_max_cap = int($maxR / $M);     # cap by N <= maxR
            $x_max_cap = $x_max if $x_max < $x_max_cap;

            for (my $x = $x_min; $x <= $x_max_cap; $x++) {
                my $N = $x * $M;
                last if $N > $maxR;

                next if $seen{$N}++;
                push @cand, $N;
            }

            $t++;
        }

        $k++;
    }

    @cand = sort { $a <=> $b } @cand;
    return \@cand;
}

sub prefix_sums ($arr) {
    my @p = (0);
    my $s = 0;
    for my $v (@$arr) {
        $s += $v;
        push @p, $s;
    }
    return \@p;
}

sub lower_bound ($arr, $x) {
    my ($l, $r) = (0, scalar(@$arr));
    while ($l < $r) {
        my $m = int(($l + $r) / 2);
        if ($arr->[$m] < $x) { $l = $m + 1 }
        else                 { $r = $m     }
    }
    return $l;
}

sub upper_bound ($arr, $x) {
    my ($l, $r) = (0, scalar(@$arr));
    while ($l < $r) {
        my $m = int(($l + $r) / 2);
        if ($arr->[$m] <= $x) { $l = $m + 1 }
        else                  { $r = $m     }
    }
    return $l;
}

my $lines = read_nonblank_lines();   

my @ranges;
for my $line (@$lines) {
    for my $tok (split /,/, $line) {
        next unless length $tok;
        my ($L, $R) = split /-/, $tok;
        push @ranges, [ $L + 0, $R + 0 ];
    }
}

my $maxR = 0;
for my $r (@ranges) {
    $maxR = $r->[1] if $r->[1] > $maxR;
}

# Part 1 
my $part1 = 0;
for my $r (@ranges) {
    my ($L, $R) = @$r;
    $part1 += sum_exact2_in_range($L, $R, $maxR);
}

# Part 2
my $cands = generate_repeated_candidates($maxR);
my $pref  = prefix_sums($cands);

my $part2 = 0;
for my $r (@ranges) {
    my ($L, $R) = @$r;

    my $lo = lower_bound($cands, $L);
    my $hi = upper_bound($cands, $R);

    $part2 += $pref->[$hi] - $pref->[$lo];
}

print "Part 1: $part1\n";
print "Part 2: $part2\n";

__DATA__
11-22,95-115,998-1012,1188511880-1188511890,222220-222224,
1698522-1698528,446443-446449,38593856-38593862,565653-565659,
824824821-824824827,2121212118-2121212124
