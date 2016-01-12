#ifndef GAME_H_
#define GAME_H_

#include "board.h"
#include "movelog.h"

typedef struct Game Game;

struct Game {
	Board board;
	Movelog movelog;
	char *start;
	int turn;
	int flags;
};

/* Game::flags */
#define REVERSE_BOARD (1 << 0)

#define cur_side(g) (((g)->turn + (g)->board.side) & 1)

extern int game_init(Game *, const char *);

#endif /* !GAME_H_ */
