#! /usr/bin/env perl
#
#  Yahoo形式からkif形式に変換する
#

$n = 0;
while (<>) {
	s/\x0d\0a$//;
	s/\x0d$//;
	s/\x0a$//;

	next if ($_ eq '');

	s/▼//g;
	s/▽//g;
	s/▲//g;
	s/△//g;
	s/不成//g;
	s/竜/龍/g;

	if (my ($day) = /送信時刻：(\d+\/\d+\/\d+)/) {
		print "開始日時：$day\n";
		print "棋戦：Yahoo!\n";
		print "手合割：平手\n";
	} elsif (/(先|後)手：/) {
		print;
		print "\n";
	} elsif (my ($n, $move) = /^(\d+)\s*(.+)$/) {
		if ($n == 1) {
			print "手数----指手---------消費時間--\n";
		}
		printf "%4d %s\n", $n, $move;
	}
}
