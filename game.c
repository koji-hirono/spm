#include <stddef.h>

#include "game.h"

int
game_init(Game *game, const char *sfen)
{
	if (board_init(&game->board) != 0)
		return -1;

	movelog_init(&game->movelog);
	game->start = NULL;
	game->turn = -1;
	game->flags = 0;

	return 0;
}
