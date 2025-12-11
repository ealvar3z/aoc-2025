#!/usr/bin/env perl
use v5.42;
use Math::BigInt;
use Math::MatrixReal;

my $MAX_LIGHTS  = 20;
my $MAX_BUTTONS = 64;
my $MAX_CNTS    = 16;

# A "button" is just an arrayref of the indices it touches
# A "machine" is a hashref:
# {
#   lights_n      => L,
#   lights_target => bitmask (u64),
#   cnt_n         => n,
#   target        => [t0..t_{n-1}],
#   btns          => [ [idx...], [idx...] ... ],
# }

sub parse_pattern($line, $m) {
    my $p = $line;

    # seek '['
    $p =~ s/^\s*//;
    my $start = index($p, '[');
    die "Missing '[' in line: $line\n" if $start < 0;

    my $rest = substr($p, $start+1);
    my $end = index($rest, ']');
    die "Missing ']' in line: $line\n" if $end < 0;

    my $pat = substr($rest, 0, $end);  # contents between [ ]

    my $L = 0;
    my $mask = 0;
    for my $ch (split //, $pat) {
        next if $ch eq ' ' || $ch eq "\t";
        if ($ch eq '.' || $ch eq '#') {
            die "Too many lights (> $MAX_LIGHTS)\n" if $L >= $MAX_LIGHTS;
            if ($ch eq '#') {
                $mask |= (1 << $L);
            }
            $L++;
        } else {
            die "Invalid char '$ch' in pattern: $line\n";
        }
    }
    die "Empty pattern in line: $line\n" if $L <= 0;

    $m->{lights_n}      = $L;
    $m->{lights_target} = $mask;

    # substring after the closing ']'
    my $after = substr($rest, $end+1);
    return $after;
}

sub parse_buttons ($segment, $m) {
    my @btns;
    my $p = $segment;

    while ((my $pos = index($p, '(')) >= 0) {
        my $rest = substr($p, $pos+1);
        my $end  = index($rest, ')');
        die "Unterminated '(' in: $segment\n" if $end < 0;

        my $inside = substr($rest, 0, $end); # "0,3,4" etc
        my @idx;
        if ($inside =~ /\S/) {
            for my $tok (split /,/, $inside) {
                $tok =~ s/^\s+//;
                $tok =~ s/\s+$//;
                next if $tok eq '';
                die "Non-integer in button spec: $segment\n" unless $tok =~ /^-?\d+$/;
                push @idx, 0 + $tok;
            }
        }
        push @btns, \@idx if @idx;

        # chop up to after ')'
        $p = substr($rest, $end+1);
    }

    die "Too many buttons (> $MAX_BUTTONS)\n" if @btns > $MAX_BUTTONS;
    $m->{btns}  = \@btns;
    return $p; # remain until '{'
}

sub parse_targets ($segment, $m) {
    my $p = $segment;
    my $start = index($p, '{');
    die "Missing '{' with joltage requirements in: $segment\n" if $start < 0;

    my $rest = substr($p, $start+1);
    my $end  = index($rest, '}');
    die "Missing '}' in joltage part: $segment\n" if $end < 0;

    my $inside = substr($rest, 0, $end); # "3,5,4,7"
    my @vals;
    for my $tok (split /,/, $inside) {
        $tok =~ s/^\s+//;
        $tok =~ s/\s+$//;
        next if $tok eq '';
        die "Non-integer joltage in: $segment\n" unless $tok =~ /^-?\d+$/;
        push @vals, 0 + $tok;
    }
    die "No joltage requirements in: $segment\n" if @vals == 0;
    die "Too many counters (> $MAX_CNTS)\n" if @vals > $MAX_CNTS;

    $m->{target} = \@vals;
    $m->{cnt_n}  = scalar @vals;
}

sub parse_machine ($line) {
    my %m = (
        lights_n      => 0,
        lights_target => 0,
        cnt_n         => 0,
        target        => [],
        btns          => [],
    );

    my $after_pat = parse_pattern($line, \%m);
    my $after_btn = parse_buttons($after_pat, \%m);
    parse_targets($after_btn, \%m);

    # sanity: button indices must be in range for both lights and counters
    my $L = $m{lights_n};
    my $C = $m{cnt_n};
    for my $b (@{ $m{btns} }) {
        for my $idx (@$b) {
            if ($idx < 0 || $idx >= $L || $idx >= $C) {
                die sprintf(
                    "Button index %d out of range (lights=%d, cnt=%d) in line: %s\n",
                    $idx, $L, $C, $line
                );
            }
        }
    }

    return \%m;
}

# Part 1: BFS on bitmasks
sub min_presses_lights($m) {
    my $L      = $m->{lights_n};
    my $target = $m->{lights_target};

    return 0 if $target == 0;
    return -1 if $L <= 0 || $L > $MAX_LIGHTS;

    my $state_count = 1 << $L;

    # memoize 
    my @bm;
    for my $b (@{ $m->{btns} }) {
        my $mask = 0;
        for my $idx (@$b) {
            next if $idx < 0 || $idx >= $L;
            $mask |= (1 << $idx);
        }
        push @bm, $mask if $mask;
    }
    return -1 unless @bm;

    my @dist = (-1) x $state_count;
    my @q;
    my $head = 0;

    $dist[0] = 0;
    push @q, 0;

    while ($head < @q) {
        my $s = $q[$head++];
        my $d = $dist[$s];

        return $d if $s == $target;

        for my $mask (@bm) {
            my $ns = $s ^ $mask;
            next if $dist[$ns] != -1;
            $dist[$ns] = $d + 1;
            push @q, $ns;
        }
    }

    return -1;
}

# Part 2: linear algebra (Math::MatrixReal)

# Solve A x = b over reals, where
#   A is n x m (n = counters, m = buttons),
#   entries 0/1: A[i,j] = 1 if button j touches counter i.
sub min_presses_jolts($m) {
    my $n = $m->{cnt_n};
    my $btns = $m->{btns};
    my $t = $m->{target};
    my $mcols = @$btns;

    return -1 if $n <= 0 || $mcols <= 0;

    # Quick check: each positive target counter must be touched by some button.
    my @seen = (0) x $n;
    for my $j (0 .. $mcols - 1) {
        for my $idx (@{ $btns->[$j] }) {
            next if $idx < 0 || $idx >= $n;
            $seen[$idx] = 1;
        }
    }
    for my $i (0 .. $n-1) {
        if ($t->[$i] > 0 && !$seen[$i]) {
            return -1;
        }
    }

    my $A = Math::MatrixReal->new($n, $mcols);
    my $bvec = Math::MatrixReal->new($n, 1);

    for my $i (0 .. $n-1) {
        $bvec->assign($i+1, 1, $t->[$i]);
        for my $j (0 .. $mcols-1) {
            my $val = 0;
            for my $idx (@{ $btns->[$j] }) {
                if ($idx == $i) {
                    $val = 1;
                    last;
                }
            }
            $A->assign($i+1, $j+1, $val);
        }
    }

    # Augmented matrix M = [ A | b ]
    my $M = $A->augment($bvec);
    # Gaussian elimination to RREF-ish form
    my ($rows, $cols_aug) = $M->dim;
    my $cols = $cols_aug - 1;

    my @pivot_row_for_col = (-1) x $cols;
    my $rank = 0;
    my $lead = 1; # columns are 1-based in Math::MatrixReal (smh)

    while ($rank < $rows && $lead <= $cols) {
        my $pivot_row = 0;
        for my $r ($rank+1 .. $rows) {
            if ($M->element($r, $lead) != 0) {
                $pivot_row = $r;
                last;
            }
        }
        if (!$pivot_row) {
            $lead++;
            next;
        }

        if ($pivot_row != $rank+1) {
            $M = $M->interchange_rows($pivot_row, $rank+1);
        }

        my $pv = $M->element($rank+1, $lead);
        for my $c ($lead .. $cols_aug) {
            my $val = $M->element($rank+1, $c) / $pv;
            $M->assign($rank+1, $c, $val);
        }

        for my $r (1 .. $rows) {
            next if $r == $rank+1;
            my $factor = $M->element($r, $lead);
            next if $factor == 0;
            for my $c ($lead .. $cols_aug) {
                my $val = $M->element($r, $c)
                         - $factor * $M->element($rank+1, $c);
                $M->assign($r, $c, $val);
            }
        }

        $pivot_row_for_col[$lead-1] = $rank;  # 0-based store
        $rank++;
        $lead++;
    }

    # Check for inconsistency: row of zeros in A but non-zero RHS
    for my $r ($rank+1 .. $rows) {
        my $all_zero = 1;
        for my $c (1 .. $cols) {
            if ($M->element($r, $c) != 0) {
                $all_zero = 0;
                last;
            }
        }
        if ($all_zero && $M->element($r, $cols+1) != 0) {
            return -1;
        }
    }

    my @is_pivot_col = map { $_ >= 0 ? 1 : 0 } @pivot_row_for_col;
    my @free_cols;
    for my $j (0 .. $cols-1) {
        push @free_cols, $j if !$is_pivot_col[$j];
    }

    my @max_press;
    for my $j (0 .. $cols-1) {
        my $bound = 1_000_000_000; # big
        my $b = $btns->[$j];
        if (!@$b) {
            $max_press[$j] = 0;
            next;
        }
        for my $idx (@$b) {
            my $t_i = $t->[$idx];
            $bound = $t_i if $t_i < $bound;
        }
        $bound = 0 if $bound < 0;
        $max_press[$j] = $bound;
    }

    # If no free variables, we have a unique candidate solution
    if (!@free_cols) {
        my $sum = 0;
        for my $j (0 .. $cols-1) {
            my $pr = $pivot_row_for_col[$j];
            die "No pivot row for col $j\n" if $pr < 0;
            my $val = $M->element($pr+1, $cols+1); # RHS
            # pivot column is 1, so coefficient 1 there, others 0
            # so x_j = RHS
            return -1 if $val < 0;
            return -1 if $val > $max_press[$j];
            # check integer:
            return -1 if abs($val - int($val+0.5)) > 1e-9;
            $sum += int($val+0.5);
        }
        return $sum;
    }

    # my $best_sum = 9_223_372_036_854_775_807; # "infinity"
    my $best_sum = Math::BigInt->binf();
    my @x_sol = (0) x $cols;
    my @x_curr = (0) x $cols;

    # Small recursive DFS over free variables
    my $dfs;
    $dfs = sub ($idx, $partial_sum) {
        return if $partial_sum >= $best_sum;

        if ($idx == @free_cols) {
            my $sum = $partial_sum;
            for my $j (0 .. $cols-1) {
                next if !$is_pivot_col[$j]; # free variable already set
                my $r = $pivot_row_for_col[$j];
                my $rhs = $M->element($r+1, $cols+1); # RHS
                # subtract contributions of free vars: sum coef * x_free
                for my $fi (0 .. $#free_cols) {
                    my $fc = $free_cols[$fi];
                    my $coef = $M->element($r+1, $fc+1);
                    next if $coef == 0;
                    $rhs -= $coef * $x_curr[$fc];
                }
                # Check integer
                return if abs($rhs - int($rhs+0.5)) > 1e-9;
                my $xj = int($rhs+0.5);
                return if $xj < 0;
                return if $xj > $max_press[$j];
                $x_curr[$j] = $xj;
                $sum += $xj;
                return if $sum >= $best_sum;
            }
            if ($sum < $best_sum) {
                $best_sum = $sum;
                @x_sol = @x_curr;
            }
            return;
        }

        my $fc = $free_cols[$idx];
        my $maxv = $max_press[$fc];
        for my $v (0 .. $maxv) {
            $x_curr[$fc] = $v;
            $dfs->($idx+1, $partial_sum + $v);
        }
    };

    $dfs->(0, 0);

    # return -1 if $best_sum == 9_223_372_036_854_775_807;
    return -1 if $best_sum == Math::BigInt->binf();
    return $best_sum;
}

sub main() {
    my $total_p1 = 0;
    my $total_p2 = 0;
    my $i = 0;

    while (my $line = <STDIN>) {
        chomp $line;
        $line =~ s/\r$//;
        next if $line =~ /^\s*$/;

        my $m = parse_machine($line);

        my $p1 = min_presses_lights($m);
        die "Machine $i: Part1 impossible\n" if $p1 < 0;
        $total_p1 += $p1;

        my $p2 = min_presses_jolts($m);
        die "Machine $i: Part2 impossible\n" if $p2 < 0;
        $total_p2 += $p2;

        $i++;
    }

    say "Part1: $total_p1";
    say "Part2: $total_p2";
}

main();
