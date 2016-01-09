#include <stdlib.h>
#include <string.h>

#include "usi.h"

typedef struct Tbl Tbl;

struct Tbl {
	const char *name;
	int id;
	int (*parse)(Usimsg *, const char *);
};

static int id_parse(Usimsg *, const char *);
static int option_parse(Usimsg *, const char *);
static int info_parse(Usimsg *, const char *);
static int bestmove_parse(Usimsg *, const char *);
static int checkmate_parse(Usimsg *, const char *);

static Tbl *tbl_find(Tbl *, Token *);

int usimsg_pushnil(Usimsg *, int);
int usimsg_pushstr(Usimsg *, int, const char *, size_t);
int usimsg_pushnum(Usimsg *, int, int);
void *usiobj_create(Usimsg *, int, size_t);

const char *token_get(Token *, const char *);
int token_equal(Token *, const char *);

static Tbl tbl_msg[] = {
	{"id", USIMSG_ID, id_parse},
	{"option", USIMSG_OPTION, option_parse},
	{"usiok", USIMSG_OK, NULL},
	{"readyok", USIMSG_READYOK, NULL},
	{"info", USIMSG_INFO, info_parse},
	{"bestmove", USIMSG_BESTMOVE, bestmove_parse},
	{"checkmate", USIMSG_CHECKMATE, checkmate_parse},
	{NULL, 0, NULL}
};

static Tbl tbl_option_type[] = {
	{"check", USIOPT_CHECK, NULL},
	{"spin", USIOPT_SPIN, NULL},
	{"combo", USIOPT_COMBO, NULL},
	{"button", USIOPT_BUTTON, NULL},
	{"string", USIOPT_STRING, NULL},
	{"filename", USIOPT_FILENAME, NULL},
	{NULL, 0, NULL}
};

static Tbl tbl_option[] = {
	{"default", USIOBJ_DEFAULT, NULL},
	{"min", USIOBJ_MIN, NULL},
	{"max", USIOBJ_MAX, NULL},
	{"var", USIOBJ_VAR, NULL},
	{NULL, 0, NULL}
};

int
usimsg_parse(Usimsg *msg, const char *s)
{
	const char *p;
	Token token;
	Tbl *tbl;

	if ((p = token_get(&token, s)) == NULL)
		return -1;

	msg->head = NULL;
	msg->tail = NULL;

	if ((tbl = tbl_find(tbl_msg, &token)) == NULL)
		return 0;

	msg->type = tbl->id;
	if (tbl->parse == NULL)
		return 0;

	(*tbl->parse)(msg, p);

	return 0;
}

void
usimsg_destroy(Usimsg *msg)
{
	Usiobj *obj;
	Usiobj *next;

	for (obj = msg->head; obj != NULL; obj = next) {
		next = obj->next;
		free(obj);
	}
}

Usiobj *
usiobj_find(Usimsg *msg, int key)
{
	Usiobj *obj;

	for (obj = msg->head; obj != NULL; obj = obj->next)
		if (obj->key == key)
			return obj;

	return NULL;
}

const char *
usiopttypestr(int type)
{
	const char *list[] = {
		"check",
		"spin",
		"combo",
		"button",
		"string",
		"filename"
	};
	const int n = sizeof(list) / sizeof(list[0]);

	if (type < 0 || type >= n)
		return "-";
	return list[type];
}

/*
 * id
 *   name <program name>
 *   author <program author>
 */
static int
id_parse(Usimsg *msg, const char *s)
{
	const char *p;
	Token token;

	if ((p = token_get(&token, s)) == NULL)
		return -1;

	if (token_equal(&token, "name"))
		return usimsg_pushstr(msg, USIOBJ_PROGNAME, p, strlen(p));

	if (token_equal(&token, "author"))
		return usimsg_pushstr(msg, USIOBJ_AUTHOR, p, strlen(p));

	return -1;
}

/*
 * option
 *   name <optionname> type <optiontype> <parameter...>
 *   <optiontype> =
 *     check
 *     spin
 *     combo
 *     button
 *     string
 *     filename
 *   <parameter> =
 *     default <x>
 *     min <x>
 *     max <x>
 *     var <x1> var <x2> ...
 */
static int
option_parse(Usimsg *msg, const char *s)
{
	const char *p;
	Token token;
	Tbl *tbl;
	int type;
	int num;

	if ((p = token_get(&token, s)) == NULL)
		return -1;
	if (!token_equal(&token, "name"))
		return -1;

	if ((p = token_get(&token, p)) == NULL)
		return -1;
	if (usimsg_pushstr(msg, USIOBJ_OPTNAME, token.s, token.len) != 0)
		return -1;

	if ((p = token_get(&token, p)) == NULL)
		return -1;
	if (!token_equal(&token, "type"))
		return -1;

	if ((p = token_get(&token, p)) == NULL)
		return -1;
	if ((tbl = tbl_find(tbl_option_type, &token)) == NULL)
		return -1;
	if (usimsg_pushnum(msg, USIOBJ_TYPE, tbl->id) != 0)
		return -1;
	type = tbl->id;

	while ((p = token_get(&token, p)) != NULL) {
		if ((tbl = tbl_find(tbl_option, &token)) == NULL)
			continue;
		switch (tbl->id) {
		case USIOBJ_DEFAULT:
			switch (type) {
			case USIOPT_CHECK:
				if ((p = token_get(&token, p)) == NULL)
					return -1;
				if (token_equal(&token, "true"))
					num = 1;
				else if (token_equal(&token, "false"))
					num = 0;
				else
					return -1;
				if (usimsg_pushnum(msg, tbl->id, num) != 0)
					return -1;
				break;
			case USIOPT_SPIN:
				if ((p = token_get(&token, p)) == NULL)
					return -1;
				num = strtol(token.s, NULL, 10);
				if (usimsg_pushnum(msg, tbl->id, num) != 0)
					return -1;
				break;
			case USIOPT_BUTTON:
				break;
			case USIOPT_COMBO:
			case USIOPT_STRING:
			case USIOPT_FILENAME:
				if ((p = token_get(&token, p)) == NULL)
					return -1;
				if (token_equal(&token, "<empty>"))
					token.len = 0;
				if (usimsg_pushstr(msg, tbl->id,
						token.s, token.len) != 0)
					return -1;
				break;
			default:
				break;
			}
			break;
		case USIOBJ_MIN:
		case USIOBJ_MAX:
			if ((p = token_get(&token, p)) == NULL)
				return -1;
			num = strtol(token.s, NULL, 10);
			if (usimsg_pushnum(msg, tbl->id, num) != 0)
				return -1;
			break;
		case USIOBJ_STRING:
			if ((p = token_get(&token, p)) == NULL)
				return -1;
			if (usimsg_pushstr(msg, tbl->id,
					token.s, strlen(token.s)) != 0)
				return -1;
			break;
		default:
			break;
		}
	}

	return 0;
}

/*
 * info
 *   depth <x>
 *   seldepth <x>
 *   time <x>
 *   nodes <x>
 *   pv <move1> ... <movei>
 *   score
 *     cp <x>
 *     mate <y>
 *   currmove <move>
 *   hashfull <x>
 *   nps <x>
 *   string <str>
 */
static int
info_parse(Usimsg *msg, const char *s)
{
	const char *p;
	Token token;
	int type;
	int key;
	int num;

	p = s;
	while ((p = token_get(&token, p)) != NULL) {
		if (token_equal(&token, "depth")) {
			type = USITYPE_NUM;
			key = USIOBJ_DEPTH;
		} else if (token_equal(&token, "seldepth")) {
			type = USITYPE_NUM;
			key = USIOBJ_SELDEPTH;
		} else if (token_equal(&token, "time")) {
			type = USITYPE_NUM;
			key = USIOBJ_TIME;
		} else if (token_equal(&token, "nodes")) {
			type = USITYPE_NUM;
			key = USIOBJ_NODES;
		} else if (token_equal(&token, "pv")) {
			type = USITYPE_OCT;
			key = USIOBJ_PV;
		} else if (token_equal(&token, "score")) {
			if ((p = token_get(&token, p)) == NULL)
				return -1;
			if (token_equal(&token, "cp")) {
				type = USITYPE_NUM;
				key = USIOBJ_SCORE_CP;
			} else if (token_equal(&token, "mate")) {
				type = USITYPE_NUM;
				key = USIOBJ_SCORE_MATE;
			} else {
				/* rollback */
				p = token.s;
				continue;
			}
		} else if (token_equal(&token, "currmove")) {
			type = USITYPE_STR;
			key = USIOBJ_CURRMOVE;
		} else if (token_equal(&token, "hashfull")) {
			type = USITYPE_NUM;
			key = USIOBJ_HASHFULL;
		} else if (token_equal(&token, "nps")) {
			type = USITYPE_NUM;
			key = USIOBJ_NPS;
		} else if (token_equal(&token, "string")) {
			type = USITYPE_STR;
			key = USIOBJ_STRING;
		} else {
			continue;
		}

		switch (type) {
		case USITYPE_NUM:
			if ((p = token_get(&token, p)) == NULL)
				return -1;
			num = strtol(token.s, NULL, 10);
			if (usimsg_pushnum(msg, key, num) != 0)
				return -1;
			break;
		case USITYPE_STR:
			if ((p = token_get(&token, p)) == NULL)
				return -1;
			if (usimsg_pushstr(msg, key, token.s, token.len) != 0)
				return -1;
			break;
		case USITYPE_OCT:
			if ((p = token_get(&token, p)) == NULL)
				return -1;
			if (usimsg_pushstr(msg, key, token.s,
					strlen(token.s)) != 0)
				return -1;
			break;
		default:
			break;
		}
	}

	return 0;
}

/*
 * bestmove
 *   resign
 *   win
 *   <move1> [ponder <move2>]
 */
static int
bestmove_parse(Usimsg *msg, const char *s)
{
	const char *p;
	Token token;

	if ((p = token_get(&token, s)) == NULL)
		return -1;

	if (token_equal(&token, "resign"))
		return usimsg_pushnil(msg, USIOBJ_RESIGN);
	if (token_equal(&token, "win"))
		return usimsg_pushnil(msg, USIOBJ_WIN);

	if (usimsg_pushstr(msg, USIOBJ_MOVE, token.s, token.len) != 0)
		return -1;

	if ((p = token_get(&token, p)) == NULL)
		return 0;
	if (!token_equal(&token, "ponder"))
		return 0;
	if ((p = token_get(&token, p)) == NULL)
		return -1;
	if (usimsg_pushstr(msg, USIOBJ_MOVE, token.s, token.len) != 0)
		return -1;

	return 0;
}

/*
 * checkmate
 *   notimplemented
 *   timeout
 *   nomate
 *   <move1> ... <movei>
 */
static int
checkmate_parse(Usimsg *msg, const char *s)
{
	const char *p;
	Token token;

	if ((p = token_get(&token, s)) == NULL)
		return -1;

	if (token_equal(&token, "notimplemented"))
		return usimsg_pushnil(msg, USIOBJ_NOTIMPLEMENTED);
	if (token_equal(&token, "timeout"))
		return usimsg_pushnil(msg, USIOBJ_TIMEOUT);
	if (token_equal(&token, "nomate"))
		return usimsg_pushnil(msg, USIOBJ_NOMATE);

	return usimsg_pushstr(msg, USIOBJ_MOVE, token.s, strlen(token.s));
}


static Tbl *
tbl_find(Tbl *root, Token *token)
{
	Tbl *e;

	for (e = root; e->name != NULL; e++)
		if (token_equal(token, e->name))
			return e;

	return NULL;
}

int
usimsg_pushnil(Usimsg *msg, int key)
{
	if (usiobj_create(msg, key, 0) == NULL)
		return -1;

	return 0;
}

int
usimsg_pushstr(Usimsg *msg, int key, const char *s, size_t len)
{
	Token *v;

	if ((v = usiobj_create(msg, key, sizeof(Token))) == NULL)
		return -1;

	v->s = s;
	v->len = len;

	return 0;
}

int
usimsg_pushnum(Usimsg *msg, int key, int num)
{
	int *v;

	if ((v = usiobj_create(msg, key, sizeof(int))) == NULL)
		return -1;

	*v = num;

	return 0;
}

void *
usiobj_create(Usimsg *msg, int key, size_t size)
{
	Usiobj *obj;

	if ((obj = malloc(sizeof(Usiobj) + size)) == NULL)
		return NULL;

	obj->key = key;
	obj->next = NULL;

	if (msg->tail != NULL)
		msg->tail->next = obj;

	msg->tail = obj;

	if (msg->head == NULL)
		msg->head = obj;

	return obj + 1;
}

const char *
token_get(Token *token, const char *s)
{
	const char *p;

	token->s = s;

	if ((token->len = strcspn(s, " ")) == 0)
		return NULL;

	p = token->s + token->len;

	return p + strspn(p, " ");
}

int
token_equal(Token *token, const char *s)
{
	if (strncmp(token->s, s, token->len) != 0)
		return 0;

	if (s[token->len] != '\0')
		return 0;

	return 1;
}
