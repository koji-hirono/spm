#! /usr/bin/env perl
#
#  kif形式からGeorge Hodgesの棋譜の形式に変換する
#

$n = 0;
while (<>) {
	s/\x0d\0a$//;
	s/\x0d$//;
	s/\x0a$//;

	s/^\s+//;
	s/\s+$//;

	s/１/1/g;
	s/２/2/g;
	s/３/3/g;
	s/４/4/g;
	s/５/5/g;
	s/６/6/g;
	s/７/7/g;
	s/８/8/g;
	s/９/9/g;

	s/一/a/g;
	s/二/b/g;
	s/三/c/g;
	s/四/d/g;
	s/五/e/g;
	s/六/f/g;
	s/七/g/g;
	s/八/h/g;
	s/九/i/g;

	s/と/+P/g;
	s/成香/+L/g;
	s/成桂/+N/g;
	s/竜/+R/g;
	s/龍/+R/g;
	s/馬/+B/g;
	s/成銀/+S/g;

#	s/全/+S/g;
#	s/圭/+N/g;
#	s/杏/+L/g;

	s/歩/P/g;
	s/香/L/g;
	s/桂/N/g;
	s/角/B/g;
	s/飛/R/g;
	s/銀/S/g;
	s/金/G/g;
	s/王/K/g;
	s/玉/K/g;

	s/　//g;
	s/不成//g;
	s/\(//g;
	s/\)//g;
	s/▼//g;
	s/▽//g;
	s/▲//g;
	s/△//g;

	s/成/+/g;
	s/打/*/g;

	s/同/$dst/g;

	s/投了/Resign/g;
	s/詰/Resign/g;

	if (/^\d+\s+Resign/) {
#		print "Resign\n";
	} elsif (($d, $p, $s, $s2) = /^\d+\.?\s+([0-9][a-i])([+]?\w[*+]?)([0-9])?([0-9])?/) {
		$dst = $d;

		$s .= ('a' .. 'i')[$s2 - 1];

#		print "$_ => ";
		if ($p =~ /\*/) {
			print "$p$d\n";
		} elsif ($p =~ /\+$/) {
			print "$s$d+\n";
		} else {
			print "$s$d\n";
		}
	}
}
