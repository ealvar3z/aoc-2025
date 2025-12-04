#!/usr/bin/env perl

use v5.42;
use Mojo::Util      qw/trim/;
use Mojo::Collection qw/c/;
use Mojo::File      qw/path/;

sub read_paper () {
    my $fh;
    if (@ARGV && -f $ARGV[0]) {
        $fh = path($ARGV[0])->open('<');
    } else {
        $fh = (-t *STDIN) ? *DATA : *STDIN;
    }

    my @lines;
    while (my $line = <$fh>) {
        chomp $line;
        push @lines, $line;
    }

    my $rows = c(@lines)
        ->map(sub ($l) { trim($l) })
        ->grep(sub ($l) { length($l) })
        ->to_array;

    die "no paper diagram\n" unless @$rows;

    my $w = length $rows->[0];
    die "inconsistent row widths\n"
        if c(@$rows)->grep(sub ($l) { length($l) != $w })->size;

    return $rows;   # arrayref of strings, each row
}

# Part 1
sub count_accessible ($paper) {
    my $H = @$paper;
    my $W = length $paper->[0];

    my @dr = (-1, -1, -1,  0, 0, 1, 1, 1);
    my @dc = (-1,  0,  1, -1, 1,-1, 0, 1);

    my $free = 0;

    for my $r (0 .. $H - 1) {
        for my $c (0 .. $W - 1) {
            next unless substr($paper->[$r], $c, 1) eq '@';

            my $adj = 0;
            for my $k (0 .. $#dr) {
                my $nr = $r + $dr[$k];
                my $nc = $c + $dc[$k];
                next if $nr < 0 || $nr >= $H || $nc < 0 || $nc >= $W;
                $adj++ if substr($paper->[$nr], $nc, 1) eq '@';
            }

            $free++ if $adj < 4;
        }
    }

    return $free;
}

# Part 2
sub count_removed ($paper) {
    my $H = @$paper;
    my $W = length $paper->[0];

    my @dr = (-1, -1, -1,  0, 0, 1, 1, 1);
    my @dc = (-1,  0,  1, -1, 1,-1, 0, 1);

    my @adj  = map { [ (0) x $W ] } 0 .. $H - 1;
    my @gone = map { [ (0) x $W ] } 0 .. $H - 1;

    for my $r (0 .. $H - 1) {
        for my $c (0 .. $W - 1) {
            next unless substr($paper->[$r], $c, 1) eq '@';
            my $cnt = 0;
            for my $k (0 .. $#dr) {
                my $nr = $r + $dr[$k];
                my $nc = $c + $dc[$k];
                next if $nr < 0 || $nr >= $H || $nc < 0 || $nc >= $W;
                $cnt++ if substr($paper->[$nr], $nc, 1) eq '@';
            }
            $adj[$r][$c] = $cnt;
        }
    }

    # remover
    my @queue;
    for my $r (0 .. $H - 1) {
        for my $c (0 .. $W - 1) {
            next unless substr($paper->[$r], $c, 1) eq '@';
            push @queue, [$r, $c] if $adj[$r][$c] < 4;
        }
    }

    my $removed = 0;

    while (@queue) {
        my ($r, $c) = @{ shift @queue };

        next if $gone[$r][$c];
        next unless substr($paper->[$r], $c, 1) eq '@';

        $gone[$r][$c] = 1;
        $removed++;

        # updater
        for my $k (0 .. $#dr) {
            my $nr = $r + $dr[$k];
            my $nc = $c + $dc[$k];
            next if $nr < 0 || $nr >= $H || $nc < 0 || $nc >= $W;
            next unless substr($paper->[$nr], $nc, 1) eq '@';
            next if $gone[$nr][$nc];

            $adj[$nr][$nc]-- if $adj[$nr][$nc] > 0;

            if ($adj[$nr][$nc] < 4) {
                push @queue, [$nr, $nc];
            }
        }
    }

    return $removed;
}

sub main () {
    my $paper = read_paper();

    my $part1 = count_accessible($paper);
    my $part2 = count_removed($paper);

    say "Part1: $part1";
    say "Part2: $part2";
}

main();

__DATA__
..@@.@@@@.
@@@.@.@.@@
@@@@@.@.@@
@.@@@@..@.
@@.@@@@.@@
.@@@@@@@.@
.@.@.@.@@@
@.@@@.@@@@
.@@@@@@@@.
@.@.@@@.@.
