#include <stdlib.h>

#include "trace.h"
#include "board.h"
#include "isreach.h"


static int
isreach_none(int dx, int dy)
{
	return 0;
}

static int
isreach_pawn(int dx, int dy)
{
	return dx == 0 && dy == 1;
}

static int
isreach_lance(int dx, int dy)
{
	return dx == 0 && dy > 0;
}

static int
isreach_knight(int dx, int dy)
{
	return abs(dx) == 1 && dy == 2;
}

static int
isreach_rook(int dx, int dy)
{
	return dx == 0 || dy == 0;
}

static int
isreach_bishop(int dx, int dy)
{
	return abs(dx) == abs(dy);
}

static int
isreach_silver(int dx, int dy)
{
	if (abs(dx) == abs(dy) && abs(dx) == 1)
		return 1;
	else
		return isreach_pawn(dx, dy);
}

static int
isreach_king(int dx, int dy)
{
	return abs(dx) <= 1 && abs(dy) <= 1;
}

static int
isreach_gold(int dx, int dy)
{
	if (dx == 0 && abs(dy) == 1)
		return 1;

	if (abs(dx) == 1 && (dy == 0 || dy == 1))
		return 1;

	return 0;
}

static int
isreach_horse(int dx, int dy)
{
	return isreach_bishop(dx, dy) || isreach_king(dx, dy);
}

static int
isreach_dragon(int dx, int dy)
{
	return isreach_rook(dx, dy) || isreach_king(dx, dy);
}

int
isreach(const Move *move)
{
	int (*list[])(int, int) = {
		isreach_none,
		isreach_pawn,
		isreach_lance,
		isreach_knight,
		isreach_bishop,
		isreach_rook,
		isreach_silver,
		isreach_gold,
		isreach_king
	};
	int (*plist[])(int, int) = {
		isreach_none,
		isreach_gold,
		isreach_gold,
		isreach_gold,
		isreach_horse,
		isreach_dragon,
		isreach_gold,
		isreach_none,
		isreach_none,
	};
	int dx, dy;
	int type;

	dy = move->dst.row - move->src.row;
	dx = move->dst.col - move->src.col;
	if (side_get(move->piece) == BLACK) {
		dx = -dx;
		dy = -dy;
	}

	type = type_get(move->piece);
	type = demote(type);

	if (ispromoted(move->piece))
		return (*plist[type])(dx, dy);
	else
		return (*list[type])(dx, dy);
}
