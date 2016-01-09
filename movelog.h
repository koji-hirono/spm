#ifndef MOVELOG_H_
#define MOVELOG_H_

#include "stream.h"
#include "board.h"

typedef struct Movelog Movelog;

struct Movelog {
	int n;
	Move *list;
};

extern void movelog_init(Movelog *);
extern void movelog_destroy(Movelog *);
extern int movelog_add(Movelog *, Move *);
extern int movelog_save(Movelog *, const char *);
extern int movelog_load(Movelog *, const char *);
extern int movelog_load_stream(Movelog *, Stream *);

#endif /* !MOVELOG_H_ */
