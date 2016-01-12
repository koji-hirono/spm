#include <stddef.h>
#include <string.h>

#include "game.h"

int
game_init(Game *game, const char *s)
{
	if (s == NULL) {
		if (board_init(&game->board) != 0)
			return -1;
		game->start = NULL;
	} else {
		if (board_sfen_init(&game->board, s) != 0)
			return -1;
		if ((game->start = strdup(s)) == NULL)
			return -1;
	}

	movelog_init(&game->movelog);
	game->turn = 0;
	game->flags = 0;

	return 0;
}
