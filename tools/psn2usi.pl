#! /usr/bin/env perl

$s = join '', <>;
$_ = $s;
s/\[.*\]//g;
s/\d+\.//g;
s/^\s+//;
s/\s+$//;
s/-//g;
s/x//g;
print STDERR "OUT: [", $_, "]\n";
@t = split /\s+/;
# print join("\n", @t), "\n";
foreach (@t) {
	if (/\*/) {
		print $_, "\n";
	} elsif (my ($m) = /\+?\w([1-9][a-i][1-9][a-i]\+?)/) {
		print $m, "\n";
	}
}
