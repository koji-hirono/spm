#include <stddef.h>

#include "trace.h"
#include "board.h"
#include "move.h"
#include "isreach.h"


int
move_equal(const Move *m1, const Move *m2)
{
	if (m1->flags != m2->flags)
		return 0;

	if (m1->dst.row != m2->dst.row)
		return 0;
	if (m1->dst.col != m2->dst.col)
		return 0;

	if (onboard(m1)) {
		if (m1->src.row != m2->src.row)
			return 0;
		if (m1->src.col != m2->src.col)
			return 0;
	} else {
		if (m1->piece != m2->flags)
			return 0;
	}


	return 1;
}

int
move_normalize(Move *move, Board *board, int side)
{
	int *p;

	if (type_get(move->piece) == NONE) {
		p = board_cell(board, move->src.row, move->src.col);
		move->piece = type_set(move->piece, type_get(*p));
	}
	move->piece = side_set(move->piece, side);

	if (type_get(move->get_piece) == NONE) {
		p = board_cell(board, move->dst.row, move->dst.col);
		move->get_piece = *p;
	}

	return 0;
}

int
isopenway(const Board *board, const Pos *src, const Pos *dst)
{
	Pos p;
	int dx, dy;
	int *piece;

	dy = dst->row - src->row;
	dx = dst->col - src->col;

	if (dy > 1)
		dy = 1;
	if (dy < -1)
		dy = -1;
	if (dx > 1)
		dx = 1;
	if (dx < -1)
		dx = -1;

	p.col = src->col + dx;
	p.row = src->row + dy;
	while (p.col != dst->col || p.row != dst->row) {
		piece = board_cell(board, p.row, p.col);
		if (*piece != NONE)
			return 0;

		p.col += dx;
		p.row += dy;
	}

	return 1;
}

int
isillegal_promote(Move *move, const Board *board)
{
	if (!dopromote(move))
		return 0;

	if (side_get(move->piece) == BLACK) {
		if (move->dst.row <= 3)
			return 0;
		if (move->src.row <= 3)
			return 0;
	} else {
		if (move->dst.row >= 7)
			return 0;
		if (move->src.row >= 7)
			return 0;
	}

	return 1;
}

int
isillegal_nextmove(Move *move, const Board *board)
{
	if (dopromote(move))
		return 0;

	switch (type_get(move->piece)) {
	case PAWN:
	case LANCE:
		if (side_get(move->piece) == BLACK) {
			if (move->dst.row > 1)
				return 0;
		} else {
			if (move->dst.row < 9)
				return 0;
		}
		break;
	case KNIGHT:
		if (side_get(move->piece) == BLACK) {
			if (move->dst.row > 2)
				return 0;
		} else {
			if (move->dst.row < 8)
				return 0;
		}
		break;
	default:
		return 0;
	}

	return 1;
}

int
isdouble_pawn(Move *move, const Board *board)
{
	int *piece;
	int row;

	if (onboard(move))
		return 0;

	if (dopromote(move))
		return 0;

	if (type_get(move->piece) != PAWN)
		return 0;

	for (row = 1; row <= board->nrow; row++) {
		piece = board_cell(board, row, move->dst.col);
		if (type_get(*piece) == PAWN &&
		    side_get(*piece) == side_get(move->piece))
			return 1;
	}

	return 0;
}

int
iskingdead(int side, const Board *board)
{
	int *piece;
	Move move;
	int row;
	int col;

	for (row = 1; row <= board->nrow; row++) {
		for (col = 1; col <= board->ncol; col++) {
			piece = board_cell(board, row, col);
			if (side_get(*piece) == side &&
			    type_get(*piece) == KING) {
				move.dst.col = col;
				move.dst.row = row;
			}
		}
	}

	for (row = 1; row <= board->nrow; row++) {
		for (col = 1; col <= board->ncol; col++) {
			piece = board_cell(board, row, col);
			if (*piece == NONE)
				continue;
			if (side_get(*piece) == side)
				continue;
			move.piece = *piece;
			move.src.row = row;
			move.src.col = col;
			if (!isreach(&move)) {
				continue;
			}
			if (type_get(move.piece) != KNIGHT) {
				if (!isopenway(board, &move.src, &move.dst)) {
					continue;
				}
			}
			error("(%d,%c)->(%d,%c): %#x",
			      move.src.col, move.src.row + 'a' - 1, 
			      move.dst.col, move.dst.row + 'a' - 1, 
			      move.piece);
			return 1;
		}
	}

	return 0;
}

int
isfoul(Move *move, const Board *board)
{
	Board next;

	if (isillegal_promote(move, board)) {
		error("illegal promote");
		return 1;
	}

	if (isillegal_nextmove(move, board)) {
		error("never move");
		return 1;
	}

	if (isdouble_pawn(move, board)) {
		error("double pawn");
		return 1;
	}

	board_clone(&next, board);
	move_next(move, &next);
	if (iskingdead(side_get(move->piece), &next)) {
		error("king is dead");
		board_destroy(&next);
		return 1;
	}
	board_destroy(&next);

	return 0;
}

int
move_check(Move *move, const Board *board)
{
	int *src;
	int *dst;

	/* 移動元のチェック */
	if (onboard(move)) {
		src = board_cell(board, move->src.row, move->src.col);

		/* pieceが一致しない */
		if (move->piece != *src) {
			error("diff piece type: %#x != %#x.",
			      move->piece, *src);
			return -1;
		}
	} else {
		int side;
		int type;

		side = side_get(move->piece);
		type = type_get(move->piece);
		type = demote(type);

		/* ストックに存在しない */
		if (board->num[side][type] == 0) {
			error("stock %d %d empty.", side, type);
			return -1;
		}
	}

	/* 移動先のチェック */
	dst = board_cell(board, move->dst.row, move->dst.col);

	/* 移動元と同じサイド */
	if (type_get(*dst) != NONE &&
	    side_get(*dst) == side_get(move->piece)) {
		error("target is same side. %#x.", *dst);
		return -1;
	}

	if (!onboard(move)) {
		if (type_get(*dst) != NONE) {
			error("target is not NONE.");
			return -1;
		}
	}

	if (onboard(move)) {
		/* 到達可能チェック */
		if (!isreach(move)) {
			error("not reached.");
			return -1;
		}

		if (type_get(move->piece) != KNIGHT) {
			if (!isopenway(board, &move->src, &move->dst)) {
				error("not open way.");
				return -1;
			}
		}
	}

	/* 禁じ手 */
	if (isfoul(move, board)) {
		error("foul.");
		return -1;
	}

	return 0;
}

void
move_next(const Move *move, Board *board)
{
	int *src;
	int *dst;
	int side;
	int type;

	if (onboard(move)) {
		src = board_cell(board, move->src.row, move->src.col);
		*src = NONE;
	} else {
		side = side_get(move->piece);
		type = type_get(move->piece);
		type = demote(type);
		board->num[side][type]--;
	}

	dst = board_cell(board, move->dst.row, move->dst.col);

	side = side_get(*dst);
	type = type_get(*dst);
	type = demote(type);
	if (type != NONE)
		board->num[side_opponent(side)][type]++;

	*dst = move->piece;
	if (dopromote(move))
		*dst = promote(*dst);
}

void
move_prev(const Move *move, Board *board)
{
	int *src;
	int *dst;
	int side;
	int type;

	side = side_get(move->get_piece);
	type = type_get(move->get_piece);
	type = demote(type);
	if (type != NONE)
		board->num[side_opponent(side)][type]--;

	dst = board_cell(board, move->dst.row, move->dst.col);
	*dst = move->get_piece;

	if (onboard(move)) {
		src = board_cell(board, move->src.row, move->src.col);
		*src = move->piece;
	} else {
		side = side_get(move->piece);
		type = type_get(move->piece);
		type = demote(type);
		board->num[side][type]++;
	}
}
