#!/usr/bin/env perl
use v5.42;

use Mojo::Util      qw/trim/;
use Mojo::Collection qw/c/;

sub read_banks () {
    my $fh = -t *STDIN ? *DATA : *STDIN;

    my @lines;
    while (my $line = <$fh>) {
        chomp $line;
        push @lines, $line;
    }

    my $banks = c(@lines)
        ->map(sub ($l) { trim($l) })
        ->grep(sub ($l) { length $l })
        ->to_array;

    return $banks;   # arrayref of strings
}

# Part 1
sub best_bank ($s) {
    my $len = length $s;
    return 0 if $len < 2;

    my $best       = 0;
    my $best_right = substr($s, -1, 1) + 0;

    for (my $i = $len - 2; $i >= 0; $i--) {
        my $d   = substr($s, $i,  1) + 0;
        my $val = 10 * $d + $best_right;

        $best = $val if $val > $best;
        $best_right = $d if $d > $best_right;
    }

    return $best;
}

# Part 2
sub output_joltage ($s) {
    my $k   = 12;
    my $len = length $s;

    return 0 if $len == 0;

    if ($len <= $k) {
        return 0 + $s;
    }

    my $start = 0;
    my $out   = '';

    for (my $picked = 0; $picked < $k; $picked++) {
        my $end      = $len - ($k - $picked);  # inclusive
        my $best_ch  = '0';
        my $best_pos = $start;

        for (my $j = $start; $j <= $end; $j++) {
            my $ch = substr($s, $j, 1);
            if ($ch gt $best_ch) {
                $best_ch  = $ch;
                $best_pos = $j;
                last if $best_ch eq '9';
            }
        }

        $out  .= $best_ch;
        $start = $best_pos + 1;
    }

    return 0 + $out;
}

sub main () {
    my $banks = read_banks();  # arrayref of strings

    my $part1 = c(@$banks)
        ->map(sub ($b) { best_bank($b) })
        ->reduce(sub { $a + $b }) // 0;

    my $part2 = c(@$banks)
        ->map(sub ($b) { output_joltage($b) })
        ->reduce(sub { $a + $b }) // 0;

    say "Part1: $part1";
    say "Part2: $part2";
}

main();

__DATA__
987654321111111
811111111111119
234234234234278
818181911112111
