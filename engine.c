#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>

#include "stringex.h"
#include "trace.h"
#include "buf.h"
#include "stream.h"
#include "cfg.h"
#include "board.h"
#include "boardfmt.h"
#include "move.h"
#include "movelog.h"
#include "usi.h"
#include "engine.h"


static int id_parse(Usimsg *, Engine *);
static int option_parse(Usimsg *, Engine *);
static int info_parse(Usimsg *, Movecap *);
static int bestmove_parse(Usimsg *, Move *);
static int checkmate_parse(Usimsg *, Movelog *);
static int infomate_parse(Usimsg *, Movelog *);
static int position_to_stream(Stream *, const Game *);
static int tx(Engine *, const char *);
static int rx(Engine *, Buf *);
static void infoout(Engine *, const char *, const char *);


int
engine_open(Engine *engine, const char *path, Cfg *cfg, int level)
{
	pid_t pid;
	int u2e[2];
	int e2u[2];

	if (pipe(u2e) != 0) {
		error("pipe");
		return -1;
	}

	if (pipe(e2u) != 0) {
		error("pipe");
		close(u2e[0]);
		close(u2e[1]);
		return -1;
	}

	pid = fork();
	if (pid == -1) {
		error("fork");
		close(u2e[0]);
		close(u2e[1]);
		close(e2u[0]);
		close(e2u[1]);
		return -1;
	}
	if (pid == 0) {
		close(u2e[1]);
		dup2(u2e[0], 0);
		close(e2u[0]);
		dup2(e2u[1], 1);
		if (execlp(path, path, (char *)0) == -1)
			fatal("execlp");
		_exit(EXIT_FAILURE);
	}

	close(u2e[0]);
	close(e2u[1]);
	stream_ioinit(&engine->w, u2e[1]);
	stream_ioinit(&engine->r, e2u[0]);
	engine->pid = pid;
	engine->infolevel = level;
	engine->name = NULL;
	engine->author = NULL;
	engine->opt = NULL;
	engine->cfg = cfg;
	engine->status = ENGINE_SPAWNED;

	return 0;
}

void
engine_close(Engine *engine)
{
	Engineopt *opt;
	Engineopt *next;

	tx(engine, "quit");

	close(engine->w.fd);
	close(engine->r.fd);

	free(engine->name);
	free(engine->author);

	for (opt = engine->opt; opt != NULL; opt = next) {
		next = opt->next;
		engineopt_destroy(opt);
	}
}

int
engine_init(Engine *engine)
{
	Usimsg msg;
	Buf buf;
	int end;

	buf_init(&buf);

	if (tx(engine, "usi") != 0)
		goto err;

	end = 0;
	while (!end) {
		if (rx(engine, &buf) != 0)
			goto err;
		if (buf.b[0] == '\0')
			continue;
		if (usimsg_parse(&msg, buf.b) != 0)
			goto err;
		switch (msg.type) {
		case USIMSG_OK:
			end = 1;
			break;
		case USIMSG_ID:
			id_parse(&msg, engine);
			break;
		case USIMSG_OPTION:
			option_parse(&msg, engine);
			break;
		default:
			break;
		}
		usimsg_destroy(&msg);
	}

	engine->status = ENGINE_INITED;

	buf_free(&buf);
	return 0;
err:
	buf_free(&buf);
	return -1;
}

int
engine_start(Engine *engine)
{
	Usimsg msg;
	Buf buf;
	int end;

	buf_init(&buf);

	if (tx(engine, "isready") != 0)
		goto err;

	end = 0;
	while (!end) {
		if (rx(engine, &buf) != 0)
			goto err;
		if (buf.b[0] == '\0')
			continue;
		if (usimsg_parse(&msg, buf.b) != 0)
			goto err;
		switch (msg.type) {
		case USIMSG_READYOK:
			end = 1;
			break;
		case USIMSG_INFO:
			break;
		default:
			error("isready? -> `%s'.", buf.b);
			usimsg_destroy(&msg);
			goto err;
		}
		usimsg_destroy(&msg);
	}

	if (tx(engine, "usinewgame") != 0)
		goto err;

	engine->status = ENGINE_STARTED;

	buf_free(&buf);
	return 0;
err:
	buf_free(&buf);
	return -1;
}

int
engine_setoption(Engine *engine, const char *name, const char *val)
{
	stream_fmtputs(&engine->w, "setoption name %s", name);
	if (val != NULL) {
		stream_puts(&engine->w, " value ");
		stream_puts(&engine->w, val);
	}
	stream_putc(&engine->w, '\n');

	return 0;
}

int
engine_autoconfig(Engine *engine)
{
	Engineopt *opt;
	Cfg *sect;
	Cfg *entry;

	for (sect = engine->cfg->child; sect != NULL; sect = sect->next)
		if (strstr(engine->name, sect->name) != NULL)
			break;
	if (sect == NULL)
		return 0;

	for (entry = sect->child; entry != NULL; entry = entry->next) {
		for (opt = engine->opt; opt != NULL; opt = opt->next)
			if (strcmp(opt->name, entry->name) == 0)
				break;
		if (opt == NULL)
			continue;
		switch (opt->type) {
		case USIOPT_CHECK:
			if (strcmp(entry->value, "true") == 0)
				opt->cur = (void *)(intptr_t)1;
			else if (strcmp(entry->value, "false") == 0)
				opt->cur = (void *)(intptr_t)0;
			break;
		case USIOPT_SPIN:
			opt->cur = (void *)(intptr_t)strtol(entry->value,
				       	NULL, 0);
			break;
		case USIOPT_BUTTON:
			break;
		case USIOPT_COMBO:
		case USIOPT_STRING:
		case USIOPT_FILENAME:
			free(opt->cur);
			opt->cur = strdup(entry->value);
			break;
		default:
			break;
		}
		engine_setoption(engine, entry->name, entry->value);
	}

	return 0;
}


int
engine_move(Engine *engine, Move *move, Movecap *cap, const Game *game)
{
	const char *btime = "0";
	const char *wtime = "0";
	const char *byoyomi = "3000";
	Usimsg msg;
	Cfg *sect;
	Cfg *entry;
	Buf buf;
	int end;
	int r;

	if (position_to_stream(&engine->w, game) != 0)
		return -1;


	if ((sect = cfg_get(engine->cfg, "engine")) != NULL) {
		if ((entry = cfg_get(sect, "btime")) != NULL)
			btime = entry->value;
		if ((entry = cfg_get(sect, "wtime")) != NULL)
			wtime = entry->value;
		if ((entry = cfg_get(sect, "byoyomi")) != NULL)
			byoyomi = entry->value;
	}

	buf_init(&buf);

	buf_pushfmt(&buf, "go btime %s wtime %s byoyomi %s",
			btime, wtime, byoyomi);

	if (tx(engine, buf.b) != 0)
		goto err;

	movecap_init(cap);

	end = 0;
	r = -1;
	while (!end) {
		if (rx(engine, &buf) != 0)
			goto err;
		if (buf.b[0] == '\0')
			continue;
		if (usimsg_parse(&msg, buf.b) != 0)
			goto err;
		switch (msg.type) {
		case USIMSG_BESTMOVE:
			r = bestmove_parse(&msg, move);
			end = 1;
			break;
		case USIMSG_INFO:
			info_parse(&msg, cap);
			break;
		default:
			break;
		}
		usimsg_destroy(&msg);
	}

	buf_free(&buf);
	return r;
err:
	buf_free(&buf);
	return -1;
}

int
engine_mate(Engine *engine, Movelog *mate, const Game *game)
{
	const char *matetimeout = "60000";
	Cfg *sect;
	Cfg *entry;
	Usimsg msg;
	Buf buf;
	int end;
	int r;

	if (position_to_stream(&engine->w, game) != 0)
		return -1;

	if ((sect = cfg_get(engine->cfg, "engine")) != NULL)
		if ((entry = cfg_get(sect, "matetimeout")) != NULL)
			matetimeout = entry->value;

	buf_init(&buf);

	buf_pushfmt(&buf, "go mate %s", matetimeout);

	if (tx(engine, buf.b) != 0)
		goto err;

	end = 0;
	r = -1;
	while (!end) {
		if (rx(engine, &buf) != 0)
			goto err;
		if (buf.b[0] == '\0')
			continue;
		if (usimsg_parse(&msg, buf.b) != 0)
			goto err;
		switch (msg.type) {
		case USIMSG_CHECKMATE:
			r = checkmate_parse(&msg, mate);
			end = 1;
			break;
		case USIMSG_BESTMOVE:
			end = 1;
			break;
		case USIMSG_INFO:
			if (r != MOVE)
				r = infomate_parse(&msg, mate);
			break;
		default:
			break;
		}
		usimsg_destroy(&msg);
	}

	buf_free(&buf);
	return r;
err:
	buf_free(&buf);
	return -1;
}

const char *
engine_status_str(int status)
{
	const char *list[] = {
		"spawned", "inited", "started"
	};
	const int n = sizeof(list) / sizeof(list[0]);

	if (status < 0 || status >= n)
		return "-";
	return list[status];
}

Engineopt *
engineopt_create(const char *name, size_t len, int type)
{
	Engineopt *opt;

	if ((opt = malloc(sizeof(Engineopt))) == NULL)
		return NULL;

	if ((opt->name = strexndup(name, len)) == NULL) {
		free(opt);
		return NULL;
	}

	opt->type = type;
	opt->next = NULL;
	opt->cur = NULL;
	opt->def = NULL;
	opt->min = 0;
	opt->max = 0;
	opt->var = NULL;

	return opt;
}

void
engineopt_destroy(Engineopt *opt)
{
	Optvar *var;
	Optvar *next;

	free(opt->name);

	switch (opt->type) {
	case USIOPT_CHECK:
	case USIOPT_SPIN:
		break;
	case USIOPT_BUTTON:
		break;
	case USIOPT_COMBO:
		free(opt->cur);
		free(opt->def);
		for (var = opt->var; var != NULL; var = next) {
			next = var->next;
			free(var->s);
			free(var);
		}
		break;
	case USIOPT_STRING:
	case USIOPT_FILENAME:
		free(opt->cur);
		free(opt->def);
		break;
	default:
		break;
	}

	free(opt);
}

void
engineopt_add(Engine *engine, Engineopt *add)
{
	Engineopt *opt;

	if ((opt = engine->opt) == NULL) {
		engine->opt = add;
		return;
	}

	while (opt->next != NULL)
		opt = opt->next;

	opt->next = add;
}

int
optvar_add(Engineopt *opt, const char *s, size_t len)
{
	Optvar *add;
	Optvar *var;

	if ((add = malloc(sizeof(Optvar))) == NULL)
		return -1;

	if ((add->s = strexndup(s, len)) == NULL) {
		free(add);
		return -1;
	}

	add->next = NULL;

	if ((var = opt->var) == NULL) {
		opt->var = add;
		return 0;
	}

	while (var->next != NULL)
		var = var->next;

	var->next = add;

	return 0;
}

static int
id_parse(Usimsg *msg, Engine *engine)
{
	Usiobj *obj;
	Token *t;

	if ((obj = msg->head) == NULL)
		return -1;

	switch (obj->key) {
	case USIOBJ_PROGNAME:
		if (engine->name != NULL)
			break;
		t = (Token *)(obj + 1);
		engine->name = strexndup(t->s, t->len);
		break;
	case USIOBJ_AUTHOR:
		if (engine->author != NULL)
			break;
		t = (Token *)(obj + 1);
		engine->author = strexndup(t->s, t->len);
		break;
	default:
		break;
	}

	return 0;
}

static int
option_parse(Usimsg *msg, Engine *engine)
{
	Engineopt *opt;
	Usiobj *obj;
	Token *t;
	int type;
	int num;

	if ((obj = msg->head) == NULL)
		return -1;
	if (obj->key != USIOBJ_OPTNAME)
		return -1;
	t = (Token *)(obj + 1);

	if ((obj = obj->next) == NULL)
		return -1;
	if (obj->key != USIOBJ_TYPE)
		return -1;
	type = *(int *)(obj + 1);

	if ((opt = engineopt_create(t->s, t->len, type)) == NULL)
		return -1;

	for (obj = obj->next; obj != NULL; obj = obj->next) {
		switch (obj->key) {
		case USIOBJ_DEFAULT:
			switch (type) {
			case USIOPT_CHECK:
				num = *(int *)(obj + 1);
				opt->def = (void *)(intptr_t)num;
				opt->cur = opt->def;
				break;
			case USIOPT_SPIN:
				num = *(int *)(obj + 1);
				opt->def = (void *)(intptr_t)num;
				opt->cur = opt->def;
				break;
			case USIOPT_BUTTON:
				break;
			case USIOPT_COMBO:
			case USIOPT_STRING:
			case USIOPT_FILENAME:
				t = (Token *)(obj + 1);
				if ((opt->def = strexndup(t->s, t->len))
						== NULL)
					goto err;
				if ((opt->cur = strexndup(t->s, t->len))
						== NULL)
					goto err;
				break;
			default:
				break;
			}
			break;
		case USIOBJ_MIN:
			num = *(int *)(obj + 1);
			opt->min = num;
			break;
		case USIOBJ_MAX:
			num = *(int *)(obj + 1);
			opt->max = num;
			break;
		case USIOBJ_VAR:
			t = (Token *)(obj + 1);
			if (optvar_add(opt, t->s, t->len) != 0)
				goto err;
			break;
		default:
			break;
		}
	}

	engineopt_add(engine, opt);
	return 0;
err:
	engineopt_destroy(opt);
	return -1;
}

static int
info_parse(Usimsg *msg, Movecap *cap)
{
	Usiobj *obj;
	Token *t;
	int num;

	for (obj = msg->head; obj != NULL; obj = obj->next) {
		switch (obj->key) {
		case USIOBJ_DEPTH:
			num = *(int *)(obj + 1);
			cap->depth = num;
			break;
		case USIOBJ_SELDEPTH:
			num = *(int *)(obj + 1);
			cap->seldepth = num;
			break;
		case USIOBJ_SCORE_CP:
			num = *(int *)(obj + 1);
			cap->score = num;
			break;
		case USIOBJ_SCORE_MATE:
			num = *(int *)(obj + 1);
			cap->score = 999999;
			if (num & (1 << 31))
				cap->score *= -1;
			break;
		case USIOBJ_PV:
			t = (Token *)(obj + 1);
			printf("pv: %.*s\n", (int)t->len, t->s);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int
bestmove_parse(Usimsg *msg, Move *move)
{
	Usiobj *obj;
	Token *t;
	Stream s;

	if ((obj = msg->head) == NULL)
		return -1;

	switch (obj->key) {
	case USIOBJ_RESIGN:
		return RESIGN;
	case USIOBJ_WIN:
		return WIN;
	case USIOBJ_MOVE:
		t = (Token *)(obj + 1);
		stream_strinit(&s, t->s);
		if (move_parse(move, &s) != 0)
			return -1;
		return MOVE;
	default:
		return -1;
	}
}

static int
checkmate_parse(Usimsg *msg, Movelog *mate)
{
	Usiobj *obj;
	Token *t;
	Stream s;

	if ((obj = msg->head) == NULL)
		return -1;

	switch (obj->key) {
	case USIOBJ_NOMATE:
		return NOMATE;
	case USIOBJ_MOVE:
		t = (Token *)(obj + 1);
		stream_strinit(&s, t->s);
		movelog_init(mate);
		if (movelog_load_stream(mate, &s) != 0)
			return -1;
		return MOVE;
	default:
		return -1;
	}
}

static int
infomate_parse(Usimsg *msg, Movelog *mate)
{
	Usiobj *obj;
	Token *t;
	Stream s;
	int num;

	if ((obj = usiobj_find(msg, USIOBJ_SCORE_MATE)) == NULL)
		return -1;

	num = *(int *)(obj + 1);
	if (num < 0)
		return -1;

	if ((obj = usiobj_find(msg, USIOBJ_PV)) == NULL)
		return -1;

	t = (Token *)(obj + 1);
	stream_strinit(&s, t->s);
	movelog_init(mate);
	if (movelog_load_stream(mate, &s) != 0)
		return -1;

	return MOVE;
}

static int
position_to_stream(Stream *out, const Game *game)
{
	Move *m;
	int i;

	if (game->turn > game->movelog.n) {
		error("turn over: %d > %d.", game->turn, game->movelog.n);
		return -1;
	}

	stream_puts(out, "position ");
	if (game->start == NULL)
		stream_puts(out, "startpos");
	else {
		stream_puts(out, "sfen ");
		stream_puts(out, game->start);
	}

	stream_puts(out, " moves");
	for (i = 0; i < game->turn; i++) {
		m = game->movelog.list + i;
		stream_putc(out, ' ');
		move_to_stream(out, m);
	}
	stream_putc(out, '\n');

	return 0;
}

static int
tx(Engine *engine, const char *req)
{
	infoout(engine, "U", req);

	stream_puts(&engine->w, req);
	stream_putc(&engine->w, '\n');

	return 0;
}

static int
rx(Engine *engine, Buf *buf)
{
	buf_clear(buf);

	if (stream_gets(&engine->r, buf) != 0)
		return -1;

	infoout(engine, "E", buf->b);

	return 0;
}

static void
infoout(Engine *engine, const char *ident, const char *info)
{
	if (engine->infolevel <= 0)
		return;
	if (ident == NULL)
		return;

	printf("%s: %s\n", ident, info);
}
