#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"
#include "buf.h"
#include "stream.h"
#include "board.h"
#include "boardfmt.h"
#include "move.h"
#include "game.h"
#include "engine.h"
#include "cmds.h"


static int cmd_help(Interp *, const char *);
static int cmd_quit(Interp *, const char *);
static int cmd_next(Interp *, const char *);
static int cmd_prev(Interp *, const char *);
static int cmd_jump(Interp *, const char *);
static int cmd_show_movelog(Interp *, const char *);
static int cmd_show(Interp *, const char *);
static int cmd_toggle_board(Interp *, const char *);
static int cmd_move(Interp *, const char *);
static int cmd_bestmove(Interp *, const char *);
static int cmd_checkmate(Interp *, const char *);
static int cmd_read(Interp *, const char *);
static int cmd_write(Interp *, const char *);
static int cmd_show_engine(Interp *, const char *);
static int cmd_start_engine(Interp *, const char *);
static int cmd_set_engine_opt(Interp *, const char *);

static Cmd cmdtbl[] = {
	{'>', "move next", cmd_next},
	{'n', "move next", cmd_next},
	{'<', "move previous", cmd_prev},
	{'p', "move previous", cmd_prev},
	{'j', "jump to <x>th position", cmd_jump},
	{'m', "move", cmd_move},
	{'b', "bestmove", cmd_bestmove},
	{'c', "checkmate", cmd_checkmate},
	{'r', "read movelog", cmd_read},
	{'w', "write movelog", cmd_write},
	{'S', "start engine", cmd_start_engine},
	{'o', "set engine option", cmd_set_engine_opt},
	{'s', "show board", cmd_show},
	{'l', "show movelog", cmd_show_movelog},
	{'E', "show engine", cmd_show_engine},
	{'t', "toggle baord", cmd_toggle_board},
	{'h', "help", cmd_help},
	{'?', "help", cmd_help},
	{'q', "quit", cmd_quit},
	{0}
};

Cmd *
cmd_find(int ch)
{
	Cmd *cmd;

	for (cmd = cmdtbl; cmd->ch != '\0'; cmd++)
		if (cmd->ch == ch)
			return cmd;

	return NULL;
}

static int
cmd_help(Interp *interp, const char *line)
{
	Cmd *cmd;

	stream_puts(interp->out, "Commands:\n");

	for (cmd = cmdtbl; cmd->ch != '\0'; cmd++)
		stream_fmtputs(interp->out, "  %c: %s\n", cmd->ch, cmd->desc);

	return 0;
}

static int
cmd_quit(Interp *interp, const char *line)
{
	return 1;
}

static int
cmd_move(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Move move;

	Stream s;

	stream_strinit(&s, line);

	if (move_parse(&move, &s) != 0) {
		stream_fmtputs(interp->out, "illegal move: `%s'.\n", line);
		return 0;
	}
	if (move_normalize(&move, &game->board, cur_side(game)) != 0) {
		error("amb move.");
		return 0;
	}
	if (move_check(&move, &game->board) != 0) {
		error("illegal move.");
		return 0;
	}
	move_next(&move, &game->board);
	game->turn++;
	game->movelog.n = game->turn;
	movelog_add(&game->movelog, &move);

	return 0;
}

static int
cmd_bestmove(Interp *interp, const char *line)
{
	Move move;
	int ret;

	ret = engine_move(interp->engine, &move, interp->game);
	switch (ret) {
	case MOVE:
		move_show(interp->out, &move);
		break;
	case RESIGN:
		stream_puts(interp->out, "resign");
		break;
	case WIN:
		stream_puts(interp->out, "win");
		break;
	default:
		stream_puts(interp->out, "fail");
		break;
	}
	stream_putc(interp->out, '\n');

	return 0;
}

static int
cmd_checkmate(Interp *interp, const char *line)
{
	Movelog mate;
	int ret;
	int i;

	ret = engine_mate(interp->engine, &mate, interp->game);
	switch (ret) {
	case MOVE:
		for (i = 0; i < mate.n; i++) {
			move_show(interp->out, mate.list + i);
			if (i != mate.n - 1)
				stream_putc(interp->out, ' ');
		}
		movelog_destroy(&mate);
		break;
	case NOMATE:
		stream_puts(interp->out, "nomate");
		break;
	default:
		stream_puts(interp->out, "fail");
		break;
	}
	stream_putc(interp->out, '\n');

	return 0;
}

static int
cmd_prev(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Move *move;

	if (game->turn < 0)
		return 0;

	move = game->movelog.list + game->turn;
	move_prev(move, &game->board);
	game->turn--;

	return 0;
}

static int
cmd_next(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Move *move;

	if (game->turn + 1 >= game->movelog.n)
		return 0;

	move = game->movelog.list + game->turn + 1;

	if (move_normalize(move, &game->board, cur_side(game)) != 0) {
		error("Illegal move.");
		return 0;
	}
	move_next(move, &game->board);
	game->turn++;

	return 0;
}

static int
cmd_jump(Interp *interp, const char *line)
{
	int no;
	int d;

	no = strtoul(line, NULL, 0);
	d = no - interp->game->turn;
	if (d == 0)
		return 0;
	if (d > 0) {
		while (--d > 0)
			cmd_next(interp, line);
	} else {
		while (d++ <= 0)
			cmd_prev(interp, line);
	}

	return 0;
}

static int
cmd_read(Interp *interp, const char *line)
{
	return 0;
}

static int
cmd_write(Interp *interp, const char *line)
{
	movelog_save(&interp->game->movelog, line);

	return 0;
}

static int
cmd_toggle_board(Interp *interp, const char *line)
{
	Game *game = interp->game;

	if (game->flags & REVERSE_BOARD)
		game->flags &= ~REVERSE_BOARD;
	else
		game->flags |= REVERSE_BOARD;

	return 0;
}

static int
cmd_show(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Stream *out = interp->out;
	Move *move;
	int side;

	stream_fmtputs(out, "#%d/%d ",
			game->turn + 1, game->movelog.n);

	if (game->turn != -1) {
		move = game->movelog.list + game->turn;
		side = side_get(move->piece);
		stream_puts(out, side_name(side));
		stream_puts(out, ": ");
		move_show(out, move);
	}
	stream_putc(out, '\n');
	if (game->flags & REVERSE_BOARD)
		board_show_reverse(out, &game->board);
	else
		board_show(out, &game->board);
	stream_putc(out, '\n');

	return 0;
}

static int
cmd_show_movelog(Interp *interp, const char *line)
{
	Stream *out = interp->out;
	Game *game = interp->game;
	Move *m;
	int i;

	for (i = 0; i < game->turn + 1; i++) {
		m = game->movelog.list + i;
		stream_putc(out, ' ');
		move_show(out, m);
	}
	stream_putc(out, '\n');

	return 0;
}

static int
cmd_show_engine(Interp *interp, const char *line)
{
	Stream *out = interp->out;
	Engine *engine = interp->engine;
	Engineopt *opt;
	Optvar *var;

	stream_puts(out, "Engine status: ");
	stream_puts(out, engine_status_str(engine->status));
	stream_putc(out, '\n');

	stream_puts(out, "Engine name  : ");
	if (engine->name == NULL)
		stream_puts(out, "-\n");
	else {
		stream_puts(out, engine->name);
		stream_putc(out, '\n');
	}

	stream_puts(out, "Engine author: ");
	if (engine->author == NULL)
		stream_puts(out, "-\n");
	else {
		stream_puts(out, engine->author);
		stream_putc(out, '\n');
	}

	if (engine->opt == NULL)
		return 0;

	stream_puts(out, "Engine option:\n");

	for (opt = engine->opt; opt != NULL; opt = opt->next) {
		stream_fmtputs(out, "  %s:\n", opt->name);
		stream_fmtputs(out, "    type: %s\n", usiopttypestr(opt->type));
		switch (opt->type) {
		case USIOPT_CHECK:
			stream_fmtputs(out, "    cur : %s\n",
					(int)opt->cur ? "true" : "false");
			stream_fmtputs(out, "    def : %s\n",
					(int)opt->def ? "true" : "false");
			break;
		case USIOPT_SPIN:
			stream_fmtputs(out, "    cur : %d\n", opt->cur);
			stream_fmtputs(out, "    def : %d\n", opt->def);
			stream_fmtputs(out, "    min : %d\n", opt->min);
			stream_fmtputs(out, "    max : %d\n", opt->max);
			break;
		case USIOPT_COMBO:
			stream_fmtputs(out, "    cur : `%s'\n", opt->cur);
			stream_fmtputs(out, "    def : `%s'\n", opt->def);
			stream_puts(out, "    var :\n");
			for (var = opt->var; var != NULL; var = var->next) {
				stream_puts(out, "      `");
				stream_puts(out, var->s);
				stream_puts(out, "'\n");
			}
			stream_putc(out, '\n');
			break;
		case USIOPT_BUTTON:
			break;
		case USIOPT_STRING:
		case USIOPT_FILENAME:
			stream_fmtputs(out, "    cur : `%s'\n", opt->cur);
			stream_fmtputs(out, "    def : `%s'\n", opt->def);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int
cmd_start_engine(Interp *interp, const char *line)
{
	Engine *engine = interp->engine;

	engine_start(engine);

	return 0;
}

static int
cmd_set_engine_opt(Interp *interp, const char *line)
{
	Engine *engine = interp->engine;
	Engineopt *opt;
	const char *p;
	size_t len;

	len = strcspn(line, " ");

	for (opt = engine->opt; opt != NULL; opt = opt->next) {
		if (strncmp(opt->name, line, len) != 0)
			continue;
		if (opt->name[len] != '\0')
			continue;
		break;
	}

	if (opt == NULL) {
		stream_fmtputs(interp->out, "not found option: %.*s\n", 
			len, line);
		return 0;
	}

	p = line + len;
	p += strspn(p, " ");

	engine_setoption(engine, opt->name, p);

	return 0;
}
