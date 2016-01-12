#ifndef ENGINE_H_
#define ENGINE_H_

#include <unistd.h>

#include "stream.h"
#include "board.h"
#include "movelog.h"
#include "game.h"
#include "usi.h"

typedef struct Engine Engine;
typedef struct Engineopt Engineopt;
typedef struct Optvar Optvar;

/* engine status */
enum {
	ENGINE_SPAWNED,
	ENGINE_INITED,
	ENGINE_STARTED
};

struct Optvar {
	Optvar *next;
	char *s;
};

struct Engineopt {
	Engineopt *next;
	char *name;
	int type;
	void *cur;
	void *def;
	int min;
	int max;
	Optvar *var;
};

struct Engine {
	int status;
	char *name;
	char *author;
	pid_t pid;
	int infolevel;
	Stream w;
	Stream r;
	Engineopt *opt;
};

extern int engine_open(Engine *, const char *, int);
extern void engine_close(Engine *);
extern int engine_init(Engine *);
extern int engine_start(Engine *);
extern int engine_setoption(Engine *, const char *, const char *);
extern int engine_move(Engine *, Move *, Movecap *, const Game *);
extern int engine_mate(Engine *, Movelog *, const Game *);
extern const char *engine_status_str(int);
extern void engineopt_destroy(Engineopt *);
extern Engineopt *engineopt_create(const char *, size_t, int);
extern void engineopt_add(Engine *, Engineopt *);
extern int optvar_add(Engineopt *, const char *, size_t);

#endif /* !ENGINE_H_ */
