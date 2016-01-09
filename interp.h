#ifndef INTERP_H_
#define INTERP_H_

#include "stream.h"
#include "game.h"
#include "engine.h"

typedef struct Interp Interp;

struct Interp {
	Stream *in;
	Stream *out;
	Game *game;
	Engine *engine;
};

extern void interp_init(Interp *);
extern void interp_loop(Interp *);

#endif /* !INTERP_H_ */
