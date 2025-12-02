package aoc;
use v5.42;
use feature 'signatures';
no warnings 'experimental::signatures';

use Exporter 'import';
use Mojo::Util      qw/dumper trim/;
use Mojo::Collection qw/c/;

our @EXPORT_OK = qw(
  read_lines
  read_nonblank_lines
  read_grid
  read_blocks
  lines_c
  nonblank_c
  ints
  ints_all
  ints_all_c
  bfs_shortest
  dump
);


sub read_lines () {
  my @lines;
  while (<STDIN>) {
    chomp;
    push @lines, $_;
  }
  return \@lines;
}

sub read_nonblank_lines () {
  return [ grep { $_ ne '' } @{ read_lines() } ];
}

sub read_grid () {
  my $lines = read_nonblank_lines();
  return [ map { [ split // ] } @$lines ];
}

sub read_blocks () {
  my @blocks;
  my @current;

  while (<STDIN>) {
    chomp;
    if (/\S/) {
      push @current, $_;
    } else {
      if (@current) {
        push @blocks, [ @current ];
        @current = ();
      }
    }
  }

  push @blocks, [ @current ] if @current;
  return \@blocks;
}

sub lines_c () {
  return c( @{ read_lines() } );
}

sub nonblank_c () {
  return lines_c()->grep(sub { $_ ne '' });
}

sub ints ($line) {
  return split /\D+/, $line;
}

sub ints_all (@lines) {
  my @out;
  for my $line (@lines) {
    push @out, ($line =~ /-?\d+/g);
  }
  return wantarray ? @out : \@out;
}

sub ints_all_c ($lines) {
  # $lines can be:
  #  - a Mojo::Collection
  #  - or an arrayref of lines
  my $col = ref($lines) && $lines->isa('Mojo::Collection')
    ? $lines
    : c(@$lines);

  return $col
    ->map(sub { [ /-?\d+/g ] })
    ->flatten;
}

sub dump ($x) {
  print dumper($x);
}

sub bfs_shortest ($grid, $sr, $sc, $tr, $tc, $is_open) {
  return -1 unless $grid && @$grid;
  return -1 unless defined $is_open && ref($is_open) eq 'CODE';

  my $H = scalar @$grid;
  my $W = length $grid->[0];

  return -1 if $sr < 0 || $sr >= $H || $sc < 0 || $sc >= $W;
  return -1 if $tr < 0 || $tr >= $H || $tc < 0 || $tc >= $W;

  my $start_ch  = substr($grid->[$sr], $sc, 1);
  my $target_ch = substr($grid->[$tr], $tc, 1);
  return -1 unless $is_open->($start_ch);
  return -1 unless $is_open->($target_ch);

  my @dist;
  my @used;
  for my $r (0 .. $H - 1) {
    vec($used[$r], $_, 1) = 0 for 0 .. $W - 1;
    $dist[$r] = [ (-1) x $W ];
  }

  my (@qr, @qc);
  my $head = 0;
  my $tail = 0;

  vec($used[$sr], $sc, 1) = 1;
  $dist[$sr][$sc] = 0;
  $qr[$tail] = $sr;
  $qc[$tail] = $sc;
  $tail++;

  my @dr = (-1, 1, 0, 0);
  my @dc = (0, 0, -1, 1);

  while ($head < $tail) {
    my $r = $qr[$head];
    my $c = $qc[$head];
    $head++;

    return $dist[$r][$c] if $r == $tr && $c == $tc;

    for my $k (0 .. 3) {
      my $nr = $r + $dr[$k];
      my $nc = $c + $dc[$k];

      next if $nr < 0 || $nr >= $H || $nc < 0 || $nc >= $W;
      next if vec($used[$nr], $nc, 1);

      my $ch = substr($grid->[$nr], $nc, 1);
      next unless $is_open->($ch);

      vec($used[$nr], $nc, 1) = 1;
      $dist[$nr][$nc] = $dist[$r][$c] + 1;
      $qr[$tail] = $nr;
      $qc[$tail] = $nc;
      $tail++;
    }
  }

  return -1;
}

1;
