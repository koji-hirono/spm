#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
static int cmd_show_sfen(Interp *, const char *);
static int cmd_flip_board(Interp *, const char *);
static int cmd_move(Interp *, const char *);
static int cmd_bestmove(Interp *, const char *);
static int cmd_checkmate(Interp *, const char *);
static int cmd_read(Interp *, const char *);
static int cmd_write(Interp *, const char *);
static int cmd_show_engine(Interp *, const char *);
static int cmd_start_engine(Interp *, const char *);
static int cmd_set_engine_opt(Interp *, const char *);
static int cmd_analyze(Interp *, const char *);

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
	{'o', "set engine option", cmd_set_engine_opt},
	{'s', "show board", cmd_show},
	{'f', "show SFEN", cmd_show_sfen},
	{'l', "show movelog", cmd_show_movelog},
	{'E', "show engine", cmd_show_engine},
	{'S', "start engine", cmd_start_engine},
	{'F', "flip baord", cmd_flip_board},
	{'a', "analyze", cmd_analyze},
	{'h', "help", cmd_help},
	{'?', "help", cmd_help},
	{'q', "quit", cmd_quit},
	{0, NULL, NULL}
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

	/* 本来はbranching */
	game->movelog.n = game->turn;
	movelog_add(&game->movelog, &move);
	game->turn++;

	return 0;
}

static int
cmd_bestmove(Interp *interp, const char *line)
{
	Stream *out = interp->out;
	Game *game = interp->game;
	Movecap cap;
	Move move;
	int ret;

	ret = engine_move(interp->engine, &move, &cap, interp->game);
	switch (ret) {
	case MOVE:
		move_to_stream(out, &move);
		stream_putc(out, '\n');
		if (cur_side(game) == WHITE)
			cap.score *= -1;
		stream_fmtputs(out, "score: %d\n", cap.score);
		stream_fmtputs(out, "depth: %d\n", cap.depth);
		stream_fmtputs(out, "seldepth: %d\n", cap.seldepth);
		break;
	case RESIGN:
		stream_puts(out, "resign\n");
		break;
	case WIN:
		stream_puts(out, "win\n");
		break;
	default:
		stream_puts(out, "fail\n");
		break;
	}

	return 0;
}

static int
cmd_checkmate(Interp *interp, const char *line)
{
	Stream *out = interp->out;
	Movelog mate;
	int ret;
	int i;

	ret = engine_mate(interp->engine, &mate, interp->game);
	switch (ret) {
	case MOVE:
		for (i = 0; i < mate.n; i++) {
			move_to_stream(out, mate.list + i);
			if (i != mate.n - 1)
				stream_putc(out, ' ');
		}
		movelog_destroy(&mate);
		break;
	case NOMATE:
		stream_puts(out, "nomate");
		break;
	default:
		stream_puts(out, "fail");
		break;
	}
	stream_putc(out, '\n');

	return 0;
}

static int
cmd_prev(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Move *move;

	if (game->turn <= 0)
		return 0;

	game->turn--;
	move = game->movelog.list + game->turn;
	move_prev(move, &game->board);

	return 0;
}

static int
cmd_next(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Move *move;

	if (game->turn >= game->movelog.n)
		return 0;

	move = game->movelog.list + game->turn;

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
	int i;

	no = strtoul(line, NULL, 0);
	d = no - interp->game->turn;
	if (d == 0)
		return 0;
	if (d > 0) {
		for (i = 0; i < d; i++)
			cmd_next(interp, line);
	} else {
		for (i = d; i < 0; i++)
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
cmd_flip_board(Interp *interp, const char *line)
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
	Movecap *cap;
	Move *move;
	int side;

	stream_fmtputs(out, "#%d/%d ",
			game->turn, game->movelog.n);

	if (game->turn > 0) {
		move = game->movelog.list + game->turn - 1;
		cap = game->movelog.cap + game->turn - 1;
		side = side_get(move->piece);
		stream_puts(out, side_name(side));
		stream_puts(out, ": ");
		move_to_stream(out, move);
		if (cap->flags & MOVECAP_VALID)
			stream_fmtputs(out, " (%d)", cap->score);
		else
			stream_puts(out, " (-)");
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
cmd_show_sfen(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Stream *out = interp->out;

	board_show_sfen(out, &game->board, cur_side(game), game->turn);

	return 0;
}

static int
cmd_show_movelog(Interp *interp, const char *line)
{
	Stream *out = interp->out;
	Game *game = interp->game;
	const char *name;
	Movecap *prev;
	Movecap *cap;
	Move *m;
	int len;
	int d;
	int i;

	prev = NULL;
	for (i = 0; i < game->turn; i++) {
		m = game->movelog.list + i;
		cap = game->movelog.cap + i;
		stream_fmtputs(out, "%3d. ", i + 1);
		name = side_name((game->board.side + i) & 1);
		stream_fmtputs(out, "%c ", toupper(name[0] & 0xff));
		len = move_to_stream(out, m);
		if (cap->flags & MOVECAP_VALID) {
			if (len < 5)
				stream_putc(out, ' ');
			stream_fmtputs(out, " (%6d) ", cap->score);
			len = move_to_stream(out, &cap->expect);
			if (len < 5)
				stream_putc(out, ' ');
			if (move_equal(m, &cap->expect)) {
				stream_puts(out, " o");
			} else if (prev != NULL) {
				d = cap->score - prev->score;
				if ((game->board.side + i) & 1)
					d *= -1;
				if (d <= -400)
					stream_puts(out, " x");
				else if (d < -200)
					stream_puts(out, " ?");
			}
		}
		stream_putc(out, '\n');
		prev = cap;
	}

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
		stream_fmtputs(out, "    type: %s\n",
				usiopttypestr(opt->type));
		switch (opt->type) {
		case USIOPT_CHECK:
			stream_fmtputs(out, "    cur : %s\n",
					opt->cur ? "true" : "false");
			stream_fmtputs(out, "    def : %s\n",
					opt->def ? "true" : "false");
			break;
		case USIOPT_SPIN:
			stream_fmtputs(out, "    cur : %d\n",
					(int)(intptr_t)opt->cur);
			stream_fmtputs(out, "    def : %d\n",
					(int)(intptr_t)opt->def);
			stream_fmtputs(out, "    min : %d\n", opt->min);
			stream_fmtputs(out, "    max : %d\n", opt->max);
			break;
		case USIOPT_COMBO:
			stream_fmtputs(out, "    cur : `%s'\n",
					(const char *)opt->cur);
			stream_fmtputs(out, "    def : `%s'\n",
				       	(const char *)opt->def);
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
			(int)len, line);
		return 0;
	}

	p = line + len;
	p += strspn(p, " ");

	engine_setoption(engine, opt->name, p);

	return 0;
}

static int
cmd_analyze(Interp *interp, const char *line)
{
	Game *game = interp->game;
	Movelog *log = &game->movelog;
	Movecap *prev;
	Movecap *cap;
	Move *move;
	int n;
	int i;
	int org;
	int ret;

	n = strtoul(line, NULL, 0);
	if (n > log->n)
		n = log->n;

	org = game->turn;
	for (i = 0; i < org; i++) {
		game->turn--;
		move_prev(log->list + game->turn, &game->board);
	}

	prev = NULL;
	for (i = 0; i < n; i++) {
		cap = log->cap + i;
		move = log->list + i;
		ret = engine_move(interp->engine, &cap->expect, cap, game);
		if (ret != MOVE)
			continue;
		if (cur_side(game) == WHITE)
			cap->score *= -1;
		if (move_normalize(move, &game->board, cur_side(game)) != 0) {
			error("Illegal move.");
			continue;
		}
		if (move_normalize(&cap->expect, &game->board,
				   cur_side(game)) != 0) {
			warn("amb move.");
			continue;
		}

		/*
		 * 指し手が一致していれば、そのままのscore 
		 * 指し手が一致していなければ、その次のscore
		 */
		if (prev)
			prev->score = cap->score;
		if (move_equal(move, &cap->expect)) {
			prev = NULL;
		} else {
			prev = cap;
		}
		cap->flags |= MOVECAP_VALID;

		move_next(log->list + i, &game->board);
		game->turn++;
	}

	return 0;
}
