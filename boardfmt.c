#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "trace.h"
#include "stream.h"
#include "board.h"
#include "boardfmt.h"


int
pos_parse(Pos *pos, Stream *s)
{
	int c;

	c = stream_getc(s);
	if (c < '1' || c > '9') {
		error("illegal position col: `%c'.", c);
		return -1;
	}

	pos->col = c - '0'; 

	c = stream_getc(s);
	if (c < 'a' || c > 'z') {
		error("illegal position row: `%c'.", c);
		return -1;
	}

	pos->row = c - 'a' + 1;

	return 0;
}

int
piece_parse(int *piece, Stream *s)
{
	const char *list = "PLNBRSGK";
	int i;
	int c;

	c = stream_getc(s);
        for (i = 0; list[i] != '\0'; i++) {
                if (list[i] == c) {
                        *piece = i + 1;
                        return 0;
                }
        }

	error("illegal piece: `%c'.", c);
        return -1;
}

int
move_parse(Move *move, Stream *s)
{
	int c;

	if ((c = stream_peekc(s)) == EOF) {
		error("eof");
		return -1;
	}

	move->flags = 0;
	move->get_piece = NONE;

	if (isdigit(c)) {
		if (pos_parse(&move->src, s) != 0) {
			error("Illegal src position.");
			return -1;
		}

		if (pos_parse(&move->dst, s) != 0) {
			error("Illegal dst position.");
			return -1;
		}

		move->piece = NONE;

		if ((c = stream_getc(s)) == '+')
			dopromote_set(move);
		else
			stream_ungetc(c, s);
	} else {
		if (piece_parse(&move->piece, s) != 0) {
			error("Illegal piece.");
			return -1;
		}

		if ((c = stream_getc(s)) != '*')
			return -1;

		fromstock_set(move);

		move->src.col = -1;
		move->src.row = -1;

		if (pos_parse(&move->dst, s) != 0) {
			error("Illegal dst position.");
			return -1;
		}
	}

	return 0;
}


const char *
side_name(int side)
{
	const char *list[] = { "black", "white" };

	if (side < 0 || side >= NSIDE) {
		warn("Unknown side: %#x", side);
		return "-";
	}

	return list[side];
}

int
piece_name(Stream *s, int piece)
{
	const char *list = ".PLNBRSGK";
	int type;
	int len;

	type = type_get(piece);
	type = demote(type);
	if (type < 0 || type >= NPIECE) {
		warn("Unknown piece: %#x", piece);
		stream_putc(s, '-');
		return 1;
	}

	len = 1;
	if (ispromoted(piece)) {
		stream_putc(s, '+');
		len++;
	}

	if (side_get(piece) == WHITE)
		stream_putc(s, tolower(list[type] & 0xff));
	else
		stream_putc(s, list[type]);

	return len;
}

void
board_show(Stream *s, const Board *board)
{
	int row, col;
	int side;
	int piece;
	int len;
	int n;

	for (col = 0; col < board->ncol; col++) {
		stream_fmtputs(s, "%d", board->ncol - col);
		if (col == board->ncol - 1)
			stream_putc(s, '\n');
		else
			stream_putc(s, ' ');
	}

	for (row = 0; row < board->nrow; row++) {
		for (col = 0; col < board->ncol; col++) {
			len = piece_name(s, board->cell[row][col]);
			if (len < 2)
				stream_putc(s, ' ');
		}
		stream_putc(s, ' ');
		stream_putc(s, row + 'a');
		stream_putc(s, '\n');
	}

	for (side = 0; side < NSIDE; side++) {
		stream_puts(s, side_name(side));
		stream_putc(s, ':');
		for (piece = 0; piece < NPIECE; piece++) {
			n = board->num[side][piece];
			if (n > 0) {
				stream_putc(s, ' ');
				piece_name(s, piece);
				stream_fmtputs(s, "x%d", n);
			}
		}
		stream_putc(s, '\n');
	}
}

void
board_show_reverse(Stream *s, const Board *board)
{
	int row, col;
	int side;
	int piece;
	int len;
	int n;

	for (col = board->ncol - 1; col >= 0; col--) {
		stream_fmtputs(s, "%d", board->ncol - col);
		if (col == 0)
			stream_putc(s, '\n');
		else
			stream_putc(s, ' ');
	}

	for (row = board->nrow - 1; row >= 0; row--) {
		for (col = board->ncol - 1; col >= 0; col--) {
			len = piece_name(s, board->cell[row][col]);
			if (len < 2)
				stream_putc(s, ' ');
		}
		stream_putc(s, ' ');
		stream_putc(s, row + 'a');
		stream_putc(s, '\n');
	}

	for (side = 0; side < NSIDE; side++) {
		stream_puts(s, side_name(side));
		stream_putc(s, ':');
		for (piece = 0; piece < NPIECE; piece++) {
			n = board->num[side][piece];
			if (n > 0) {
				stream_putc(s, ' ');
				piece_name(s, piece);
				stream_fmtputs(s, "x%d", n);
			}
		}
		stream_putc(s, '\n');
	}
}

void
move_show(Stream *s, const Move *move)
{
	int type;

	if (onboard(move)) {
		stream_fmtputs(s, "%d", move->src.col);
		stream_putc(s, move->src.row + 'a' - 1);
		stream_fmtputs(s, "%d", move->dst.col);
		stream_putc(s, move->dst.row + 'a' - 1);
		if (dopromote(move)) {
			stream_putc(s, '+');
		}
	} else {
		type = type_get(move->piece);
		piece_name(s, type);
		stream_fmtputs(s, "*%d%c",
		       move->dst.col, move->dst.row + 'a' - 1);
	}
}
