#!/usr/bin/env perl
use v5.42;

use Mojo::Util      qw/trim/;
use Mojo::Collection qw/c/;

sub read_db () {
    my $fh = -t *STDIN ? *DATA : *STDIN;

    my @raw;
    while (my $line = <$fh>) {
        chomp $line;
        push @raw, $line;
    }

    my $blank_idx = -1;
    for my $i (0 .. $#raw) {
        if (trim($raw[$i]) eq '') {
            $blank_idx = $i;
            last;
        }
    }

    my @range_lines;
    my @id_lines;

    if ($blank_idx == -1) {
        @range_lines = @raw;
        @id_lines    = ();
    } else {
        @range_lines = @raw[0 .. $blank_idx - 1];
        @id_lines    = @raw[$blank_idx + 1 .. $#raw];
    }

    my $ranges = c(@range_lines)
        ->map(sub ($l) { trim($l) })
        ->grep(sub ($l) { length($l) })
        ->to_array;

    my $ids = c(@id_lines)
        ->map(sub ($l) { trim($l) })
        ->grep(sub ($l) { length($l) })
        ->to_array;

    return ($ranges, $ids);    # arrayrefs of strings
}

sub parse_ranges ($lines) {
    my @ranges;

    for my $l (@$lines) {
        if ($l =~ /^\s*(\d+)\s*-\s*(\d+)\s*$/) {
            my ($lo, $hi) = ($1 + 0, $2 + 0);
            ($lo, $hi) = ($hi, $lo) if $hi < $lo;
            push @ranges, [$lo, $hi];
        } else {
            die "Bad range line: '$l'\n";
        }
    }

    return \@ranges;
}

sub parse_ids ($lines) {
    my @ids;

    for my $l (@$lines) {
        next if $l eq '';
        if ($l =~ /^\s*(\d+)\s*$/) {
            push @ids, $1 + 0;
        } else {
            die "Bad ID line: '$l'\n";
        }
    }

    return \@ids;
}

sub merge_ranges ($ranges) {
    return [] unless @$ranges;

    my @sorted = sort {
        $a->[0] <=> $b->[0] || $a->[1] <=> $b->[1]
    } @$ranges;

    my @out;
    my ($cur_lo, $cur_hi) = @{$sorted[0]};

    for my $i (1 .. $#sorted) {
        my ($lo, $hi) = @{$sorted[$i]};
        if ($lo <= $cur_hi + 1) {
            # Overlap or touch: extend
            $cur_hi = $hi if $hi > $cur_hi;
        } else {
            # Disjoint: push current, start new
            push @out, [$cur_lo, $cur_hi];
            ($cur_lo, $cur_hi) = ($lo, $hi);
        }
    }

    push @out, [$cur_lo, $cur_hi];
    return \@out;
}

sub is_fresh ($id, $ranges) {
    my $lo = 0;
    my $hi = $#$ranges;

    while ($lo <= $hi) {
        my $mid = int(($lo + $hi) / 2);
        my ($a, $b) = @{$ranges->[$mid]};

        if    ($id < $a) { $hi = $mid - 1; }
        elsif ($id > $b) { $lo = $mid + 1; }
        else             { return 1; }
    }
    return 0;
}

sub main () {
    my ($range_lines, $id_lines) = read_db();
    my $ranges_raw  = parse_ranges($range_lines);
    my $ids         = parse_ids($id_lines);

    my $merged = merge_ranges($ranges_raw);

    my $part1 = 0;
    for my $id (@$ids) {
        $part1++ if is_fresh($id, $merged);
    }

    my $part2 = 0;
    c(@$merged)->each(sub ($r, $idx) {
        my ($lo, $hi) = @$r;
        $part2 += $hi - $lo + 1;
    });

    say "Part1: $part1";
    say "Part2: $part2";
}

main();

__DATA__
3-5
10-14
16-20
12-18

1
5
8
11
17
32
