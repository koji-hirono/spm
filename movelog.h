#ifndef MOVELOG_H_
#define MOVELOG_H_

#include "stream.h"
#include "board.h"

typedef struct Movelog Movelog;

struct Movelog {
	Movelog *parent;
	int from;
	int n;
	Move *list;
	Movecap *cap;
};

extern void movelog_init(Movelog *);
extern void movelog_destroy(Movelog *);
extern int movelog_add(Movelog *, Move *);
extern void movelog_branch(Movelog *, Movelog *, int);
extern int movelog_save(Movelog *, const char *);
extern int movelog_load_stream(Movelog *, Stream *);
extern int movelog_load(Movelog *, const char *);

#endif /* !MOVELOG_H_ */
