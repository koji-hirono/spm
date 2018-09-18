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
piece_to_stream(Stream *s, int piece)
{
	const char *list = ".PLNBRSGK";
	int type;
	int side;
	int len;

	type = type_get(piece);
	type = demote(type);
	if (type < 0 || type >= NPIECE) {
		warn("Unknown piece: %#x", piece);
		stream_putc(s, '-');
		return 1;
	}

	len = 0;
	side = side_get(piece);
	switch (side) {
	case BLACK:
		if (ispromoted(piece)) {
			stream_putc(s, '+');
			len++;
		}
		stream_putc(s, list[type]);
		len++;
		break;
	case WHITE:
		if (ispromoted(piece)) {
			stream_putc(s, '+');
			len++;
		}
		stream_putc(s, tolower(list[type] & 0xff));
		len++;
		break;
	default:
		warn("Unknown side: %#x", side);
		stream_putc(s, '-');
		return 1;
	}

	return len;
}

int
move_to_stream(Stream *s, const Move *move)
{
	int type;
	int len;

	if (onboard(move)) {
		len = stream_fmtputs(s, "%d%c%d%c%s",
			move->src.col, move->src.row + 'a' - 1,
			move->dst.col, move->dst.row + 'a' - 1,
			dopromote(move) ? "+" : "");
	} else {
		type = type_get(move->piece);
		len = piece_to_stream(s, type);
		len += stream_fmtputs(s, "*%d%c",
		       move->dst.col, move->dst.row + 'a' - 1);
	}

	return len;
}

void
stock_show(Stream *s, const Board *board)
{
	int piece;
	int side;
	int n;

	for (side = 0; side < NSIDE; side++) {
		stream_puts(s, side_name(side));
		stream_putc(s, ':');
		for (piece = 0; piece < NPIECE; piece++) {
			n = board->num[side][piece];
			if (n > 0) {
				stream_putc(s, ' ');
				piece_to_stream(s, piece);
				stream_putc(s, 'x');
				stream_fmtputs(s, "%d", n);
			}
		}
		stream_putc(s, '\n');
	}
}

void
board_show(Stream *s, const Board *board)
{
	int row, col;
	int len;

	for (col = 0; col < board->ncol; col++) {
		stream_fmtputs(s, "%d", board->ncol - col);
		if (col == board->ncol - 1)
			stream_putc(s, '\n');
		else
			stream_putc(s, ' ');
	}

	for (row = 0; row < board->nrow; row++) {
		for (col = 0; col < board->ncol; col++) {
			len = piece_to_stream(s, board->cell[row][col]);
			stream_fmtputs(s, "%*s", 2 - len, "");
		}
		stream_putc(s, ' ');
		stream_putc(s, row + 'a');
		stream_putc(s, '\n');
	}

	stock_show(s, board);
}

void
board_show_reverse(Stream *s, const Board *board)
{
	int row, col;
	int len;

	for (col = board->ncol - 1; col >= 0; col--) {
		stream_fmtputs(s, "%d", board->ncol - col);
		if (col == 0)
			stream_putc(s, '\n');
		else
			stream_putc(s, ' ');
	}

	for (row = board->nrow - 1; row >= 0; row--) {
		for (col = board->ncol - 1; col >= 0; col--) {
			len = piece_to_stream(s, board->cell[row][col]);
			stream_fmtputs(s, "%*s", 2 - len, "");
		}
		stream_putc(s, ' ');
		stream_putc(s, row + 'a');
		stream_putc(s, '\n');
	}

	stock_show(s, board);
}

void
board_show_sfen(Stream *s, const Board *board, int side, int turn)
{
	int row, col;
	int piece;
	int type;
	int stock;
	int n;

	for (row = 0; row < board->nrow; row++) {
		n = 0;
		for (col = 0; col < board->ncol; col++) {
			piece = board->cell[row][col];
			type = type_get(piece);
			type = demote(type);
			if (type == NONE) {
				n++;
			} else {
				if (n > 0)
					stream_fmtputs(s, "%d", n);
				n = 0;
				piece_to_stream(s, piece);
			}
		}
		if (n > 0)
			stream_fmtputs(s, "%d", n);
		if (row < board->nrow - 1)
			stream_putc(s, '/');
	}

	stream_putc(s, ' ');

	switch (side) {
	case BLACK:
		stream_putc(s, 'b');
		break;
	case WHITE:
		stream_putc(s, 'w');
		break;
	default:
		stream_putc(s, '-');
		break;
	}

	stream_putc(s, ' ');

	stock = 0;
	for (side = 0; side < NSIDE; side++) {
		for (piece = 0; piece < NPIECE; piece++) {
			n = board->num[side][piece];
			if (n > 0) {
				if (n > 1)
					stream_fmtputs(s, "%d", n);
				piece_to_stream(s, side_set(piece, side));
				stock += n;
			}
		}
	}

	if (stock == 0)
		stream_putc(s, '-');

	stream_fmtputs(s, " %d", turn + 1);

	stream_putc(s, '\n');
}
